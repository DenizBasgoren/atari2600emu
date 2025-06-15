#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "atari2600emulator.h"


Cpu cpu;

// note: 6B, 8B, AB, 93, 9F, 9C, 9E, 9B are not implemented
// for illegal instructions refer to: https://www.masswerk.at/nowgobang/2021/6502-illegal-opcodes

uint8_t cpu_get_flags_as_integer( ) {
    return cpu.flag_n<<7
        | cpu.flag_v<<6
        | 1<<5
        | 1<<4
        | cpu.flag_d<<3
        | cpu.flag_i<<2
        | cpu.flag_z<<1
        | cpu.flag_c;
}

void cpu_set_flags_from_integer( uint8_t value) {
    cpu.flag_n = (value & (1 << 7)) != 0;
    cpu.flag_v = (value & (1 << 6)) != 0;
    cpu.flag_d = (value & (1 << 3)) != 0;
    cpu.flag_i = (value & (1 << 2)) != 0;
    cpu.flag_z = (value & (1 << 1)) != 0;
    cpu.flag_c = (value & (1 << 0)) != 0;
}


void cpu_update_nz_flags( const uint8_t value) {
  cpu.flag_z = value==0;
  cpu.flag_n = value>127;
}


void cpu_update_cv_flags(const uint8_t arg1, const uint8_t arg2, const bool carry) {
    int sum = arg1 + arg2 + carry;
    cpu.flag_v = (((arg1 ^ arg2) & 0x80) == 0) && (((arg1 ^ sum) & 0x80) != 0);
    cpu.flag_c = sum > 255;
}


void cpu_update_cv_flags_subtraction( const uint8_t arg1, const uint8_t arg2, const bool carry) {
    int difference = arg1 + carry - 1 - arg2;
    cpu.flag_v = (arg1 ^ arg2) & (arg1 ^ difference) & 0x80;
    cpu.flag_c = (difference & 0xFF00) == 0;
}

// adapted from https://github.com/stella-emu/stella/blob/master/src/emucore/M6502.ins
void cpu_add_in_bcd_mode(uint8_t arg2 ) {
    int A = cpu.a;
    int AL, AH;
    int C = cpu.flag_c;
    int Z = cpu.flag_z;
    int V = cpu.flag_v;
    int N = cpu.flag_n;
    int B = arg2;

    AL = (A & 0x0F) + (B & 0x0F) + C;
    AH = (A & 0xF0) + (B & 0xF0);
    Z = !((AL + AH) & 0xFF);
    if (AL >= 10) {
        AH += 0x10;
        AL += 0x06;
    }
    N = AH & 0x80;
    V = ~(A ^ B) & (A ^ AH) & 0x80;
    if (AH > 0x90) AH += 0x60;
    C = AH & 0xFF00;
    A = (AL & 0x0F) + (AH & 0xF0);
    
    cpu.a = A;
    cpu.flag_c = C;
    cpu.flag_z = Z;
    cpu.flag_n = N;
    cpu.flag_v = V;
}

// adapted from https://github.com/stella-emu/stella/blob/master/src/emucore/M6502.ins
void cpu_subtract_in_bcd_mode( uint8_t arg2 ) {
    int A = cpu.a;
    int AL, AH;
    int C = cpu.flag_c;
    int B = arg2;

    AL = (A & 0x0F) - (B & 0x0F) + C-1;
    AH = (A & 0xF0) - (B & 0xF0);
    if (AL & 0x10) {
        AL -= 6;
        AH -= 1;
    }
    if (AH & 0x100) AH -= 0x60;

    cpu.a = (AL & 0x0F) | (AH & 0xF0);
    
    int sum = A - B + C-1;
    cpu_update_cv_flags_subtraction(A, B, C);
    cpu_update_nz_flags(sum);
}


void cpu_push_to_stack( uint8_t value) {
    memory_write( 0x100 + cpu.s , value);
    cpu.s -= 1;
}


uint8_t cpu_pop_from_stack( void ) {
    cpu.s += 1;
    return memory_read( 0x100 + cpu.s );
}


void cpu_go_to_reset_vector( void ) {
    uint8_t low = memory_read(0xFFFC);
    uint8_t high = memory_read(0xFFFD);
    cpu.pc = high<<8 | low;
    cpu.flag_v = 1; // stella does it this way
    cpu.cycle = 1;
}























// returns the delay (measured in cpu cycles) after which the change in cpu and memory will take effect
void cpu_run_for_one_machine_cycle ( void ) {

    if (cpu.is_killed) return; // one cycle delay

    if (cpu.cycle == 1) {
        cpu.opcode = memory_read( cpu.pc );
        cpu.cycle++;
        return;
    }

    switch(cpu.opcode) {

    case 0xA8: // TAY
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.y = cpu.a;
            cpu_update_nz_flags(cpu.y);
            cpu.pc += 1;
            cpu.cycle = 1;
        }
        return;
    }

    case 0xAA: // TAX
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.x = cpu.a;
            cpu_update_nz_flags(cpu.x);
            cpu.pc += 1;
            cpu.cycle = 1;
        }
        return;
    }

    case 0xBA: // TSX
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.x = cpu.s;
            cpu_update_nz_flags(cpu.x);
            cpu.pc += 1;
            cpu.cycle = 1;
        }
        return;
    }

    case 0x98: // TYA
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.a = cpu.y;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 1;
            cpu.cycle = 1;
        }
        return;
    }

    case 0x8A: // TXA
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.a = cpu.x;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 1;
            cpu.cycle = 1;
        }
        return;
    }

    case 0x9A: // TXS
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.s = cpu.x;
            cpu.pc += 1;
            cpu.cycle = 1;
        }
        return;
    }

    case 0xA9: // LDA_immediate
    {
        if (cpu.cycle==2) {
            cpu.a = memory_read(cpu.pc + 1);
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    case 0xA2: // LDX_immediate
    {
        if (cpu.cycle==2) {
            cpu.x = memory_read(cpu.pc + 1);
            cpu_update_nz_flags(cpu.x);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    case 0xA0: // LDY_immediate
    {
        if (cpu.cycle==2) {
            cpu.y = memory_read(cpu.pc + 1);
            cpu_update_nz_flags(cpu.y);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    case 0xA5: // LDA_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.a = memory_read(cpu.temp1);
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xB5: // LDA_zeropage_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.a = memory_read( (cpu.temp1 + cpu.x) % 256 );
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xAD: // LDA_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.a = memory_read( cpu.temp2<<8 | cpu.temp1 );
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xBD: // LDA_absolute_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.x) % 256);
            cpu.a = memory_read(temp_addr);
            cpu.cycle++;
            if (temp_addr == real_addr) {
                cpu_update_nz_flags(cpu.a);
                cpu.pc += 3;
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            cpu.a = memory_read(real_addr);
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xB9: // LDA_absolute_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.y) % 256);
            cpu.a = memory_read(temp_addr);
            cpu.cycle++;
            if (temp_addr == real_addr) {
                cpu_update_nz_flags(cpu.a);
                cpu.pc += 3;
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            cpu.a = memory_read(real_addr);
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xA1: // LDA_indirect_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = memory_read( (cpu.temp1 + cpu.x) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            cpu.temp3 = memory_read( (cpu.temp1 + cpu.x + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            cpu.a = memory_read( cpu.temp3<<8 | cpu.temp2 );
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xB1: // LDA_indirect_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( (cpu.temp1 + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            uint16_t temp_addr = cpu.temp3<<8 | ((cpu.temp2 + cpu.y) % 256);
            cpu.a = memory_read(temp_addr);
            cpu.cycle++;
            if (temp_addr == real_addr) {
                cpu_update_nz_flags(cpu.a);
                cpu.pc += 2;
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            cpu.a = memory_read(real_addr);
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xA6: // LDX_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.x = memory_read(cpu.temp1);
            cpu_update_nz_flags(cpu.x);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xB6: // LDX_zeropage_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.x = memory_read( (cpu.temp1 + cpu.y) % 256 );
            cpu_update_nz_flags(cpu.x);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xAE: // LDX_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.x = memory_read( cpu.temp2<<8 | cpu.temp1 );
            cpu_update_nz_flags(cpu.x);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xBE: // LDX_absolute_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.y) % 256);
            cpu.x = memory_read(temp_addr);
            cpu.cycle++;
            if (temp_addr == real_addr) {
                cpu_update_nz_flags(cpu.x);
                cpu.pc += 3;
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            cpu.x = memory_read(real_addr);
            cpu_update_nz_flags(cpu.x);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xA4: // LDY_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.y = memory_read(cpu.temp1);
            cpu_update_nz_flags(cpu.y);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0xB4: // LDY_zeropage_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.y = memory_read( (cpu.temp1 + cpu.x) % 256 );
            cpu_update_nz_flags(cpu.y);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xAC: // LDY_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.y = memory_read( cpu.temp2<<8 | cpu.temp1 );
            cpu_update_nz_flags(cpu.y);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xBC: // LDY_absolute_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.x) % 256);
            cpu.y = memory_read(temp_addr);
            cpu.cycle++;
            if (temp_addr == real_addr) {
                cpu_update_nz_flags(cpu.y);
                cpu.pc += 3;
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            cpu.y = memory_read(real_addr);
            cpu_update_nz_flags(cpu.y);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x85: // STA_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_write(cpu.temp1, cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0x95: // STA_zeropage_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            memory_write( (cpu.temp1 + cpu.x) % 256, cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0x8D: // STA_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            memory_write( cpu.temp2<<8 | cpu.temp1, cpu.a );
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x9D: // STA_absolute_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            memory_read( cpu.temp2<<8 | (cpu.temp1 + cpu.x) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            memory_write( (cpu.temp2<<8 | cpu.temp1) + cpu.x, cpu.a);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x99: // STA_absolute_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            memory_read( cpu.temp2<<8 | (cpu.temp1 + cpu.y) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            memory_write( (cpu.temp2<<8 | cpu.temp1) + cpu.y, cpu.a);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x81: // STA_indirect_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = memory_read( (cpu.temp1 + cpu.x) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            cpu.temp3 = memory_read( (cpu.temp1 + cpu.x + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            memory_write( cpu.temp3<<8 | cpu.temp2, cpu.a );
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x91: // STA_indirect_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( (cpu.temp1 + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            memory_read(cpu.temp3<<8 | (cpu.temp2 + cpu.y) % 256);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            memory_write((cpu.temp3<<8 | cpu.temp2) + cpu.y, cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x86: // STX_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_write(cpu.temp1, cpu.x);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x96: // STX_zeropage_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            memory_write( (cpu.temp1 + cpu.y) % 256, cpu.x);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x8E: // STX_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            memory_write( cpu.temp2<<8 | cpu.temp1, cpu.x );
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x84: // STY_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_write(cpu.temp1, cpu.y);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x94: // STY_zeropage_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            memory_write( (cpu.temp1 + cpu.x) % 256, cpu.y);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0x8C: // STY_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            memory_write( cpu.temp2<<8 | cpu.temp1, cpu.y );
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x48: // PHA
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu_push_to_stack( cpu.a );
            cpu.pc += 1;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x08: // PHP
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            uint8_t flags = cpu_get_flags_as_integer();
            cpu_push_to_stack( flags );
            cpu.pc += 1;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x68: // PLA
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.s + 0x100);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.s++;
            cpu.a = memory_read(cpu.s + 0x100);
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 1;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x28: // PLP
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.s + 0x100);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.s++;
            uint8_t flags = memory_read(cpu.s + 0x100);
            cpu_set_flags_from_integer(flags);
            cpu.pc += 1;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x69: // ADC_immediate
    {
        if (cpu.cycle==2) {
            uint8_t arg2 = memory_read(cpu.pc + 1);
            if (cpu.flag_d) {
                cpu_add_in_bcd_mode(arg2);
            }
            else {
                int temp = cpu.a + cpu.flag_c + arg2;
                cpu_update_cv_flags(cpu.a, arg2, cpu.flag_c);
                cpu.a = temp;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    case 0x65: // ADC_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            uint8_t arg2 = memory_read(cpu.temp1);
            if (cpu.flag_d) {
                cpu_add_in_bcd_mode(arg2);
            }
            else {
                int temp = cpu.a + cpu.flag_c + arg2;
                cpu_update_cv_flags(cpu.a, arg2, cpu.flag_c);
                cpu.a = temp;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    case 0x75: // ADC_zeropage_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = cpu.temp1 + cpu.x;
            uint8_t arg2 = memory_read(cpu.temp2);
            if (cpu.flag_d) {
                cpu_add_in_bcd_mode(arg2);
            }
            else {
                int temp = cpu.a + cpu.flag_c + arg2;
                cpu_update_cv_flags(cpu.a, arg2, cpu.flag_c);
                cpu.a = temp;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    case 0x6D: // ADC_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint8_t arg2 = memory_read( cpu.temp2<<8 | cpu.temp1 );
            if (cpu.flag_d) {
                cpu_add_in_bcd_mode(arg2);
            }
            else {
                int temp = cpu.a + cpu.flag_c + arg2;
                cpu_update_cv_flags(cpu.a, arg2, cpu.flag_c);
                cpu.a = temp;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    case 0x7D: // ADC_absolute_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.x) % 256);
            uint8_t arg2 = memory_read(temp_addr);
            cpu.cycle++;
            if (temp_addr == real_addr) {
                if (cpu.flag_d) {
                    cpu_add_in_bcd_mode(arg2);
                }
                else {
                    int temp = cpu.a + cpu.flag_c + arg2;
                    cpu_update_cv_flags(cpu.a, arg2, cpu.flag_c);
                    cpu.a = temp;
                    cpu_update_nz_flags(cpu.a);
                }
                cpu.pc += 3;
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            uint8_t arg2 = memory_read(real_addr);
            if (cpu.flag_d) {
                cpu_add_in_bcd_mode(arg2);
            }
            else {
                int temp = cpu.a + cpu.flag_c + arg2;
                cpu_update_cv_flags(cpu.a, arg2, cpu.flag_c);
                cpu.a = temp;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    case 0x79: // ADC_absolute_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.y) % 256);
            uint8_t arg2 = memory_read(temp_addr);
            cpu.cycle++;
            if (temp_addr == real_addr) {
                if (cpu.flag_d) {
                    cpu_add_in_bcd_mode(arg2);
                }
                else {
                    int temp = cpu.a + cpu.flag_c + arg2;
                    cpu_update_cv_flags(cpu.a, arg2, cpu.flag_c);
                    cpu.a = temp;
                    cpu_update_nz_flags(cpu.a);
                }
                cpu.pc += 3;
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            uint8_t arg2 = memory_read(real_addr);
            if (cpu.flag_d) {
                cpu_add_in_bcd_mode(arg2);
            }
            else {
                int temp = cpu.a + cpu.flag_c + arg2;
                cpu_update_cv_flags(cpu.a, arg2, cpu.flag_c);
                cpu.a = temp;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    case 0x61: // ADC_indirect_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = memory_read( (cpu.temp1 + cpu.x) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            cpu.temp3 = memory_read( (cpu.temp1 + cpu.x + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint8_t arg2 = memory_read( cpu.temp3<<8 | cpu.temp2 );
            if (cpu.flag_d) {
                cpu_add_in_bcd_mode(arg2);
            }
            else {
                int temp = cpu.a + cpu.flag_c + arg2;
                cpu_update_cv_flags(cpu.a, arg2, cpu.flag_c);
                cpu.a = temp;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    case 0x71: // ADC_indirect_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( (cpu.temp1 + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            uint16_t temp_addr = cpu.temp3<<8 | ((cpu.temp2 + cpu.y) % 256);
            uint8_t arg2 = memory_read(temp_addr);
            cpu.cycle++;
            if (temp_addr == real_addr) {
                if (cpu.flag_d) {
                    cpu_add_in_bcd_mode(arg2);
                }
                else {
                    int temp = cpu.a + cpu.flag_c + arg2;
                    cpu_update_cv_flags(cpu.a, arg2, cpu.flag_c);
                    cpu.a = temp;
                    cpu_update_nz_flags(cpu.a);
                }
                cpu.pc += 2;
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            uint8_t arg2 = memory_read(real_addr);
            if (cpu.flag_d) {
                cpu_add_in_bcd_mode(arg2);
            }
            else {
                int temp = cpu.a + cpu.flag_c + arg2;
                cpu_update_cv_flags(cpu.a, arg2, cpu.flag_c);
                cpu.a = temp;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    
    case 0xE9: // SBC_immediate
    {
        if (cpu.cycle==2) {
            uint8_t arg2 = memory_read(cpu.pc + 1);
            if (cpu.flag_d) {
                cpu_subtract_in_bcd_mode(arg2);
            }
            else {
                int temp = cpu.a + cpu.flag_c - 1 - arg2;
                cpu_update_cv_flags_subtraction(cpu.a, arg2, cpu.flag_c);
                cpu.a = temp;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    case 0xE5: // SBC_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            uint8_t arg2 = memory_read(cpu.temp1);
            if (cpu.flag_d) {
                cpu_subtract_in_bcd_mode(arg2);
            }
            else {
                int temp = cpu.a + cpu.flag_c - 1 - arg2;
                cpu_update_cv_flags_subtraction(cpu.a, arg2, cpu.flag_c);
                cpu.a = temp;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0xF5: // SBC_zeropage_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = cpu.temp1 + cpu.x;
            uint8_t arg2 = memory_read(cpu.temp2);
            if (cpu.flag_d) {
                cpu_subtract_in_bcd_mode(arg2);
            }
            else {
                int temp = cpu.a + cpu.flag_c - 1 - arg2;
                cpu_update_cv_flags_subtraction(cpu.a, arg2, cpu.flag_c);
                cpu.a = temp;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0xED: // SBC_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint8_t arg2 = memory_read( cpu.temp2<<8 | cpu.temp1 );
            if (cpu.flag_d) {
                cpu_subtract_in_bcd_mode(arg2);
            }
            else {
                int temp = cpu.a + cpu.flag_c - 1 - arg2;
                cpu_update_cv_flags_subtraction(cpu.a, arg2, cpu.flag_c);
                cpu.a = temp;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0xFD: // SBC_absolute_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.x) % 256);
            uint8_t arg2 = memory_read(temp_addr);
            cpu.cycle++;
            if (temp_addr == real_addr) {
                if (cpu.flag_d) {
                    cpu_subtract_in_bcd_mode(arg2);
                }
                else {
                    int temp = cpu.a + cpu.flag_c - 1 - arg2;
                    cpu_update_cv_flags_subtraction(cpu.a, arg2, cpu.flag_c);
                    cpu.a = temp;
                    cpu_update_nz_flags(cpu.a);
                }
                cpu.pc += 3;
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            uint8_t arg2 = memory_read(real_addr);
            if (cpu.flag_d) {
                cpu_subtract_in_bcd_mode(arg2);
            }
            else {
                int temp = cpu.a + cpu.flag_c - 1 - arg2;
                cpu_update_cv_flags_subtraction(cpu.a, arg2, cpu.flag_c);
                cpu.a = temp;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0xF9: // SBC_absolute_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.y) % 256);
            uint8_t arg2 = memory_read(temp_addr);
            cpu.cycle++;
            if (temp_addr == real_addr) {
                if (cpu.flag_d) {
                    cpu_subtract_in_bcd_mode(arg2);
                }
                else {
                    int temp = cpu.a + cpu.flag_c - 1 - arg2;
                    cpu_update_cv_flags_subtraction(cpu.a, arg2, cpu.flag_c);
                    cpu.a = temp;
                    cpu_update_nz_flags(cpu.a);
                }
                cpu.pc += 3;
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            uint8_t arg2 = memory_read(real_addr);
            if (cpu.flag_d) {
                cpu_subtract_in_bcd_mode(arg2);
            }
            else {
                int temp = cpu.a + cpu.flag_c - 1 - arg2;
                cpu_update_cv_flags_subtraction(cpu.a, arg2, cpu.flag_c);
                cpu.a = temp;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0xE1: // SBC_indirect_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = memory_read( (cpu.temp1 + cpu.x) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            cpu.temp3 = memory_read( (cpu.temp1 + cpu.x + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint8_t arg2 = memory_read( cpu.temp3<<8 | cpu.temp2 );
            if (cpu.flag_d) {
                cpu_subtract_in_bcd_mode(arg2);
            }
            else {
                int temp = cpu.a + cpu.flag_c - 1 - arg2;
                cpu_update_cv_flags_subtraction(cpu.a, arg2, cpu.flag_c);
                cpu.a = temp;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0xF1: // SBC_indirect_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( (cpu.temp1 + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            uint16_t temp_addr = cpu.temp3<<8 | ((cpu.temp2 + cpu.y) % 256);
            uint8_t arg2 = memory_read(temp_addr);
            cpu.cycle++;
            if (temp_addr == real_addr) {
                if (cpu.flag_d) {
                    cpu_subtract_in_bcd_mode(arg2);
                }
                else {
                    int temp = cpu.a + cpu.flag_c - 1 - arg2;
                    cpu_update_cv_flags_subtraction(cpu.a, arg2, cpu.flag_c);
                    cpu.a = temp;
                    cpu_update_nz_flags(cpu.a);
                }
                cpu.pc += 2;
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            uint8_t arg2 = memory_read(real_addr);
            if (cpu.flag_d) {
                cpu_subtract_in_bcd_mode(arg2);
            }
            else {
                int temp = cpu.a + cpu.flag_c - 1 - arg2;
                cpu_update_cv_flags_subtraction(cpu.a, arg2, cpu.flag_c);
                cpu.a = temp;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0x29: // AND_immediate
    {
        if (cpu.cycle==2) {
            cpu.a &= memory_read(cpu.pc + 1);
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    case 0x25: // AND_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.a &= memory_read(cpu.temp1);
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x35: // AND_zeropage_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = cpu.temp1 + cpu.x;
            cpu.a &= memory_read(cpu.temp2);
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0x2D: // AND_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.a &= memory_read( cpu.temp2<<8 | cpu.temp1 );
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x3D: // AND_absolute_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.x) % 256);
            uint8_t value = memory_read(temp_addr);
            cpu.cycle++;
            if (temp_addr == real_addr) {
                cpu.a &= value;
                cpu_update_nz_flags(cpu.a);
                cpu.pc += 3;
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            cpu.a &= memory_read(real_addr);
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0x39: // AND_absolute_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.y) % 256);
            uint8_t value = memory_read(temp_addr);
            cpu.cycle++;
            if (temp_addr == real_addr) {
                cpu.a &= value;
                cpu_update_nz_flags(cpu.a);
                cpu.pc += 3;
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            cpu.a &= memory_read(real_addr);
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0x21: // AND_indirect_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = memory_read( (cpu.temp1 + cpu.x) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            cpu.temp3 = memory_read( (cpu.temp1 + cpu.x + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            cpu.a &= memory_read( cpu.temp3<<8 | cpu.temp2 );
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0x31: // AND_indirect_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( (cpu.temp1 + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            uint16_t temp_addr = cpu.temp3<<8 | ((cpu.temp2 + cpu.y) % 256);
            uint8_t val = memory_read(temp_addr);
            cpu.cycle++;
            if (temp_addr == real_addr) {
                cpu.a &= val;
                cpu_update_nz_flags(cpu.a);
                cpu.pc += 2;
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            cpu.a &= memory_read(real_addr);
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0x49: // EOR_immediate
    {
        if (cpu.cycle==2) {
            cpu.a ^= memory_read(cpu.pc + 1);
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    case 0x45: // EOR_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.a ^= memory_read(cpu.temp1);
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x55: // EOR_zeropage_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = cpu.temp1 + cpu.x;
            cpu.a ^= memory_read(cpu.temp2);
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0x4D: // EOR_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.a ^= memory_read( cpu.temp2<<8 | cpu.temp1 );
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x5D: // EOR_absolute_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.x) % 256);
            uint8_t value = memory_read(temp_addr);
            cpu.cycle++;
            if (temp_addr == real_addr) {
                cpu.a ^= value;
                cpu_update_nz_flags(cpu.a);
                cpu.pc += 3;
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            cpu.a ^= memory_read(real_addr);
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0x59: // EOR_absolute_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.y) % 256);
            uint8_t value = memory_read(temp_addr);
            cpu.cycle++;
            if (temp_addr == real_addr) {
                cpu.a ^= value;
                cpu_update_nz_flags(cpu.a);
                cpu.pc += 3;
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            cpu.a ^= memory_read(real_addr);
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0x41: // EOR_indirect_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = memory_read( (cpu.temp1 + cpu.x) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            cpu.temp3 = memory_read( (cpu.temp1 + cpu.x + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            cpu.a ^= memory_read( cpu.temp3<<8 | cpu.temp2 );
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0x51: // EOR_indirect_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( (cpu.temp1 + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            uint16_t temp_addr = cpu.temp3<<8 | ((cpu.temp2 + cpu.y) % 256);
            uint8_t val = memory_read(temp_addr);
            cpu.cycle++;
            if (temp_addr == real_addr) {
                cpu.a ^= val;
                cpu_update_nz_flags(cpu.a);
                cpu.pc += 2;
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            cpu.a ^= memory_read(real_addr);
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0x09: // ORA_immediate
    {
        if (cpu.cycle==2) {
            cpu.a |= memory_read(cpu.pc + 1);
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    case 0x05: // ORA_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.a |= memory_read(cpu.temp1);
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x15: // ORA_zeropage_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = cpu.temp1 + cpu.x;
            cpu.a |= memory_read(cpu.temp2);
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0x0D: // ORA_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.a |= memory_read( cpu.temp2<<8 | cpu.temp1 );
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x1D: // ORA_absolute_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.x) % 256);
            uint8_t value = memory_read(temp_addr);
            cpu.cycle++;
            if (temp_addr == real_addr) {
                cpu.a |= value;
                cpu_update_nz_flags(cpu.a);
                cpu.pc += 3;
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            cpu.a |= memory_read(real_addr);
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0x19: // ORA_absolute_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.y) % 256);
            uint8_t value = memory_read(temp_addr);
            cpu.cycle++;
            if (temp_addr == real_addr) {
                cpu.a |= value;
                cpu_update_nz_flags(cpu.a);
                cpu.pc += 3;
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            cpu.a |= memory_read(real_addr);
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0x01: // ORA_indirect_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = memory_read( (cpu.temp1 + cpu.x) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            cpu.temp3 = memory_read( (cpu.temp1 + cpu.x + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            cpu.a |= memory_read( cpu.temp3<<8 | cpu.temp2 );
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0x11: // ORA_indirect_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( (cpu.temp1 + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            uint16_t temp_addr = cpu.temp3<<8 | ((cpu.temp2 + cpu.y) % 256);
            uint8_t val = memory_read(temp_addr);
            cpu.cycle++;
            if (temp_addr == real_addr) {
                cpu.a |= val;
                cpu_update_nz_flags(cpu.a);
                cpu.pc += 2;
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            cpu.a |= memory_read(real_addr);
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0xC9: // CMP_immediate
    {
        if (cpu.cycle==2) {
            int difference = cpu.a - memory_read(cpu.pc + 1);
            cpu.flag_c = difference>=0; 
            cpu_update_nz_flags(difference);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0xC5: // CMP_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            int difference = cpu.a - memory_read(cpu.temp1);
            cpu.flag_c = difference>=0; 
            cpu_update_nz_flags(difference);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0xD5: // CMP_zeropage_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = cpu.temp1 + cpu.x;
            int difference = cpu.a - memory_read(cpu.temp2);
            cpu.flag_c = difference>=0; 
            cpu_update_nz_flags(difference);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0xCD: // CMP_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            int difference = cpu.a - memory_read( cpu.temp2<<8 | cpu.temp1 );
            cpu.flag_c = difference>=0; 
            cpu_update_nz_flags(difference);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0xDD: // CMP_absolute_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.x) % 256);
            int difference = cpu.a - memory_read(temp_addr);
            cpu.cycle++;
            if (temp_addr == real_addr) {
                cpu.flag_c = difference>=0; 
                cpu_update_nz_flags(difference);
                cpu.pc += 3;
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            int difference = cpu.a - memory_read(real_addr);
            cpu.flag_c = difference>=0; 
            cpu_update_nz_flags(difference);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0xD9: // CMP_absolute_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.y) % 256);
            int difference = cpu.a - memory_read(temp_addr);
            cpu.cycle++;
            if (temp_addr == real_addr) {
                cpu.flag_c = difference>=0; 
                cpu_update_nz_flags(difference);
                cpu.pc += 3;
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            int difference = cpu.a - memory_read(real_addr);
            cpu.flag_c = difference>=0; 
            cpu_update_nz_flags(difference);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0xC1: // CMP_indirect_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = memory_read( (cpu.temp1 + cpu.x) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            cpu.temp3 = memory_read( (cpu.temp1 + cpu.x + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            int difference = cpu.a - memory_read( cpu.temp3<<8 | cpu.temp2 );
            cpu.flag_c = difference>=0; 
            cpu_update_nz_flags(difference);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0xD1: // CMP_indirect_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( (cpu.temp1 + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            uint16_t temp_addr = cpu.temp3<<8 | ((cpu.temp2 + cpu.y) % 256);
            int difference = cpu.a - memory_read(temp_addr);
            cpu.cycle++;
            if (temp_addr == real_addr) {
                cpu.flag_c = difference>=0; 
                cpu_update_nz_flags(difference);
                cpu.pc += 2;
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            int difference = cpu.a - memory_read(real_addr);
            cpu.flag_c = difference>=0; 
            cpu_update_nz_flags(difference);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0xE0: // CPX_immediate
    {
        if (cpu.cycle==2) {
            int difference = cpu.x - memory_read(cpu.pc + 1);
            cpu.flag_c = difference>=0; 
            cpu_update_nz_flags(difference);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0xE4: // CPX_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            int difference = cpu.x - memory_read(cpu.temp1);
            cpu.flag_c = difference>=0; 
            cpu_update_nz_flags(difference);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0xEC: // CPX_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            int difference = cpu.x - memory_read( cpu.temp2<<8 | cpu.temp1 );
            cpu.flag_c = difference>=0; 
            cpu_update_nz_flags(difference);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0xC0: // CPY_immediate
    {
        if (cpu.cycle==2) {
            int difference = cpu.y - memory_read(cpu.pc + 1);
            cpu.flag_c = difference>=0; 
            cpu_update_nz_flags(difference);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0xC4: // CPY_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            int difference = cpu.y - memory_read(cpu.temp1);
            cpu.flag_c = difference>=0; 
            cpu_update_nz_flags(difference);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0xCC: // CPY_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            int difference = cpu.y - memory_read( cpu.temp2<<8 | cpu.temp1 );
            cpu.flag_c = difference>=0; 
            cpu_update_nz_flags(difference);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0x24: // BIT_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            uint8_t arg = memory_read(cpu.temp1);
            cpu.flag_z = (cpu.a & arg) == 0;
            cpu.flag_n = arg & 0x80;
            cpu.flag_v = arg & 0x40;
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0x2C: // BIT_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint8_t arg = memory_read( cpu.temp2<<8 | cpu.temp1 );
            cpu.flag_z = (cpu.a & arg) == 0;
            cpu.flag_n = arg & 0x80;
            cpu.flag_v = arg & 0x40;
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0xE6: // INC_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            memory_write(cpu.temp1, cpu.temp2);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint8_t result = cpu.temp2 + 1;
            memory_write(cpu.temp1, result);
            cpu_update_nz_flags(result);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xF6: // INC_zeropage_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = memory_read( (cpu.temp1 + cpu.x) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            memory_write( (cpu.temp1 + cpu.x) % 256 , cpu.temp2 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint8_t result = cpu.temp2 + 1;
            memory_write( (cpu.temp1 + cpu.x) % 256 , result );
            cpu_update_nz_flags(result);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xEE: // INC_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( cpu.temp2<<8 | cpu.temp1 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            memory_write( cpu.temp2<<8 | cpu.temp1, cpu.temp3 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint8_t result = cpu.temp3 + 1;
            memory_write( cpu.temp2<<8 | cpu.temp1, result );
            cpu_update_nz_flags(result);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xFE: // INC_absolute_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.x) % 256);
            memory_read(temp_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            cpu.temp3 = memory_read(real_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            memory_write(real_addr, cpu.temp3);
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            uint8_t result = cpu.temp3 + 1;
            memory_write(real_addr, result);
            cpu_update_nz_flags(result);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xE8: // INX
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.x += 1;
            cpu_update_nz_flags(cpu.x);
            cpu.pc += 1;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xC8: // INY
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.y += 1;
            cpu_update_nz_flags(cpu.y);
            cpu.pc += 1;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0xC6: // DEC_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            memory_write(cpu.temp1, cpu.temp2);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint8_t result = cpu.temp2 - 1;
            memory_write(cpu.temp1, result);
            cpu_update_nz_flags(result);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xD6: // DEC_zeropage_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = memory_read( (cpu.temp1 + cpu.x) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            memory_write( (cpu.temp1 + cpu.x) % 256 , cpu.temp2 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint8_t result = cpu.temp2 - 1;
            memory_write( (cpu.temp1 + cpu.x) % 256 , result );
            cpu_update_nz_flags(result);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0xCE: // DEC_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( cpu.temp2<<8 | cpu.temp1 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            memory_write( cpu.temp2<<8 | cpu.temp1, cpu.temp3 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint8_t result = cpu.temp3 - 1;
            memory_write( cpu.temp2<<8 | cpu.temp1, result );
            cpu_update_nz_flags(result);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xDE: // DEC_absolute_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.x) % 256);
            memory_read(temp_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            cpu.temp3 = memory_read(real_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            memory_write(real_addr, cpu.temp3);
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            uint8_t result = cpu.temp3 - 1;
            memory_write(real_addr, result);
            cpu_update_nz_flags(result);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xCA: // DEX
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.x -= 1;
            cpu_update_nz_flags(cpu.x);
            cpu.pc += 1;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x88: // DEY
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.y -= 1;
            cpu_update_nz_flags(cpu.y);
            cpu.pc += 1;
            cpu.cycle = 1;
        }
        return;
    }


    case 0x0A: // ASL
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.flag_c = cpu.a & 0x80;
            cpu.a <<= 1;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 1;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0x06: // ASL_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            memory_write(cpu.temp1, cpu.temp2);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            cpu.flag_c = cpu.temp2 & 0x80;
            uint8_t result = cpu.temp2 << 1;
            cpu_update_nz_flags(result);
            memory_write(cpu.temp1, result);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }
    
    

    case 0x16: // ASL_zeropage_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = cpu.temp1 + cpu.x;
            cpu.temp3 = memory_read(cpu.temp2);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            memory_write(cpu.temp2, cpu.temp3);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            cpu.flag_c = cpu.temp3 & 0x80;
            uint8_t result = cpu.temp3 << 1;
            memory_write(cpu.temp2, result);
            cpu_update_nz_flags(result);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x0E: // ASL_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( cpu.temp2<<8 | cpu.temp1 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            memory_write(cpu.temp2<<8 | cpu.temp1, cpu.temp3);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            cpu.flag_c = cpu.temp3 & 0x80;
            uint8_t result = cpu.temp3 << 1;
            memory_write(cpu.temp2<<8 | cpu.temp1, result);
            cpu_update_nz_flags(result);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x1E: // ASL_absolute_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.x) % 256);
            memory_read(temp_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            cpu.temp3 = memory_read(real_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            memory_write(real_addr, cpu.temp3);
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            cpu.flag_c = cpu.temp3 & 0x80;
            uint8_t result = cpu.temp3 << 1;
            memory_write(real_addr, result);
            cpu_update_nz_flags(result);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x4A: // LSR
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.flag_c = cpu.a & 1;
            cpu.a >>= 1;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 1;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x46: // LSR_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            memory_write(cpu.temp1, cpu.temp2);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            cpu.flag_c = cpu.temp2 & 1;
            uint8_t result = cpu.temp2 >> 1;
            cpu_update_nz_flags(result);
            memory_write(cpu.temp1, result);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x56: // LSR_zeropage_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = cpu.temp1 + cpu.x;
            cpu.temp3 = memory_read(cpu.temp2);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            memory_write(cpu.temp2, cpu.temp3);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            cpu.flag_c = cpu.temp3 & 1;
            uint8_t result = cpu.temp3 >> 1;
            memory_write(cpu.temp2, result);
            cpu_update_nz_flags(result);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x4E: // LSR_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( cpu.temp2<<8 | cpu.temp1 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            memory_write(cpu.temp2<<8 | cpu.temp1, cpu.temp3);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            cpu.flag_c = cpu.temp3 & 1;
            uint8_t result = cpu.temp3 >> 1;
            memory_write(cpu.temp2<<8 | cpu.temp1, result);
            cpu_update_nz_flags(result);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x5E: // LSR_absolute_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.x) % 256);
            memory_read(temp_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            cpu.temp3 = memory_read(real_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            memory_write(real_addr, cpu.temp3);
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            cpu.flag_c = cpu.temp3 & 1;
            uint8_t result = cpu.temp3 >> 1;
            memory_write(real_addr, result);
            cpu_update_nz_flags(result);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x2A: // ROL
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            int result = (cpu.a << 1) | cpu.flag_c;
            cpu.flag_c = result & 0x100;
            cpu.a = result;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 1;
            cpu.cycle = 1;
        }
        return;
    }
    

    

    case 0x26: // ROL_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            memory_write(cpu.temp1, cpu.temp2);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            int result = (cpu.temp2 << 1) | cpu.flag_c;
            cpu.flag_c = result & 0x100;
            cpu_update_nz_flags(result);
            memory_write(cpu.temp1, result);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return; 
    }

    

    case 0x36: // ROL_zeropage_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = cpu.temp1 + cpu.x;
            cpu.temp3 = memory_read(cpu.temp2);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            memory_write(cpu.temp2, cpu.temp3);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            int result = (cpu.temp3 << 1) | cpu.flag_c;
            cpu.flag_c = result & 0x100;
            memory_write(cpu.temp2, result);
            cpu_update_nz_flags(result);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x2E: // ROL_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( cpu.temp2<<8 | cpu.temp1 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            memory_write(cpu.temp2<<8 | cpu.temp1, cpu.temp3);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            int result = (cpu.temp3 << 1) | cpu.flag_c;
            cpu.flag_c = result & 0x100;
            memory_write(cpu.temp2<<8 | cpu.temp1, result);
            cpu_update_nz_flags(result);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x3E: // ROL_absolute_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.x) % 256);
            memory_read(temp_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            cpu.temp3 = memory_read(real_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            memory_write(real_addr, cpu.temp3);
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            int result = (cpu.temp3 << 1) | cpu.flag_c;
            cpu.flag_c = result & 0x100;
            memory_write(real_addr, result);
            cpu_update_nz_flags(result);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x6A: // ROR
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            bool old_flag_c = cpu.flag_c;
            cpu.flag_c = cpu.a & 1;
            int result = (cpu.a >> 1) | (old_flag_c << 7);
            cpu.a = result;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 1;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x66: // ROR_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            memory_write(cpu.temp1, cpu.temp2);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            bool old_flag_c = cpu.flag_c;
            cpu.flag_c = cpu.temp2 & 1;
            int result = (cpu.temp2 >> 1) | (old_flag_c << 7);
            cpu_update_nz_flags(result);
            memory_write(cpu.temp1, result);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x76: // ROR_zeropage_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = cpu.temp1 + cpu.x;
            cpu.temp3 = memory_read(cpu.temp2);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            memory_write(cpu.temp2, cpu.temp3);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            bool old_flag_c = cpu.flag_c;
            cpu.flag_c = cpu.temp3 & 1;
            int result = (cpu.temp3 >> 1) | (old_flag_c << 7);
            memory_write(cpu.temp2, result);
            cpu_update_nz_flags(result);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x6E: // ROR_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( cpu.temp2<<8 | cpu.temp1 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            memory_write(cpu.temp2<<8 | cpu.temp1, cpu.temp3);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            bool old_flag_c = cpu.flag_c;
            cpu.flag_c = cpu.temp3 & 1;
            int result = (cpu.temp3 >> 1) | (old_flag_c << 7);
            memory_write(cpu.temp2<<8 | cpu.temp1, result);
            cpu_update_nz_flags(result);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x7E: // ROR_absolute_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.x) % 256);
            memory_read(temp_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            cpu.temp3 = memory_read(real_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            memory_write(real_addr, cpu.temp3);
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            bool old_flag_c = cpu.flag_c;
            cpu.flag_c = cpu.temp3 & 1;
            int result = (cpu.temp3 >> 1) | (old_flag_c << 7);
            memory_write(real_addr, result);
            cpu_update_nz_flags(result);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x4C: // JMP
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.pc = cpu.temp2<<8 | cpu.temp1;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x6C: // JMP_indirect
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( cpu.temp2<<8 | cpu.temp1 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            // glitch: JMP [03FFh] would fetch the MSB from [0300h] instead of [0400h]
            uint8_t temp = cpu.temp1 + 1;
            uint8_t highbyte = memory_read( cpu.temp2<<8 | temp );
            cpu.pc = highbyte<<8 | cpu.temp3;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x20: // JSR_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read( cpu.s + 0x100 );
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu_push_to_stack((cpu.pc+2)>>8);
            // edge case: refetch high byte, but keep the low one
            // https://www.nesdev.org/6502_cpu.txt
            // https://github.com/SingleStepTests/ProcessorTests/issues/65
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            cpu_push_to_stack(cpu.pc+2);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.pc = cpu.temp2<<8 | cpu.temp1;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x40: // RTI
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.s + 0x100);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint8_t flags = cpu_pop_from_stack();
            cpu_set_flags_from_integer(flags);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            cpu.temp1 = cpu_pop_from_stack();
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint8_t high = cpu_pop_from_stack();
            cpu.pc = high<<8 | cpu.temp1;
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0x60: // RTS
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.s + 0x100);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp1 = cpu_pop_from_stack();
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            cpu.temp2 = cpu_pop_from_stack();
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint16_t addr = cpu.temp2<<8 | cpu.temp1;
            memory_read( addr);
            cpu.pc = addr + 1;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x10: // BPL
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
            cpu.pc += 2;
            if (cpu.flag_n) {
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.pc);
            int delta = cpu.temp1<=127 ? cpu.temp1 : cpu.temp1-256;
            uint8_t page_before = cpu.temp2 = cpu.pc / 256;
            cpu.pc += delta;
            uint8_t page_after = cpu.pc / 256;
            cpu.cycle++;
            if (page_before == page_after) {
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==4) {
            uint8_t offset_after = cpu.pc & 0xFF;
            memory_read( cpu.temp2<<8 | offset_after);
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0x30: // BMI
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
            cpu.pc += 2;
            if (!cpu.flag_n) {
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.pc);
            int delta = cpu.temp1<=127 ? cpu.temp1 : cpu.temp1-256;
            uint8_t page_before = cpu.temp2 = cpu.pc / 256;
            cpu.pc += delta;
            uint8_t page_after = cpu.pc / 256;
            cpu.cycle++;
            if (page_before == page_after) {
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==4) {
            uint8_t offset_after = cpu.pc & 0xFF;
            memory_read( cpu.temp2<<8 | offset_after);
            cpu.cycle = 1;
        }
        return;
    }
    

    case 0x50: // BVC
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
            cpu.pc += 2;
            if (cpu.flag_v) {
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.pc);
            int delta = cpu.temp1<=127 ? cpu.temp1 : cpu.temp1-256;
            uint8_t page_before = cpu.temp2 = cpu.pc / 256;
            cpu.pc += delta;
            uint8_t page_after = cpu.pc / 256;
            cpu.cycle++;
            if (page_before == page_after) {
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==4) {
            uint8_t offset_after = cpu.pc & 0xFF;
            memory_read( cpu.temp2<<8 | offset_after);
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x70: // BVS
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
            cpu.pc += 2;
            if (!cpu.flag_v) {
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.pc);
            int delta = cpu.temp1<=127 ? cpu.temp1 : cpu.temp1-256;
            uint8_t page_before = cpu.temp2 = cpu.pc / 256;
            cpu.pc += delta;
            uint8_t page_after = cpu.pc / 256;
            cpu.cycle++;
            if (page_before == page_after) {
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==4) {
            uint8_t offset_after = cpu.pc & 0xFF;
            memory_read( cpu.temp2<<8 | offset_after);
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x90: // BCC, BLT
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
            cpu.pc += 2;
            if (cpu.flag_c) {
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.pc);
            int delta = cpu.temp1<=127 ? cpu.temp1 : cpu.temp1-256;
            uint8_t page_before = cpu.temp2 = cpu.pc / 256;
            cpu.pc += delta;
            uint8_t page_after = cpu.pc / 256;
            cpu.cycle++;
            if (page_before == page_after) {
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==4) {
            uint8_t offset_after = cpu.pc & 0xFF;
            memory_read( cpu.temp2<<8 | offset_after);
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xB0: // BCS, BGE
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
            cpu.pc += 2;
            if (!cpu.flag_c) {
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.pc);
            int delta = cpu.temp1<=127 ? cpu.temp1 : cpu.temp1-256;
            uint8_t page_before = cpu.temp2 = cpu.pc / 256;
            cpu.pc += delta;
            uint8_t page_after = cpu.pc / 256;
            cpu.cycle++;
            if (page_before == page_after) {
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==4) {
            uint8_t offset_after = cpu.pc & 0xFF;
            memory_read( cpu.temp2<<8 | offset_after);
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xD0: // BNE, BZC
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
            cpu.pc += 2;
            if (cpu.flag_z) {
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.pc);
            int delta = cpu.temp1<=127 ? cpu.temp1 : cpu.temp1-256;
            uint8_t page_before = cpu.temp2 = cpu.pc / 256;
            cpu.pc += delta;
            uint8_t page_after = cpu.pc / 256;
            cpu.cycle++;
            if (page_before == page_after) {
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==4) {
            uint8_t offset_after = cpu.pc & 0xFF;
            memory_read( cpu.temp2<<8 | offset_after);
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xF0: // BEQ, BZS
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
            cpu.pc += 2;
            if (!cpu.flag_z) {
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.pc);
            int delta = cpu.temp1<=127 ? cpu.temp1 : cpu.temp1-256;
            uint8_t page_before = cpu.temp2 = cpu.pc / 256;
            cpu.pc += delta;
            uint8_t page_after = cpu.pc / 256;
            cpu.cycle++;
            if (page_before == page_after) {
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==4) {
            uint8_t offset_after = cpu.pc & 0xFF;
            memory_read( cpu.temp2<<8 | offset_after);
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x00: // BRK
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            uint16_t saved_pc = cpu.pc + 2;
            cpu_push_to_stack(saved_pc>>8); // high byte
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t saved_pc = cpu.pc + 2;
            cpu_push_to_stack(saved_pc); // low byte
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint8_t flags = cpu_get_flags_as_integer();
            cpu_push_to_stack(flags);
            cpu.flag_i = 1;
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            cpu.temp1 = memory_read(0xFFFE);
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            uint8_t high = memory_read(0xFFFF);
            cpu.pc = high<<8 | cpu.temp1;
            cpu.cycle = 1;
        }
    }

    

    case 0x18: // CLC
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.flag_c = 0;
            cpu.pc += 1;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x58: // CLI
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.flag_i = 0;
            cpu.pc += 1;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xD8: // CLD
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.flag_d = 0;
            cpu.pc += 1;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xB8: // CLV
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.flag_v = 0;
            cpu.pc += 1;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x38: // SEC
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.flag_c = 1;
            cpu.pc += 1;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0x78: // SEI
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.flag_i = 1;
            cpu.pc += 1;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xF8: // SED
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.flag_d = 1;
            cpu.pc += 1;
            cpu.cycle = 1;
        }
        return;
    }

    

    case 0xEA: // NOP
    case 0x1A:
    case 0x3A:
    case 0x5A:
    case 0x7A:
    case 0xDA:
    case 0xFA:
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.pc += 1;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x80: // NOP_immediate
    case 0x82:
    case 0x89:
    case 0xC2:
    case 0xE2:
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x04: // NOP_zeropage
    case 0x44:
    case 0x64:
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x14: // NOP_zeropage_x
    case 0x34:
    case 0x54:
    case 0x74:
    case 0xD4:
    case 0xF4:
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            memory_read( (cpu.temp1 + cpu.x) % 256);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x0C: // NOP_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            memory_read( cpu.temp2<<8 | cpu.temp1 );
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x1C: // NOP_absolute_x
    case 0x3C:
    case 0x5C:
    case 0x7C:
    case 0xDC:
    case 0xFC:
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.x) % 256);
            memory_read(temp_addr);
            cpu.cycle++;
            if (temp_addr == real_addr) {
                cpu.pc += 3;
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            memory_read(real_addr);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x02: // KIL
    case 0x12:
    case 0x22:
    case 0x32:
    case 0x42:
    case 0x52:
    case 0x62:
    case 0x72:
    case 0x92:
    case 0xB2:
    case 0xD2:
    case 0xF2:
    {
        if (cpu.cycle==2) {
            memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.pc + 1);
            cpu.is_killed = true;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x87: // SAX_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_write(cpu.temp1, cpu.a & cpu.x);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x97: // SAX_zeropage_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            memory_write( (cpu.temp1 + cpu.y) % 256, cpu.a & cpu.x);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x8F: // SAX_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            memory_write( cpu.temp2<<8 | cpu.temp1, cpu.a & cpu.x );
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x83: // SAX_indirectx
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = memory_read( (cpu.temp1 + cpu.x) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            cpu.temp3 = memory_read( (cpu.temp1 + cpu.x + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            memory_write( cpu.temp3<<8 | cpu.temp2, cpu.a & cpu.x );
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0xA7: // LAX_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.x = cpu.a = memory_read(cpu.temp1);
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0xB7: // LAX_zeropage_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.x = cpu.a = memory_read( (cpu.temp1 + cpu.y) % 256 );
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0xAF: // LAX_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.x = cpu.a = memory_read( cpu.temp2<<8 | cpu.temp1 );
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }



    case 0xA3: // LAX_indirect_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = memory_read( (cpu.temp1 + cpu.x) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            cpu.temp3 = memory_read( (cpu.temp1 + cpu.x + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            cpu.x = cpu.a = memory_read( cpu.temp3<<8 | cpu.temp2 );
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0xB3: // LAX_indirect_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( (cpu.temp1 + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            uint16_t temp_addr = cpu.temp3<<8 | ((cpu.temp2 + cpu.y) % 256);
            cpu.x = cpu.a = memory_read(temp_addr);
            cpu.cycle++;
            if (temp_addr == real_addr) {
                cpu_update_nz_flags(cpu.a);
                cpu.pc += 2;
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            cpu.x = cpu.a = memory_read(real_addr);
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x07: // SLO_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            memory_write(cpu.temp1, cpu.temp2);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            cpu.flag_c = cpu.temp2 & 0x80;
            uint8_t result = cpu.temp2 << 1;
            memory_write(cpu.temp1, result);
            cpu.a |= result;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x17: // SLO_zeropage_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = memory_read( (cpu.temp1 + cpu.x) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            memory_write( (cpu.temp1 + cpu.x) % 256 , cpu.temp2 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            cpu.flag_c = cpu.temp2 & 0x80;
            uint8_t result = cpu.temp2 << 1;
            memory_write( (cpu.temp1 + cpu.x) % 256 , result );
            cpu.a |= result;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x03: // SLO_indirect_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = memory_read( (cpu.temp1 + cpu.x) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            cpu.temp3 = memory_read( (cpu.temp1 + cpu.x + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            cpu.temp4 = memory_read( cpu.temp3<<8 | cpu.temp2 );
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            memory_write(cpu.temp3<<8 | cpu.temp2, cpu.temp4);
            cpu.cycle++;
        }
        else if (cpu.cycle==8) {
            cpu.flag_c = cpu.temp4 & 0x80;
            uint8_t result = cpu.temp4 << 1;
            memory_write(cpu.temp3<<8 | cpu.temp2, result);
            cpu.a |= result;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x13: // SLO_indirect_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( (cpu.temp1 + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t temp_addr = cpu.temp3<<8 | ((cpu.temp2 + cpu.y) % 256);
            memory_read(temp_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            cpu.temp4 = memory_read(real_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            memory_write(real_addr, cpu.temp4);
            cpu.cycle++;
        }
        else if (cpu.cycle==8) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            cpu.flag_c = cpu.temp4 & 0x80;
            uint8_t result = cpu.temp4 << 1;
            memory_write(real_addr, result);
            cpu.a |= result;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x0F: // SLO_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( cpu.temp2<<8 | cpu.temp1 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            memory_write( cpu.temp2<<8 | cpu.temp1, cpu.temp3 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            cpu.flag_c = cpu.temp3 & 0x80;
            uint8_t result = cpu.temp3 << 1;
            memory_write( cpu.temp2<<8 | cpu.temp1, result );
            cpu.a |= result;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x1F: // SLO_absolute_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.x) % 256);
            memory_read(temp_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            cpu.temp3 = memory_read(real_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            memory_write(real_addr, cpu.temp3);
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            cpu.flag_c = cpu.temp3 & 0x80;
            uint8_t result = cpu.temp3 << 1;
            memory_write(real_addr, result);
            cpu.a |= result;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x1B: // SLO_absolute_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.y) % 256);
            memory_read(temp_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            cpu.temp3 = memory_read(real_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            memory_write(real_addr, cpu.temp3);
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            cpu.flag_c = cpu.temp3 & 0x80;
            uint8_t result = cpu.temp3 << 1;
            memory_write(real_addr, result);
            cpu.a |= result;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }

    
    
    case 0x27: // RLA_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            memory_write(cpu.temp1, cpu.temp2);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            int result = (cpu.temp2 << 1) | cpu.flag_c;
            cpu.flag_c = result & 0x100;
            memory_write(cpu.temp1, result);
            cpu.a &= result;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x37: // RLA_zeropage_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = memory_read( (cpu.temp1 + cpu.x) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            memory_write( (cpu.temp1 + cpu.x) % 256 , cpu.temp2 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            int result = (cpu.temp2 << 1) | cpu.flag_c;
            cpu.flag_c = result & 0x100;
            memory_write( (cpu.temp1 + cpu.x) % 256 , result );
            cpu.a &= result;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x23: // RLA_indirect_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = memory_read( (cpu.temp1 + cpu.x) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            cpu.temp3 = memory_read( (cpu.temp1 + cpu.x + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            cpu.temp4 = memory_read( cpu.temp3<<8 | cpu.temp2 );
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            memory_write(cpu.temp3<<8 | cpu.temp2, cpu.temp4);
            cpu.cycle++;
        }
        else if (cpu.cycle==8) {
            int result = (cpu.temp4 << 1) | cpu.flag_c;
            cpu.flag_c = result & 0x100;
            memory_write(cpu.temp3<<8 | cpu.temp2, result);
            cpu.a &= result;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x33: // RLA_indirect_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( (cpu.temp1 + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t temp_addr = cpu.temp3<<8 | ((cpu.temp2 + cpu.y) % 256);
            memory_read(temp_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            cpu.temp4 = memory_read(real_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            memory_write(real_addr, cpu.temp4);
            cpu.cycle++;
        }
        else if (cpu.cycle==8) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            int result = (cpu.temp4 << 1) | cpu.flag_c;
            cpu.flag_c = result & 0x100;
            memory_write(real_addr, result);
            cpu.a &= result;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x2F: // RLA_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( cpu.temp2<<8 | cpu.temp1 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            memory_write( cpu.temp2<<8 | cpu.temp1, cpu.temp3 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            int result = (cpu.temp3 << 1) | cpu.flag_c;
            cpu.flag_c = result & 0x100;
            memory_write( cpu.temp2<<8 | cpu.temp1, result );
            cpu.a &= result;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x3F: // RLA_absolute_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.x) % 256);
            memory_read(temp_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            cpu.temp3 = memory_read(real_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            memory_write(real_addr, cpu.temp3);
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            int result = (cpu.temp3 << 1) | cpu.flag_c;
            cpu.flag_c = result & 0x100;
            memory_write(real_addr, result);
            cpu.a &= result;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x3B: // RLA_absolute_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.y) % 256);
            memory_read(temp_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            cpu.temp3 = memory_read(real_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            memory_write(real_addr, cpu.temp3);
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            int result = (cpu.temp3 << 1) | cpu.flag_c;
            cpu.flag_c = result & 0x100;
            memory_write(real_addr, result);
            cpu.a &= result;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }




    
    
    case 0x47: // SRE_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            memory_write(cpu.temp1, cpu.temp2);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            cpu.flag_c = cpu.temp2 & 1;
            uint8_t result = cpu.temp2 >> 1;
            memory_write(cpu.temp1, result);
            cpu.a ^= result;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x57: // SRE_zeropage_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = memory_read( (cpu.temp1 + cpu.x) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            memory_write( (cpu.temp1 + cpu.x) % 256 , cpu.temp2 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            cpu.flag_c = cpu.temp2 & 1;
            uint8_t result = cpu.temp2 >> 1;
            memory_write( (cpu.temp1 + cpu.x) % 256 , result );
            cpu.a ^= result;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x43: // SRE_indirect_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = memory_read( (cpu.temp1 + cpu.x) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            cpu.temp3 = memory_read( (cpu.temp1 + cpu.x + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            cpu.temp4 = memory_read( cpu.temp3<<8 | cpu.temp2 );
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            memory_write(cpu.temp3<<8 | cpu.temp2, cpu.temp4);
            cpu.cycle++;
        }
        else if (cpu.cycle==8) {
            cpu.flag_c = cpu.temp4 & 1;
            uint8_t result = cpu.temp4 >> 1;
            memory_write(cpu.temp3<<8 | cpu.temp2, result);
            cpu.a ^= result;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x53: // SRE_indirect_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( (cpu.temp1 + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t temp_addr = cpu.temp3<<8 | ((cpu.temp2 + cpu.y) % 256);
            memory_read(temp_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            cpu.temp4 = memory_read(real_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            memory_write(real_addr, cpu.temp4);
            cpu.cycle++;
        }
        else if (cpu.cycle==8) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            cpu.flag_c = cpu.temp4 & 1;
            uint8_t result = cpu.temp4 >> 1;
            memory_write(real_addr, result);
            cpu.a ^= result;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x4F: // SRE_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( cpu.temp2<<8 | cpu.temp1 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            memory_write( cpu.temp2<<8 | cpu.temp1, cpu.temp3 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            cpu.flag_c = cpu.temp3 & 1;
            uint8_t result = cpu.temp3 >> 1;
            memory_write( cpu.temp2<<8 | cpu.temp1, result );
            cpu.a ^= result;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x5F: // SRE_absolute_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.x) % 256);
            memory_read(temp_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            cpu.temp3 = memory_read(real_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            memory_write(real_addr, cpu.temp3);
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            cpu.flag_c = cpu.temp3 & 1;
            uint8_t result = cpu.temp3 >> 1;
            memory_write(real_addr, result);
            cpu.a ^= result;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x5B: // SRE_absolute_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.y) % 256);
            memory_read(temp_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            cpu.temp3 = memory_read(real_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            memory_write(real_addr, cpu.temp3);
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            cpu.flag_c = cpu.temp3 & 1;
            uint8_t result = cpu.temp3 >> 1;
            memory_write(real_addr, result);
            cpu.a ^= result;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }




    
    
    case 0x67: // RRA_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            memory_write(cpu.temp1, cpu.temp2);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            bool old_flag_c = cpu.flag_c;
            cpu.flag_c = cpu.temp2 & 1;
            uint8_t rotated = (cpu.temp2 >> 1) | (old_flag_c << 7);
            memory_write(cpu.temp1, rotated);
            if (cpu.flag_d) {
                cpu_add_in_bcd_mode(rotated);
            }
            else {
                int sum = cpu.a + cpu.flag_c + rotated;
                cpu_update_cv_flags(cpu.a, rotated, cpu.flag_c);
                cpu.a = sum;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x77: // RRA_zeropage_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = memory_read( (cpu.temp1 + cpu.x) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            memory_write( (cpu.temp1 + cpu.x) % 256 , cpu.temp2 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            bool old_flag_c = cpu.flag_c;
            cpu.flag_c = cpu.temp2 & 1;
            uint8_t rotated = (cpu.temp2 >> 1) | (old_flag_c << 7);
            memory_write( (cpu.temp1 + cpu.x) % 256 , rotated );
            if (cpu.flag_d) {
                cpu_add_in_bcd_mode(rotated);
            }
            else {
                int sum = cpu.a + cpu.flag_c + rotated;
                cpu_update_cv_flags(cpu.a, rotated, cpu.flag_c);
                cpu.a = sum;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x63: // RRA_indirect_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = memory_read( (cpu.temp1 + cpu.x) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            cpu.temp3 = memory_read( (cpu.temp1 + cpu.x + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            cpu.temp4 = memory_read( cpu.temp3<<8 | cpu.temp2 );
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            memory_write(cpu.temp3<<8 | cpu.temp2, cpu.temp4);
            cpu.cycle++;
        }
        else if (cpu.cycle==8) {
            bool old_flag_c = cpu.flag_c;
            cpu.flag_c = cpu.temp4 & 1;
            uint8_t rotated = (cpu.temp4 >> 1) | (old_flag_c << 7);
            memory_write(cpu.temp3<<8 | cpu.temp2, rotated);
            if (cpu.flag_d) {
                cpu_add_in_bcd_mode(rotated);
            }
            else {
                int sum = cpu.a + cpu.flag_c + rotated;
                cpu_update_cv_flags(cpu.a, rotated, cpu.flag_c);
                cpu.a = sum;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x73: // RRA_indirect_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( (cpu.temp1 + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t temp_addr = cpu.temp3<<8 | ((cpu.temp2 + cpu.y) % 256);
            memory_read(temp_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            cpu.temp4 = memory_read(real_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            memory_write(real_addr, cpu.temp4);
            cpu.cycle++;
        }
        else if (cpu.cycle==8) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            bool old_flag_c = cpu.flag_c;
            cpu.flag_c = cpu.temp4 & 1;
            uint8_t rotated = (cpu.temp4 >> 1) | (old_flag_c << 7);
            memory_write(real_addr, rotated);
            if (cpu.flag_d) {
                cpu_add_in_bcd_mode(rotated);
            }
            else {
                int sum = cpu.a + cpu.flag_c + rotated;
                cpu_update_cv_flags(cpu.a, rotated, cpu.flag_c);
                cpu.a = sum;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x6F: // RRA_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( cpu.temp2<<8 | cpu.temp1 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            memory_write( cpu.temp2<<8 | cpu.temp1, cpu.temp3 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            bool old_flag_c = cpu.flag_c;
            cpu.flag_c = cpu.temp3 & 1;
            uint8_t rotated = (cpu.temp3 >> 1) | (old_flag_c << 7);
            memory_write( cpu.temp2<<8 | cpu.temp1, rotated );
            if (cpu.flag_d) {
                cpu_add_in_bcd_mode(rotated);
            }
            else {
                int sum = cpu.a + cpu.flag_c + rotated;
                cpu_update_cv_flags(cpu.a, rotated, cpu.flag_c);
                cpu.a = sum;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x7F: // RRA_absolute_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.x) % 256);
            memory_read(temp_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            cpu.temp3 = memory_read(real_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            memory_write(real_addr, cpu.temp3);
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            bool old_flag_c = cpu.flag_c;
            cpu.flag_c = cpu.temp3 & 1;
            uint8_t rotated = (cpu.temp3 >> 1) | (old_flag_c << 7);
            memory_write(real_addr, rotated);
            if (cpu.flag_d) {
                cpu_add_in_bcd_mode(rotated);
            }
            else {
                int sum = cpu.a + cpu.flag_c + rotated;
                cpu_update_cv_flags(cpu.a, rotated, cpu.flag_c);
                cpu.a = sum;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x7B: // RRA_absolute_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.y) % 256);
            memory_read(temp_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            cpu.temp3 = memory_read(real_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            memory_write(real_addr, cpu.temp3);
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            bool old_flag_c = cpu.flag_c;
            cpu.flag_c = cpu.temp3 & 1;
            uint8_t rotated = (cpu.temp3 >> 1) | (old_flag_c << 7);
            memory_write(real_addr, rotated);
            if (cpu.flag_d) {
                cpu_add_in_bcd_mode(rotated);
            }
            else {
                int sum = cpu.a + cpu.flag_c + rotated;
                cpu_update_cv_flags(cpu.a, rotated, cpu.flag_c);
                cpu.a = sum;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }




    
    
    case 0xC7: // DCP_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            memory_write(cpu.temp1, cpu.temp2);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint8_t result = cpu.temp2-1;
            memory_write(cpu.temp1, result);
            int difference = cpu.a - result;
            cpu.flag_c = difference>=0;
            cpu_update_nz_flags(difference);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0xD7: // DCP_zeropage_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = memory_read( (cpu.temp1 + cpu.x) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            memory_write( (cpu.temp1 + cpu.x) % 256 , cpu.temp2 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint8_t result = cpu.temp2 - 1;
            memory_write( (cpu.temp1 + cpu.x) % 256 , result );
            int difference = cpu.a - result;
            cpu.flag_c = difference>=0;
            cpu_update_nz_flags(difference);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0xC3: // DCP_indirect_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = memory_read( (cpu.temp1 + cpu.x) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            cpu.temp3 = memory_read( (cpu.temp1 + cpu.x + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            cpu.temp4 = memory_read( cpu.temp3<<8 | cpu.temp2 );
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            memory_write(cpu.temp3<<8 | cpu.temp2, cpu.temp4);
            cpu.cycle++;
        }
        else if (cpu.cycle==8) {
            uint8_t result = cpu.temp4 - 1;
            memory_write(cpu.temp3<<8 | cpu.temp2, result);
            int difference = cpu.a - result;
            cpu.flag_c = difference>=0;
            cpu_update_nz_flags(difference);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0xD3: // DCP_indirect_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( (cpu.temp1 + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t temp_addr = cpu.temp3<<8 | ((cpu.temp2 + cpu.y) % 256);
            memory_read(temp_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            cpu.temp4 = memory_read(real_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            memory_write(real_addr, cpu.temp4);
            cpu.cycle++;
        }
        else if (cpu.cycle==8) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            uint8_t result = cpu.temp4 - 1;
            memory_write(real_addr, result);
            int difference = cpu.a - result;
            cpu.flag_c = difference>=0;
            cpu_update_nz_flags(difference);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0xCF: // DCP_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( cpu.temp2<<8 | cpu.temp1 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            memory_write( cpu.temp2<<8 | cpu.temp1, cpu.temp3 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint8_t result = cpu.temp3 - 1;
            memory_write( cpu.temp2<<8 | cpu.temp1, result );
            int difference = cpu.a - result;
            cpu.flag_c = difference>=0;
            cpu_update_nz_flags(difference);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }



    case 0xDF: // DCP_absolute_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.x) % 256);
            memory_read(temp_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            cpu.temp3 = memory_read(real_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            memory_write(real_addr, cpu.temp3);
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            uint8_t result = cpu.temp3 - 1;
            memory_write(real_addr, result);
            int difference = cpu.a - result;
            cpu.flag_c = difference>=0;
            cpu_update_nz_flags(difference);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }



    case 0xDB: // DCP_absolute_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.y) % 256);
            memory_read(temp_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            cpu.temp3 = memory_read(real_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            memory_write(real_addr, cpu.temp3);
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            uint8_t result = cpu.temp3 - 1;
            memory_write(real_addr, result);
            int difference = cpu.a - result;
            cpu.flag_c = difference>=0;
            cpu_update_nz_flags(difference);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }




    
    
    case 0xE7: // ISC_zeropage
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            memory_write(cpu.temp1, cpu.temp2);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint8_t incremented = cpu.temp2 + 1;
            memory_write(cpu.temp1, incremented);
            if (cpu.flag_d) {
                cpu_subtract_in_bcd_mode(incremented);
            }
            else {
                int difference = cpu.a + cpu.flag_c - 1 - incremented;
                cpu_update_cv_flags_subtraction(cpu.a, incremented, cpu.flag_c);
                cpu.a = difference;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0xF7: // ISC_zeropage_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = memory_read( (cpu.temp1 + cpu.x) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            memory_write( (cpu.temp1 + cpu.x) % 256 , cpu.temp2 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint8_t incremented = cpu.temp2 + 1;
            memory_write( (cpu.temp1 + cpu.x) % 256 , incremented );
            if (cpu.flag_d) {
                cpu_subtract_in_bcd_mode(incremented);
            }
            else {
                int difference = cpu.a + cpu.flag_c - 1 - incremented;
                cpu_update_cv_flags_subtraction(cpu.a, incremented, cpu.flag_c);
                cpu.a = difference;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0xE3: // ISC_indirect_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp2 = memory_read( (cpu.temp1 + cpu.x) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            cpu.temp3 = memory_read( (cpu.temp1 + cpu.x + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            cpu.temp4 = memory_read( cpu.temp3<<8 | cpu.temp2 );
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            memory_write(cpu.temp3<<8 | cpu.temp2, cpu.temp4);
            cpu.cycle++;
        }
        else if (cpu.cycle==8) {
            uint8_t incremented = cpu.temp4 + 1;
            memory_write(cpu.temp3<<8 | cpu.temp2, incremented);
            if (cpu.flag_d) {
                cpu_subtract_in_bcd_mode(incremented);
            }
            else {
                int difference = cpu.a + cpu.flag_c - 1 - incremented;
                cpu_update_cv_flags_subtraction(cpu.a, incremented, cpu.flag_c);
                cpu.a = difference;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0xF3: // ISC_indirect_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.temp1);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( (cpu.temp1 + 1) % 256 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t temp_addr = cpu.temp3<<8 | ((cpu.temp2 + cpu.y) % 256);
            memory_read(temp_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            cpu.temp4 = memory_read(real_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            memory_write(real_addr, cpu.temp4);
            cpu.cycle++;
        }
        else if (cpu.cycle==8) {
            uint16_t real_addr = (cpu.temp3<<8 | cpu.temp2) + cpu.y;
            uint8_t incremented = cpu.temp4 + 1;
            memory_write(real_addr, incremented);
            if (cpu.flag_d) {
                cpu_subtract_in_bcd_mode(incremented);
            }
            else {
                int difference = cpu.a + cpu.flag_c - 1 - incremented;
                cpu_update_cv_flags_subtraction(cpu.a, incremented, cpu.flag_c);
                cpu.a = difference;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }



    case 0xEF: // ISC_absolute
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            cpu.temp3 = memory_read( cpu.temp2<<8 | cpu.temp1 );
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            memory_write( cpu.temp2<<8 | cpu.temp1, cpu.temp3 );
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint8_t incremented = cpu.temp3 + 1;
            memory_write( cpu.temp2<<8 | cpu.temp1, incremented );
            if (cpu.flag_d) {
                cpu_subtract_in_bcd_mode(incremented);
            }
            else {
                int difference = cpu.a + cpu.flag_c - 1 - incremented;
                cpu_update_cv_flags_subtraction(cpu.a, incremented, cpu.flag_c);
                cpu.a = difference;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }



    case 0xFF: // ISC_absolute_x
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.x) % 256);
            memory_read(temp_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            cpu.temp3 = memory_read(real_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            memory_write(real_addr, cpu.temp3);
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.x;
            uint8_t incremented = cpu.temp3 + 1;
            memory_write(real_addr, incremented);
            if (cpu.flag_d) {
                cpu_subtract_in_bcd_mode(incremented);
            }
            else {
                int difference = cpu.a + cpu.flag_c - 1 - incremented;
                cpu_update_cv_flags_subtraction(cpu.a, incremented, cpu.flag_c);
                cpu.a = difference;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }



    case 0xFB: // ISC_absolute_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.y) % 256);
            memory_read(temp_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            cpu.temp3 = memory_read(real_addr);
            cpu.cycle++;
        }
        else if (cpu.cycle==6) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            memory_write(real_addr, cpu.temp3);
            cpu.cycle++;
        }
        else if (cpu.cycle==7) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            uint8_t incremented = cpu.temp3 + 1;
            memory_write(real_addr, incremented);
            if (cpu.flag_d) {
                cpu_subtract_in_bcd_mode(incremented);
            }
            else {
                int difference = cpu.a + cpu.flag_c - 1 - incremented;
                cpu_update_cv_flags_subtraction(cpu.a, incremented, cpu.flag_c);
                cpu.a = difference;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }



    case 0x0B: // ANC_immediate
    case 0x2B:
    {
        if (cpu.cycle==2) {
            cpu.a &= memory_read(cpu.pc + 1);
            cpu_update_nz_flags(cpu.a);
            cpu.flag_c = cpu.flag_n;
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }


    case 0x4B: // ALR_immediate
    {
        if (cpu.cycle==2) {
            cpu.a = cpu.a & memory_read(cpu.pc + 1);
            cpu.flag_c = cpu.a & 1;
            cpu.a >>= 1;
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }


    // case 0x6B: // ARR_immediate
    // {
    //     cpu.a &= nn;
    //     bool oldC = cpu.flag_c;
    //     cpu_update_cv_flags(cpu.a, cpu.a, cpu.flag_c);
    //     cpu.a = (cpu.a >> 1) | (oldC & 0x80);
    //     cpu.flag_c = cpu.a & 0x40;
    //     cpu_update_nz_flags(cpu.a);
    //     cpu.pc += 2;
    //     return 2;
    // }

    case 0xBB: // LAS_absolute_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.y) % 256);
            uint8_t value = memory_read(temp_addr);
            cpu.cycle++;
            if (temp_addr == real_addr) {
                cpu.a = value & cpu.s;
                cpu_update_nz_flags(cpu.a);
                cpu.x = value & cpu.s;
                cpu.s = value & cpu.s;
                cpu.pc += 3;
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            uint8_t value = memory_read(real_addr);
            cpu.a = value & cpu.s;
            cpu_update_nz_flags(cpu.a);
            cpu.x = value & cpu.s;
            cpu.s = value & cpu.s;
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }


    // case 0x9C: // SHY_absolute_x
    // {
    //     uint8_t value = cpu.y & cpu.a & (nnnn >> 8) ;
    //     memory_write_in_absolutex_mode(nnnn, cpu.x, value);
    //     cpu.pc += 3;
    //     return 5;
    // }



    case 0xCB: // AXS_immediate
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            int difference = (cpu.a & cpu.x) - cpu.temp1;
            cpu.flag_c = difference>=0;
            cpu_update_nz_flags(difference);
            cpu.x = difference;
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    case 0xEB: // SBC_NOP_immediate
    {
        if (cpu.cycle==2) {
            uint8_t arg2 = memory_read(cpu.pc + 1);
            if (cpu.flag_d) {
                cpu_subtract_in_bcd_mode(arg2);
            }
            else {
                int temp = cpu.a + cpu.flag_c - 1 - arg2;
                cpu_update_cv_flags_subtraction(cpu.a, arg2, cpu.flag_c);
                cpu.a = temp;
                cpu_update_nz_flags(cpu.a);
            }
            cpu.pc += 2;
            cpu.cycle = 1;
        }
        return;
    }

    case 0xBF: // LAX_absolute_y
    {
        if (cpu.cycle==2) {
            cpu.temp1 = memory_read(cpu.pc + 1);
            cpu.cycle++;
        }
        else if (cpu.cycle==3) {
            cpu.temp2 = memory_read(cpu.pc + 2);
            cpu.cycle++;
        }
        else if (cpu.cycle==4) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            uint16_t temp_addr = cpu.temp2<<8 | ((cpu.temp1 + cpu.y) % 256);
            cpu.x = cpu.a = memory_read(temp_addr);
            cpu.cycle++;
            if (temp_addr == real_addr) {
                cpu_update_nz_flags(cpu.a);
                cpu.pc += 3;
                cpu.cycle = 1;
            }
        }
        else if (cpu.cycle==5) {
            uint16_t real_addr = (cpu.temp2<<8 | cpu.temp1) + cpu.y;
            cpu.x = cpu.a = memory_read(real_addr);
            cpu_update_nz_flags(cpu.a);
            cpu.pc += 3;
            cpu.cycle = 1;
        }
        return;
    }


    

    default:
        fprintf(stderr, "Unknown opcode %2hhx at pc=%4hx! \n", cpu.opcode, cpu.pc);
        exit(1);
    };

}