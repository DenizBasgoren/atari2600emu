
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct {
    void* fn;
    bool withArg;
    uint8_t optionalArg;
    int delayInColorClocks;
} DWEntry;

#define DWLEN 5
DWEntry dw_list[DWLEN];

typedef void(FnWithoutArg)(void);
typedef void(FnWithArg)(uint8_t);


// returns an empty spot in range 0..4, or -1 if no empty spot found
int find_an_empty_spot(void) {
    int foundSpot = -1;
    for (int i = 0 ; i<DWLEN; i++) {
        if ( dw_list[i].fn == NULL ) {
            foundSpot = i;
            break; // found an empty spot
        }
    }
    return foundSpot;
}

void write_after_delay( void* fn, bool withArg, uint8_t optionalArg, int delayInColorClocks ) {
    int i = find_an_empty_spot();
    if (i == -1) return; // if the list is full, ignore this write.

    dw_list[i].fn = fn;
    dw_list[i].withArg = withArg;
    dw_list[i].optionalArg = optionalArg;
    dw_list[i].delayInColorClocks = delayInColorClocks;
}

void tick_delayed_writes(void) {
    for (int i = 0; i<DWLEN; i++) {
        if ( dw_list[i].fn == NULL ) continue;
        
        if ( dw_list[i].delayInColorClocks-- ) continue;

        if (dw_list[i].withArg) {
            ((FnWithArg*) dw_list[i].fn)(dw_list[i].optionalArg);
        } else {
            ((FnWithoutArg*) dw_list[i].fn)();
        }

        dw_list[i].fn = NULL;
    }
}