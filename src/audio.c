
#include <raylib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "atari2600emulator.h"

/*
    15      AUDC0   ....1111  audio control 0
    16      AUDC1   ....1111  audio control 1
    17      AUDF0   ...11111  audio frequency 0
    18      AUDF1   ...11111  audio frequency 1
    19      AUDV0   ....1111  audio volume 0
    1A      AUDV1   ....1111  audio volume 1
*/


AudioChannel left = { .poly4=0xF, .poly5=0x1F, .poly9=0x1FF };
AudioChannel right = { .poly4=0xF, .poly5=0x1F, .poly9=0x1FF };
AudioStream stream;

void audio_AUDC0_write ( uint8_t value ) {
    left.mode = value & 15;
}

void audio_AUDC1_write ( uint8_t value ) {
    right.mode = value & 15;
}

void audio_AUDF0_write ( uint8_t value ) {
    left.frequency = value & 31;
}

void audio_AUDF1_write ( uint8_t value ) {
    right.frequency = value & 31;
}

void audio_AUDV0_write ( uint8_t value ) {
    left.volume = value & 15;
    if (left.volume>0 || right.volume>0) {
        PlayAudioStream(stream);
    } else {
        StopAudioStream(stream);
    }
}

void audio_AUDV1_write ( uint8_t value ) {
    right.volume = value & 15;
    if (left.volume>0 || right.volume>0) {
        PlayAudioStream(stream);
    } else {
        StopAudioStream(stream);
    }
}

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


bool audio_current_bit( AudioChannel ch ) {
    // mode=0  set to 1
    if ( ch.mode == 0 ) {
        return 1;
    }

    // mode=1  4 bit poly
    else if ( ch.mode == 1 ) {
        return ch.poly4 & 1;
    }

    // mode=2  div 15 -> 4 bit poly
    else if ( ch.mode == 2 ) {
        return ch.poly4 & 1;
    }

    // mode=3  5 bit poly -> 4 bit poly
    else if ( ch.mode == 3 ) {
        return ch.poly4 & 1;
    }

    // mode=4  div 2 : pure tone
    else if ( ch.mode == 4 ) {
        return ch.counter % 2;
    }

    // mode=5  div 2 : pure tone
    else if ( ch.mode == 5 ) {
        return ch.counter % 2;
    }

    // mode=6  div 31 : pure tone
    else if ( ch.mode == 6 ) {
        return ch.counter % 31 < 18;
    }

    // mode=7  5 bit poly -> div 2
    else if ( ch.mode == 7 ) {
        return ch.poly5 & 1;
    }

    // mode=8  9 bit poly (white noise)
    else if ( ch.mode == 8 ) {
        return ch.poly9 & 1;
    }

    // mode=9  5 bit poly
    else if ( ch.mode == 9 ) {
        return ch.poly5 & 1;
    }

    // mode=A  div 31 : pure tone
    else if ( ch.mode == 0xA ) {
        return ch.counter % 31 < 18;
    }

    // mode=B  set last 4 bits to 1
    else if ( ch.mode == 0xB ) {
        return 1;
    }

    // mode=C  div 6 : pure tone
    else if ( ch.mode == 0xC ) {
        return ch.counter % 6 < 3;
    }

    // mode=D  div 6 : pure tone
    else if ( ch.mode == 0xD ) {
        return ch.counter % 6 < 3;
    }

    // mode=E  div 93 : pure tone
    else if ( ch.mode == 0xE ) {
        return ch.counter % 93 < 49;
    }

    // mode=F  5 bit poly div 6
    else if ( ch.mode == 0xF ) {
        return ch.counter % 6 < 3;
    }

    else {
        perror( "AUDC > 15 which it shouldn\'t.");
        exit(1);
    }
}

void audio_next_state ( AudioChannel *ch ) {

    // mode=0  set to 1
    if ( ch->mode == 0 ) {
        // do nothing
    }

    // mode=1  4 bit poly
    else if ( ch->mode == 1 ) {
        ch->poly4 = poly4( ch->poly4 );
    }

    // mode=2  div 15 -> 4 bit poly
    else if ( ch->mode == 2 ) {
        ch->counter++;
        if (ch->counter % 31 == 0 || ch->counter % 31 == 18) {
            ch->poly4 = poly4( ch->poly4 );
        }
    }

    // mode=3  5 bit poly -> 4 bit poly
    else if ( ch->mode == 3 ) {
        ch->poly5 = poly5( ch->poly5 );
        if (ch->poly5 & 1) {
            ch->poly4 = poly4( ch->poly4 );
        }
    }

    // mode=4  div 2 : pure tone
    else if ( ch->mode == 4 ) {
        ch->counter++;
    }

    // mode=5  div 2 : pure tone
    else if ( ch->mode == 5 ) {
        ch->counter++;
    }

    // mode=6  div 31 : pure tone
    else if ( ch->mode == 6 ) {
        ch->counter++;
    }

    // mode=7  5 bit poly -> div 2
    else if ( ch->mode == 7 ) {
        ch->poly5 = poly5( ch->poly5 );
    }

    // mode=8  9 bit poly (white noise)
    else if ( ch->mode == 8 ) {
        ch->poly9 = poly9( ch->poly9 );
    }

    // mode=9  5 bit poly
    else if ( ch->mode == 9 ) {
        ch->poly5 = poly5( ch->poly5 );
    }

    // mode=A  div 31 : pure tone
    else if ( ch->mode == 0xA ) {
        ch->counter++;
    }

    // mode=B  set last 4 bits to 1
    else if ( ch->mode == 0xB ) {
        // do nothing
    }

    // mode=C  div 6 : pure tone
    else if ( ch->mode == 0xC ) {
        ch->counter++;
    }

    // mode=D  div 6 : pure tone
    else if ( ch->mode == 0xD ) {
        ch->counter++;
    }

    // mode=E  div 93 : pure tone
    else if ( ch->mode == 0xE ) {
        ch->counter++;
    }

    // mode=F  5 bit poly div 6
    else if ( ch->mode == 0xF ) {
        bool outbefore = ch->poly5 & 1;
        ch->poly5 = poly5( ch->poly5 );
        bool outafter = ch->poly5 & 1;
        if (outbefore != outafter) {
            ch->counter++;
        }
    }

    else {
        perror( "AUDC > 15 which it shouldn\'t.");
        exit(1);
    }
    
}


int max( int a, int b ) {
    return a>b ? a : b;
}

void audio_next_cb (void *buffer, unsigned int length) {
    static unsigned int audio_counter;
    
    signed char *b = buffer;
    for ( unsigned int i = 0; i<length; i++) {
        b[i] = max(audio_current_bit(left)*left.volume, audio_current_bit(right)*right.volume) * 8;
        
        audio_counter++;
        if ( audio_counter % (left.frequency+1) == 0 ) {
            audio_next_state(&left);
        }
        if ( audio_counter % (right.frequency+1) == 0 ) {
            audio_next_state(&right);
        }
    }
        
    
}

#define AUDIO_LEN 512

void audio_init( void ) {
    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(AUDIO_LEN);
    stream = LoadAudioStream( tv.standard==TV_NTSC ? 31400 : 31113, 8, 1);
    // 31400 = (3.579545 MHz) / (114 color clocks per audio tick)
    // 31113 = (3.546894 MHz) / (114 color clocks per audio tick)
    // 8 bit depth but actually 4 bit, 1 channel
    SetAudioStreamCallback(stream, audio_next_cb );
}

