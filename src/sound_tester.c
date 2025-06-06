

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int volume; // 0..15
    int frequency; // 0..31
    int mode; // 0..15

    int poly4;
    int poly5;
    int poly9;
    unsigned int counter;
} AudioChannel;

AudioChannel left = { .poly4=0xF, .poly5=0x1F, .poly9=0x1FF };
AudioChannel right = { .poly4=0xF, .poly5=0x1F, .poly9=0x1FF };




// takes a state, applies poly4, returns new state
int poly4( int state ) {
    bool c = (state >> 1) & 1;
    bool d = state & 1;
    int abc = (state >> 1) & 7;
    bool z = c ^ d;
    return (z<<3) | abc;
}

// takes a state, applies poly5, returns new state
int poly5( int state ) {
    bool e = state & 1;
    bool c = (state >> 2) & 1;
    int abcd = (state >> 1) & 0xF;
    bool z = c ^ e;
    return (z<<4) | abcd;
}

// takes a state, applies poly9, returns new state
int poly9( int state ) {
    bool i = state & 1;
    bool e = (state >> 4) & 1;
    int abcdefgh = (state >> 1) & 0xFF;
    bool z = e ^ i;
    return (z<<8) | abcdefgh;
}



bool audio_next_bit ( AudioChannel *ch ) {

    // mode=0  set to 1
    if ( ch->mode == 0 ) {
        return 1;
    }

    // mode=1  4 bit poly
    else if ( ch->mode == 1 ) {
        bool out = ch->poly4 & 1;
        ch->poly4 = poly4( ch->poly4 );
        return out;
    }

    // mode=2  div 15 -> 4 bit poly
    else if ( ch->mode == 2 ) {
        ch->counter++;
        if (ch->counter % 31 == 0 || ch->counter % 31 == 18) {
            ch->poly4 = poly4( ch->poly4 );
        }
        return ch->poly4 & 1;
    }

    // mode=3  5 bit poly -> 4 bit poly
    else if ( ch->mode == 3 ) {
        bool out = ch->poly4 & 1;
        ch->poly5 = poly5( ch->poly5 );
        if (ch->poly5 & 1) {
            ch->poly4 = poly4( ch->poly4 );
        }
        return out;
    }

    // mode=4  div 2 : pure tone
    else if ( ch->mode == 4 ) {
        ch->counter++;
        return ch->counter % 2;
    }

    // mode=5  div 2 : pure tone
    else if ( ch->mode == 5 ) {
        ch->counter++;
        return ch->counter % 2;
    }

    // mode=6  div 31 : pure tone
    else if ( ch->mode == 6 ) {
        ch->counter++;
        return ch->counter % 31 < 18;
    }

    // mode=7  5 bit poly -> div 2
    else if ( ch->mode == 7 ) {
        // same as mode 9?
        bool out = ch->poly5 & 1;
        ch->poly5 = poly5( ch->poly5 );
        return out;
    }

    // mode=8  9 bit poly (white noise)
    else if ( ch->mode == 8 ) {
        bool out = ch->poly9 & 1;
        ch->poly9 = poly9( ch->poly9 );
        return out;
    }

    // mode=9  5 bit poly
    else if ( ch->mode == 9 ) {
        bool out = ch->poly5 & 1;
        ch->poly5 = poly5( ch->poly5 );
        return out;
    }

    // mode=A  div 31 : pure tone
    else if ( ch->mode == 0xA ) {
        ch->counter++;
        return ch->counter % 31 < 18;
    }

    // mode=B  set last 4 bits to 1
    else if ( ch->mode == 0xB ) {
        return 1;
    }

    // mode=C  div 6 : pure tone
    else if ( ch->mode == 0xC ) {
        ch->counter++;
        return ch->counter % 6 < 3;
    }

    // mode=D  div 6 : pure tone
    else if ( ch->mode == 0xD ) {
        ch->counter++;
        return ch->counter % 6 < 3;
    }

    // mode=E  div 93 : pure tone
    else if ( ch->mode == 0xE ) {
        ch->counter++;
        return ch->counter % 93 < 49;
    }

    // mode=F  5 bit poly div 6
    else if ( ch->mode == 0xF ) {
        bool outbefore = ch->poly5 & 1;
        ch->poly5 = poly5( ch->poly5 );
        bool outafter = ch->poly5 & 1;

        if (outbefore != outafter) {
            ch->counter++;
        }

        return ch->counter % 6 < 3;
    }

    else {
        perror( "AUDC > 15 which it shouldn\'t.");
        exit(1);
    }
    
}

int main(void) {

    AudioChannel a = {.mode = 0,  .poly4=0xF, .poly5=0x1F, .poly9=0x1FF };

    for (int i = 0; i<16; i++) {
        a.mode = i;

        printf("\nMode %2d: ", i);

        for (int j = 0; j<2000; j++) {
            bool out = audio_next_bit( &a );
            printf("%c", out?'-':'_' );
        }
    }
}