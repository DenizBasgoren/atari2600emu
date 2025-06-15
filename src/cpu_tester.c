// gcc src/cpu_tester.c src/cpu.c -lcjson -g && ./a.out


#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <cjson/cJSON.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "atari2600emulator.h"


typedef struct {
    uint16_t address;
    uint8_t value;
    bool isWrite;
} Av;

Av logs[100] = {0};
int logs_len = 0;

void empty_logs (void ) {
    logs_len = 0;
    memset(logs, 0, sizeof(Av) * 100);
}

void add_log( Av av ) {
    if (logs_len==100) {
        logs[99] = av;
    }
    else {
        logs[logs_len++] = av;
    }
}

uint8_t mem64k[0x10000];

uint8_t memory_read(uint16_t address) {
    Av av = {.address=address, .value=mem64k[address], .isWrite=false };
    add_log( av );
    return av.value;
}

void memory_write(uint16_t address, uint8_t value) {
    Av av = {.address=address, .value=value, .isWrite=true };
    add_log(av);
    mem64k[address] = value;
}




bool do_test( cJSON *test ) {
    // clear logs
    empty_logs();

    // set initial cpu to whats in the test.initial
    memset( &cpu, 0, sizeof(Cpu) );
    cpu.cycle = 1;
    cJSON *initial = cJSON_GetObjectItemCaseSensitive(test, "initial");
    {
        cJSON *pc = cJSON_GetObjectItemCaseSensitive(initial, "pc");
        cJSON *s = cJSON_GetObjectItemCaseSensitive(initial, "s");
        cJSON *a = cJSON_GetObjectItemCaseSensitive(initial, "a");
        cJSON *x = cJSON_GetObjectItemCaseSensitive(initial, "x");
        cJSON *y = cJSON_GetObjectItemCaseSensitive(initial, "y");
        cJSON *p = cJSON_GetObjectItemCaseSensitive(initial, "p");
        cpu.pc = pc->valueint;
        cpu.s = s->valueint;
        cpu.a = a->valueint;
        cpu.x = x->valueint;
        cpu.y = y->valueint;
        cpu_set_flags_from_integer(p->valueint);
    }

    // clear mem
    memset(mem64k, 0, 0x10000 );

    // fill in initial mem according to test.initial
    {
        cJSON *ram = cJSON_GetObjectItemCaseSensitive(initial, "ram");
        cJSON *item;
        cJSON_ArrayForEach(item, ram) {
            cJSON *addr = cJSON_GetArrayItem(item, 0);
            cJSON *value = cJSON_GetArrayItem(item, 1);
            mem64k[addr->valueint] = value->valueint;
        }
    }

    // cycle cpu until cycle=1
    do {
        cpu_run_for_one_machine_cycle();
    } while(cpu.cycle!=1);

    // compare final cpu state
    cJSON *final = cJSON_GetObjectItemCaseSensitive(test, "final");
    {
        cJSON *pc = cJSON_GetObjectItemCaseSensitive(final, "pc");
        cJSON *s = cJSON_GetObjectItemCaseSensitive(final, "s");
        cJSON *a = cJSON_GetObjectItemCaseSensitive(final, "a");
        cJSON *x = cJSON_GetObjectItemCaseSensitive(final, "x");
        cJSON *y = cJSON_GetObjectItemCaseSensitive(final, "y");
        cJSON *p = cJSON_GetObjectItemCaseSensitive(final, "p");
        if (cpu.pc != pc->valueint) {
            printf("pc must be %hu (%04hx) but is %hu (%04hx)\n", pc->valueint, pc->valueint, cpu.pc, cpu.pc);
            return false;
        }
        if (cpu.s != s->valueint) {
            printf("s must be %hhu (%02hhx) but is %hhu (%02hhx)\n", s->valueint, s->valueint, cpu.s, cpu.s);
            return false;
        }
        if (cpu.a != a->valueint) {
            printf("a must be %hhu (%02hhx) but is %hhu (%02hhx)\n", a->valueint, a->valueint, cpu.a, cpu.a);
            return false;
        }
        if (cpu.x != x->valueint) {
            printf("x must be %hhu (%02hhx) but is %hhu (%02hhx)\n", x->valueint, x->valueint, cpu.x, cpu.x);
            return false;
        }
        if (cpu.y != y->valueint) {
            printf("y must be %hhu (%02hhx) but is %hhu (%02hhx)\n", y->valueint, y->valueint, cpu.y, cpu.y);
            return false;
        }
        if ((cpu_get_flags_as_integer() & 0xEF) != (p->valueint & 0xEF) ) {
            printf("p must be %08hhb but is %08hhb\n",  p->valueint & 0xEF,
                                                        cpu_get_flags_as_integer() & 0xEF
                                                        );
            return false;
        }
    }
    // compare final ram
    {
        cJSON *ram = cJSON_GetObjectItemCaseSensitive(final, "ram");
        cJSON *item;
        cJSON_ArrayForEach(item, ram) {
            cJSON *addr = cJSON_GetArrayItem(item, 0);
            cJSON *value = cJSON_GetArrayItem(item, 1);
            if (mem64k[addr->valueint] != value->valueint) {
                printf("ram[%d] must be %hhu (%02hhx) but is %hhu (%02hhx)\n", addr->valueint,
                                                                                value->valueint, value->valueint, 
                                                                                mem64k[addr->valueint], mem64k[addr->valueint] );
                return false;
            }
        }
    }

    // compare cycles
    {
        cJSON *cycles = cJSON_GetObjectItemCaseSensitive(test, "cycles");
        cJSON *item;
        int cycle = 0;
        cJSON_ArrayForEach(item, cycles) {
            uint16_t addr = cJSON_GetArrayItem(item, 0)->valueint;
            uint8_t value = cJSON_GetArrayItem(item, 1)->valueint;
            bool isWrite = cJSON_GetArrayItem(item, 2)->valuestring[0] == 'w';

            if (    logs[cycle].address != addr
                ||  logs[cycle].value != value
                ||  logs[cycle].isWrite != isWrite) {
                printf("cycle %d \n", cycle);
                printf("addr should be %d but is %d\n", addr, logs[cycle].address);
                printf("val should be %d but is %d\n", value, logs[cycle].value);
                printf("isWrite should be %d but is %d\n", isWrite, logs[cycle].isWrite);
                return false;
            }
            cycle++;
        }
    }

    return true;
}

bool test_file( char *filepath ) {
    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        puts("no tests");
        return false;
    }

    static char fileContents[10*1024*1024];
    memset( fileContents, 0, sizeof(fileContents));

    read(fd, fileContents, 10*1024*1024);

    cJSON *tests = cJSON_Parse(fileContents);
    if (!tests) {
        puts("Cant parse");
        return false;
    }

    cJSON *test;
    int i = 0;
    cJSON_ArrayForEach(test, tests) {
        bool succ = do_test(test);
        if (!succ) {
            printf("test i=%d failed\n", i);
            return false;
        }
        i++;
    }
    return true;
}

int main(void) {

    for (int i = 0x0; i<256; i++) {

        // if (i==0x6B || i==0x8B || i==0xAB || i==0x93 || i==0x9F || i==0x9C || i==0x9E || i==0x9B  ) continue;

        char *c = "/home/korsan/proj/atari2600emu/cpu_tests/%02hhx.json";
        char filename[100] = {0};
        sprintf(filename, c, i);
        printf("%s \n", filename);
        bool succ = test_file(filename);
        if (!succ) break;
    }
}