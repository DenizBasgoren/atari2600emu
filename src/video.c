
#include <stdint.h>
#include <stdbool.h>

#include "atari2600emulator.h"

/*
    00      VSYNC   ......1.  vertical sync set-clear
    01      VBLANK  11....1.  vertical blank set-clear
    02      WSYNC   <strobe>  wait for leading edge of horizontal blank
    03      RSYNC   <strobe>  reset horizontal sync counter
    04      NUSIZ0  ..111111  number-size player-missile 0
    05      NUSIZ1  ..111111  number-size player-missile 1
    06      COLUP0  1111111.  color-lum player 0 and missile 0
    07      COLUP1  1111111.  color-lum player 1 and missile 1
    08      COLUPF  1111111.  color-lum playfield and ball
    09      COLUBK  1111111.  color-lum background
    0A      CTRLPF  ..11.111  control playfield ball size & collisions
    0B      REFP0   ....1...  reflect player 0
    0C      REFP1   ....1...  reflect player 1
    0D      PF0     1111....  playfield register byte 0
    0E      PF1     11111111  playfield register byte 1
    0F      PF2     11111111  playfield register byte 2
    10      RESP0   <strobe>  reset player 0
    11      RESP1   <strobe>  reset player 1
    12      RESM0   <strobe>  reset missile 0
    13      RESM1   <strobe>  reset missile 1
    14      RESBL   <strobe>  reset ball
    1B      GRP0    11111111  graphics player 0
    1C      GRP1    11111111  graphics player 1
    1D      ENAM0   ......1.  graphics (enable) missile 0
    1E      ENAM1   ......1.  graphics (enable) missile 1
    1F      ENABL   ......1.  graphics (enable) ball
    20      HMP0    1111....  horizontal motion player 0
    21      HMP1    1111....  horizontal motion player 1
    22      HMM0    1111....  horizontal motion missile 0
    23      HMM1    1111....  horizontal motion missile 1
    24      HMBL    1111....  horizontal motion ball
    25      VDELP0  .......1  vertical delay player 0
    26      VDELP1  .......1  vertical delay player 1
    27      VDELBL  .......1  vertical delay ball
    28      RESMP0  ......1.  reset missile 0 to player 0
    29      RESMP1  ......1.  reset missile 1 to player 1
    2A      HMOVE   <strobe>  apply horizontal motion
    2B      HMCLR   <strobe>  clear horizontal motion registers
    2C      CXCLR   <strobe>  clear collision latches

    30      CXM0P   11......  read collision M0-P1, M0-P0 (Bit 7,6)
    31      CXM1P   11......  read collision M1-P0, M1-P1
    32      CXP0FB  11......  read collision P0-PF, P0-BL
    33      CXP1FB  11......  read collision P1-PF, P1-BL
    34      CXM0FB  11......  read collision M0-PF, M0-BL
    35      CXM1FB  11......  read collision M1-PF, M1-BL
    36      CXBLPF  1.......  read collision BL-PF, unused
    37      CXPPMM  11......  read collision P0-P1, M0-M1
*/


Player0 player0;
Player1 player1;
Missile0 missile0;
Missile1 missile1;
Ball ball;
Playfield playfield;

int video_priority;
bool vsync;
bool vblank;
bool wsync;

uint8_t debug_nusiz0, debug_nusiz1, debug_hmp0, debug_hmp1, debug_hmm0, debug_hmm1, debug_hmbl;

void video_init_objects( void ) {
    player0.color = 0x000000ff; // black
    player0.copies = 1;
    player0.size = 8;
    player0.distance_between_copies = 16;
    player0.x = 2;
    
    player1.color = 0x000000ff; // black
    player1.copies = 1;
    player1.size = 8;
    player1.distance_between_copies = 16;
    player1.x = 2;

    missile0.color = 0x000000ff; // black
    missile0.copies = 1;
    missile0.size = 1;
    missile0.distance_between_copies = 16;
    missile0.x = 1;


    missile1.color = 0x000000ff; // black
    missile1.copies = 1;
    missile1.size = 1;
    missile1.distance_between_copies = 16;
    missile1.x = 1;

    ball.color = 0x000000ff; // black
    ball.size = 1;
    ball.x = 1;

    playfield.color_background =  0x000000ff; // black
    playfield.color_left_half = 0x000000ff; // black
    playfield.color_right_half =  0x000000ff; // black
    
}

void video_VSYNC_write ( uint8_t value ) {
    vsync = value>>1 & 1;
}



void video_VBLANK_write ( uint8_t value ) {
    vblank = value>>1 & 1;
    vblank6 = value>>6 & 1;
    vblank7 = value>>7 & 1;
}



void video_WSYNC_write ( void ) {
    wsync = true;
}



void video_RSYNC_write ( void ) {
    if (tv.x < 158) {
        player0.x = (player0.x + 157 - tv.x) % 160;
        player1.x = (player1.x + 157 - tv.x) % 160;
        missile0.x = (missile0.x + 157 - tv.x) % 160;
        missile1.x = (missile1.x + 157 - tv.x) % 160;
        ball.x = (ball.x + 157 - tv.x) % 160;
    }
    tv.x = 157;
    // if (tv.x < 156) {
    //     player0.x = (player0.x + 155 - tv.x) % 160;
    //     player1.x = (player1.x + 155 - tv.x) % 160;
    //     missile0.x = (missile0.x + 155 - tv.x) % 160;
    //     missile1.x = (missile1.x + 155 - tv.x) % 160;
    //     ball.x = (ball.x + 155 - tv.x) % 160;
    // }
    // tv.x = 155;
}



void video_NUSIZ0_write ( uint8_t value ) {
    debug_nusiz0 = value & 3;

    switch (value>>4 & 3) {
        case 0: missile0.size = 1; break;
        case 1: missile0.size = 2; break;
        case 2: missile0.size = 4; break;
        case 3: missile0.size = 8; break;
    }

    switch (value & 3) {
        case 0: {
            missile0.copies = player0.copies = 1;
            missile0.distance_between_copies = player0.distance_between_copies = 0;
            player0.size = 8;
            break;
        }
        case 1: {
            missile0.copies = player0.copies = 2;
            missile0.distance_between_copies = player0.distance_between_copies = 16;
            player0.size = 8;
            break;
        }
        case 2: {
            missile0.copies = player0.copies = 2;
            missile0.distance_between_copies = player0.distance_between_copies = 32;
            player0.size = 8;
            break;
        }
        case 3: {
            missile0.copies = player0.copies = 3;
            missile0.distance_between_copies = player0.distance_between_copies = 16;
            player0.size = 8;
            break;
        }
        case 4: {
            missile0.copies = player0.copies = 2;
            missile0.distance_between_copies = player0.distance_between_copies = 64;
            player0.size = 8;
            break;
        }
        case 5: {
            missile0.copies = player0.copies = 1;
            missile0.distance_between_copies = player0.distance_between_copies = 0;
            player0.size = 16;
            break;
        }
        case 6: {
            missile0.copies = player0.copies = 3;
            missile0.distance_between_copies = player0.distance_between_copies = 32;
            player0.size = 8;
            break;
        }
        case 7: {
            missile0.copies = player0.copies = 1;
            missile0.distance_between_copies = player0.distance_between_copies = 0;
            player0.size = 32;
            break;
        }
        
    }
}



void video_NUSIZ1_write ( uint8_t value ) {
    debug_nusiz1 = value & 3;

    switch (value>>4 & 3) {
        case 0: missile1.size = 1; break;
        case 1: missile1.size = 2; break;
        case 2: missile1.size = 4; break;
        case 3: missile1.size = 8; break;
    }

    switch (value & 3) {
        case 0: {
            missile1.copies = player1.copies = 1;
            missile1.distance_between_copies = player1.distance_between_copies = 0;
            player1.size = 8;
            break;
        }
        case 1: {
            missile1.copies = player1.copies = 2;
            missile1.distance_between_copies = player1.distance_between_copies = 16;
            player1.size = 8;
            break;
        }
        case 2: {
            missile1.copies = player1.copies = 2;
            missile1.distance_between_copies = player1.distance_between_copies = 32;
            player1.size = 8;
            break;
        }
        case 3: {
            missile1.copies = player1.copies = 3;
            missile1.distance_between_copies = player1.distance_between_copies = 16;
            player1.size = 8;
            break;
        }
        case 4: {
            missile1.copies = player1.copies = 2;
            missile1.distance_between_copies = player1.distance_between_copies = 64;
            player1.size = 8;
            break;
        }
        case 5: {
            missile1.copies = player1.copies = 1;
            missile1.distance_between_copies = player1.distance_between_copies = 0;
            player1.size = 16;
            break;
        }
        case 6: {
            missile1.copies = player1.copies = 3;
            missile1.distance_between_copies = player1.distance_between_copies = 32;
            player1.size = 8;
            break;
        }
        case 7: {
            missile1.copies = player1.copies = 1;
            missile1.distance_between_copies = player1.distance_between_copies = 0;
            player1.size = 32;
            break;
        }
        
    }
}



void video_COLUP0_write ( uint8_t value ) {
    player0.color = video_decode_color(value, tv.standard);
    missile0.color = video_decode_color(value, tv.standard);
    if (video_priority == VIDEO_PRIORITY_SCORE) {
        playfield.color_left_half = video_decode_color(value, tv.standard);
    }
}



void video_COLUP1_write ( uint8_t value ) {
    player1.color = video_decode_color(value, tv.standard);
    missile1.color = video_decode_color(value, tv.standard);
    if (video_priority == VIDEO_PRIORITY_SCORE) {
        playfield.color_right_half = video_decode_color(value, tv.standard);
    }
}



void video_COLUPF_write ( uint8_t value ) {
    ball.color = video_decode_color(value, tv.standard);
    if (video_priority != VIDEO_PRIORITY_SCORE) {
        playfield.color_left_half = video_decode_color(value, tv.standard);
        playfield.color_right_half = video_decode_color(value, tv.standard);
    }
}



void video_COLUBK_write ( uint8_t value ) {
    playfield.color_background = video_decode_color(value, tv.standard);
}



void video_CTRLPF_write ( uint8_t value ) {
    playfield.is_reflected = value & 1;
    if ( value>>2 & 1 ) video_priority = VIDEO_PRIORITY_HIGH;
    else if ( value>>1 & 1 ) video_priority = VIDEO_PRIORITY_SCORE;
    else video_priority = VIDEO_PRIORITY_NORMAL;

    if (video_priority == VIDEO_PRIORITY_SCORE) {
        playfield.color_left_half = player0.color;
        playfield.color_right_half = player1.color;
    }
    else {
        playfield.color_left_half = ball.color;
        playfield.color_right_half = ball.color;
    }

    switch (value>>4 & 3) {
        case 0: ball.size = 1; break;
        case 1: ball.size = 2; break;
        case 2: ball.size = 4; break;
        case 3: ball.size = 8; break;
    }
}



void video_REFP0_write ( uint8_t value ) {
    player0.is_mirrored = value & (1<<3);
}



void video_REFP1_write ( uint8_t value ) {
    player1.is_mirrored = value & (1<<3);
}



void video_PF0_write ( uint8_t value ) {
    playfield.sprite[0] = value & (1<<4);
    playfield.sprite[1] = value & (1<<5);
    playfield.sprite[2] = value & (1<<6);
    playfield.sprite[3] = value & (1<<7);
}



void video_PF1_write ( uint8_t value ) {
    playfield.sprite[4] = value & (1<<7);
    playfield.sprite[5] = value & (1<<6);
    playfield.sprite[6] = value & (1<<5);
    playfield.sprite[7] = value & (1<<4);
    playfield.sprite[8] = value & (1<<3);
    playfield.sprite[9] = value & (1<<2);
    playfield.sprite[10] = value & (1<<1);
    playfield.sprite[11] = value & (1<<0);
}



void video_PF2_write ( uint8_t value ) {
    playfield.sprite[12] = value & (1<<0);
    playfield.sprite[13] = value & (1<<1);
    playfield.sprite[14] = value & (1<<2);
    playfield.sprite[15] = value & (1<<3);
    playfield.sprite[16] = value & (1<<4);
    playfield.sprite[17] = value & (1<<5);
    playfield.sprite[18] = value & (1<<6);
    playfield.sprite[19] = value & (1<<7);
}



// https://www.youtube.com/watch?v=sJFnWZH5FXc see 12:47. mentions 4px, 5px, 6px
void video_RESP0_write ( void ) {
    if (player0.size > 8) {
        player0.x = (tv.x<160) ? (tv.x+6)%160 : 4;
        // player0.x = (tv.x+2<160) ? (tv.x+2+6)%160 : 4;
    }
    else {
        player0.x = (tv.x<160) ? (tv.x+5)%160 : 3;
        // player0.x = (tv.x+2<160) ? (tv.x+2+5)%160 : 3;
    }
}



void video_RESP1_write ( void ) {
    if (player1.size > 8) {
        player1.x = (tv.x<160) ? (tv.x+6)%160 : 4;
        // player1.x = (tv.x<158) ? (tv.x+8)%160 : 4;
    }
    else {
        player1.x = (tv.x<160) ? (tv.x+5)%160 : 3;
        // player1.x = (tv.x<158) ? (tv.x+7)%160 : 3;
    }
}



void video_RESM0_write ( void ) {
    missile0.x = (tv.x<160) ? (tv.x+4)%160 : 2;
    // missile0.x = (tv.x<158) ? (tv.x+6)%160 : 2;
}



void video_RESM1_write ( void ) {
    missile1.x = (tv.x<160) ? (tv.x+4)%160 : 2;
    // missile1.x = (tv.x<158) ? (tv.x+6)%160 : 2;
}



void video_RESBL_write ( void ) {
    ball.x = (tv.x<160) ? (tv.x+4)%160 : 2;
    // ball.x = (tv.x<158) ? (tv.x+6)%160 : 2;
}



void video_GRP0_write ( uint8_t value ) {
    player0.main_sprite[0] = value & (1<<7);
    player0.main_sprite[1] = value & (1<<6);
    player0.main_sprite[2] = value & (1<<5);
    player0.main_sprite[3] = value & (1<<4);
    player0.main_sprite[4] = value & (1<<3);
    player0.main_sprite[5] = value & (1<<2);
    player0.main_sprite[6] = value & (1<<1);
    player0.main_sprite[7] = value & (1<<0);
    
    player1.vdel_sprite[0] = player1.main_sprite[0];
    player1.vdel_sprite[1] = player1.main_sprite[1];
    player1.vdel_sprite[2] = player1.main_sprite[2];
    player1.vdel_sprite[3] = player1.main_sprite[3];
    player1.vdel_sprite[4] = player1.main_sprite[4];
    player1.vdel_sprite[5] = player1.main_sprite[5];
    player1.vdel_sprite[6] = player1.main_sprite[6];
    player1.vdel_sprite[7] = player1.main_sprite[7];
}



void video_GRP1_write ( uint8_t value ) {
    player1.main_sprite[0] = value & (1<<7);
    player1.main_sprite[1] = value & (1<<6);
    player1.main_sprite[2] = value & (1<<5);
    player1.main_sprite[3] = value & (1<<4);
    player1.main_sprite[4] = value & (1<<3);
    player1.main_sprite[5] = value & (1<<2);
    player1.main_sprite[6] = value & (1<<1);
    player1.main_sprite[7] = value & (1<<0);
    
    player0.vdel_sprite[0] = player0.main_sprite[0];
    player0.vdel_sprite[1] = player0.main_sprite[1];
    player0.vdel_sprite[2] = player0.main_sprite[2];
    player0.vdel_sprite[3] = player0.main_sprite[3];
    player0.vdel_sprite[4] = player0.main_sprite[4];
    player0.vdel_sprite[5] = player0.main_sprite[5];
    player0.vdel_sprite[6] = player0.main_sprite[6];
    player0.vdel_sprite[7] = player0.main_sprite[7];

    ball.vdel_sprite[0] = ball.main_sprite[0];
}



void video_ENAM0_write ( uint8_t value ) {
    missile0.sprite[0] = value & (1<<1);
}



void video_ENAM1_write ( uint8_t value ) {
    missile1.sprite[0] = value & (1<<1);
}



void video_ENABL_write ( uint8_t value ) {
    ball.main_sprite[0] = value & (1<<1);
}


int video_horizontal_movement_helper( uint8_t value ) {
    int dx = (value & 0xF0) >> 4;
    if (dx > 7) dx -= 16;
    return -dx;
}

void video_HMP0_write ( uint8_t value ) {
    debug_hmp0 = (value & 0xF0) >> 4;
    player0.dx = video_horizontal_movement_helper(value);
}



void video_HMP1_write ( uint8_t value ) {
    debug_hmp1 = (value & 0xF0) >> 4;
    player1.dx = video_horizontal_movement_helper(value);
}



void video_HMM0_write ( uint8_t value ) {
    debug_hmm0 = (value & 0xF0) >> 4;
    missile0.dx = video_horizontal_movement_helper(value);
}



void video_HMM1_write ( uint8_t value ) {
    debug_hmm1 = (value & 0xF0) >> 4;
    missile1.dx = video_horizontal_movement_helper(value);
}



void video_HMBL_write ( uint8_t value ) {
    debug_hmbl = (value & 0xF0) >> 4;
    ball.dx = video_horizontal_movement_helper(value);
}



void video_VDELP0_write ( uint8_t value ) {
    player0.is_vertically_delayed = value & 1;
}



void video_VDELP1_write ( uint8_t value ) {
    player1.is_vertically_delayed = value & 1;
}



void video_VDELBL_write ( uint8_t value ) {
    ball.is_vertically_delayed = value & 1;
}



void video_RESMP0_write ( uint8_t value ) {
    missile0.is_locked_on_player = (value>>1 & 1);
}



void video_RESMP1_write ( uint8_t value ) {
    missile1.is_locked_on_player = (value>>1 & 1);
}



void video_HMOVE_write ( void ) {
    // chatgpt claims repeated HMOVE calls don't accumulate dx, but it actually does
    // dont trust chatgpt, analyze working emulators instead
    player0.x = (player0.x + player0.dx + 160) % 160;
    player1.x = (player1.x + player1.dx + 160) % 160;
    missile0.x = (missile0.x + missile0.dx + 160) % 160;
    missile1.x = (missile1.x + missile1.dx + 160) % 160;
    ball.x = (ball.x + ball.dx + 160) % 160;
}



void video_HMCLR_write ( void ) {
    player0.dx = 0;
    player1.dx = 0;
    missile0.dx = 0;
    missile1.dx = 0;
    ball.dx = 0;
}



void video_CXCLR_write ( void ) {
    player0.collides_with_player1 = false;
    player0.collides_with_missile0 = false;
    player0.collides_with_missile1 = false;
    player0.collides_with_ball = false;
    player0.collides_with_playfield = false;
    player1.collides_with_missile0 = false;
    player1.collides_with_missile1 = false;
    player1.collides_with_ball = false;
    player1.collides_with_playfield = false;
    missile0.collides_with_missile1 = false;
    missile0.collides_with_ball = false;
    missile0.collides_with_playfield = false;
    missile1.collides_with_ball = false;
    missile1.collides_with_playfield = false;
    ball.collides_with_playfield = false;
}



uint8_t video_CXM0P_read ( void ) {
    bool m0p1 = player1.collides_with_missile0;
    bool m0p0 = player0.collides_with_missile0;
    return (m0p1<<7) | (m0p0<<6);
}



uint8_t video_CXM1P_read ( void ) {
    bool m1p0 = player0.collides_with_missile1;
    bool m1p1 = player1.collides_with_missile1;
    return (m1p0<<7) | (m1p1<<6);
}



uint8_t video_CXP0FB_read ( void ) {
    bool p0pf = player0.collides_with_playfield;
    bool p0bl = player0.collides_with_ball;
    return (p0pf<<7) | (p0bl<<6);
}



uint8_t video_CXP1FB_read ( void ) {
    bool p1pf = player1.collides_with_playfield;
    bool p1bl = player1.collides_with_ball;
    return (p1pf<<7) | (p1bl<<6);
}



uint8_t video_CXM0FB_read ( void ) {
    bool m0pf = missile0.collides_with_playfield;
    bool m0bl = missile0.collides_with_ball;
    return (m0pf<<7) | (m0bl<<6);
}



uint8_t video_CXM1FB_read ( void ) {
    bool m1pf = missile1.collides_with_playfield;
    bool m1bl = missile1.collides_with_ball;
    return (m1pf<<7) | (m1bl<<6);
}



uint8_t video_CXBLPF_read ( void ) {
    bool blpf = ball.collides_with_playfield;
    return (blpf<<7);
}



uint8_t video_CXPPMM_read ( void ) {
    bool p0p1 = player0.collides_with_player1;
    bool m0m1 = missile0.collides_with_missile1;
    return (p0p1<<7) | (m0m1<<6);
}



void video_calculate_collisions( int x) {
    if (x<0 || x>=160) return;
    bool pf = video_playfield_occupies_x(x);
    bool bl = video_ball_occupies_x(x);
    bool m0 = video_missile0_occupies_x(x);
    bool m1 = video_missile1_occupies_x(x);
    bool p0 = video_player0_occupies_x(x);
    bool p1 = video_player1_occupies_x(x);

    player0.collides_with_player1 |= p0 && p1;
    player0.collides_with_missile0 |= p0 && m0;
    player0.collides_with_missile1 |= p0 && m1;
    player0.collides_with_ball |= p0 && bl;
    player0.collides_with_playfield |= p0 && pf;
    player1.collides_with_missile0 |= p1 && m0;
    player1.collides_with_missile1 |= p1 && m1;
    player1.collides_with_ball |= p1 && bl;
    player1.collides_with_playfield |= p1 && pf;
    missile0.collides_with_missile1 |= m0 && m1;
    missile0.collides_with_ball |= m0 && bl;
    missile0.collides_with_playfield |= m0 && pf;
    missile1.collides_with_ball |= m1 && bl;
    missile1.collides_with_playfield |= m1 && pf;
    ball.collides_with_playfield |= bl && pf;
}


int video_calculate_pixel( int x ) {
    if (x<0 || x>=160 || vblank) return 0x000000ff; // black
    bool pf = video_playfield_occupies_x(x);
    bool bl = video_ball_occupies_x(x);
    bool m0 = video_missile0_occupies_x(x);
    bool m1 = video_missile1_occupies_x(x);
    bool p0 = video_player0_occupies_x(x);
    bool p1 = video_player1_occupies_x(x);
    if (video_priority==VIDEO_PRIORITY_NORMAL) {
        if (p0 || m0) return player0.color;
        if (p1 || m1) return player1.color;
        if (pf || bl) return ball.color;
        return playfield.color_background;
    }
    else if (video_priority==VIDEO_PRIORITY_SCORE) {
        if (x<80) {
            if (p0 || m0 || pf) return player0.color;
            if (p1 || m1) return player1.color;
            if (bl) return ball.color;
            return playfield.color_background;
        } else {
            if (p0 || m0) return player0.color;
            if (p1 || m1 || pf) return player1.color;
            if (bl) return ball.color;
            return playfield.color_background;
        }
    }
    else { // VIDEO_PRIORITY_HIGH
        if (pf || bl) return ball.color;
        if (p0 || m0) return player0.color;
        if (p1 || m1) return player1.color;
        return playfield.color_background;
    }
}


bool video_playfield_occupies_x( int x ) {
    if (x<0 || x>=160 ) return false;
    if (x < 80 ) return playfield.sprite[ x/4 ];
    // 80<=x<160
    if (!playfield.is_reflected) return playfield.sprite[ (x-80)/4 ];
    // reflected
    return playfield.sprite[ 19 - (x-80)/4 ];
}

bool video_ball_occupies_x( int x ) {
    if (x<0 || x>=160 ) return false;
    bool* effective_sprite = ball.is_vertically_delayed ? ball.vdel_sprite : ball.main_sprite;
    if (!effective_sprite[0]) return false;
    for (int i=0; i<ball.size; i++) {
        if ( (video_ball_effective_x() + i ) % 160 == x ) return true;
    }
    return false;
}

bool video_missile0_occupies_x( int x ) {
    if (x<0 || x>=160 ) return false;
    if (!missile0.sprite[0]) return false;
    int start = video_missile0_effective_x();
    for (int i=0; i<missile0.size; i++) {
        for (int j=0; j<missile0.copies; j++) {
            if ( (start + i + j*missile0.distance_between_copies) % 160 == x ) return true;
        }
    }
    return false;
}


bool video_missile1_occupies_x( int x ) {
    if (x<0 || x>=160 ) return false;
    if (!missile1.sprite[0]) return false;
    int start = video_missile1_effective_x();
    for (int i=0; i<missile1.size; i++) {
        for (int j=0; j<missile1.copies; j++) {
            if ( (start + i + j*missile1.distance_between_copies) % 160 == x ) return true;
        }
    }
    return false;
}


void video_calculate_effective_player_sprite( bool final_sprite[32], bool init_sprite[8], int size, bool is_mirrored ) {
    if (size == 8) {
        if (!is_mirrored) {
            for (int i = 0; i<8; i++) {
                final_sprite[i] = init_sprite[i];
            }
        } else { // mirrored
            for (int i = 0; i<8; i++) {
                final_sprite[i] = init_sprite[7-i];
            }
        }
    } else if (size == 16) {
        if (!is_mirrored) {
            for (int i = 0; i<16; i++) {
                final_sprite[i] = init_sprite[i/2];
            }
        } else { // mirrored
            for (int i = 0; i<16; i++) {
                final_sprite[i] = init_sprite[7-i/2];
            }
        }
    } else { // size==32
        if (!is_mirrored) {
            for (int i = 0; i<32; i++) {
                final_sprite[i] = init_sprite[i/4];
            }
        } else { // mirrored
            for (int i = 0; i<32; i++) {
                final_sprite[i] = init_sprite[7-i/4];
            }
        }
    }
}



bool video_player0_occupies_x( int x) {
    if (x<0 || x>=160 ) return false;
    bool effective_sprite[32] = {0};
    video_calculate_effective_player_sprite(
        effective_sprite,
        player0.is_vertically_delayed ? player0.vdel_sprite : player0.main_sprite,
        player0.size,
        player0.is_mirrored);
    int start = video_player0_effective_x();
    for (int i=0; i<player0.size; i++) {
        for (int j=0; j<player0.copies; j++) {
            if ( (start + i + j*player0.distance_between_copies) % 160 == x ) return effective_sprite[i];
        }
    }
    return false;
}

bool video_player1_occupies_x( int x) {
    if (x<0 || x>=160 ) return false;
    bool effective_sprite[32] = {0};
    video_calculate_effective_player_sprite(
        effective_sprite,
        player1.is_vertically_delayed ? player1.vdel_sprite : player1.main_sprite,
        player1.size,
        player1.is_mirrored);
    int start = video_player1_effective_x();
    for (int i=0; i<player1.size; i++) {
        for (int j=0; j<player1.copies; j++) {
            if ( (start + i + j*player1.distance_between_copies) % 160 == x ) return effective_sprite[i];
        }
    }
    return false;
}


int video_decode_color( uint8_t color, int tv_standard ) {
    const int ntsc_colors[] = {
        0x000000ff, 0x404040ff, 0x6c6c6cff, 0x909090ff, 0xb0b0b0ff, 0xc8c8c8ff, 0xdcdcdcff, 0xecececff,
        0x444400ff, 0x646410ff, 0x848424ff, 0xa0a034ff, 0xb8b840ff, 0xd0d050ff, 0xe8e85cff, 0xfcfc68ff,
        0x702800ff, 0x844414ff, 0x985c28ff, 0xac783cff, 0xbc8c4cff, 0xcca05cff, 0xdcb468ff, 0xecc878ff,
        0x841800ff, 0x983418ff, 0xac5030ff, 0xc06848ff, 0xd0805cff, 0xe09470ff, 0xeca880ff, 0xfcbc94ff,
        0x880000ff, 0x9c2020ff, 0xb03c3cff, 0xc05858ff, 0xd07070ff, 0xe08888ff, 0xeca0a0ff, 0xfcb4b4ff,
        0x78005cff, 0x8c2074ff, 0xa03c88ff, 0xb0589cff, 0xc070b0ff, 0xd084c0ff, 0xdc9cd0ff, 0xecb0e0ff,
        0x480078ff, 0x602090ff, 0x783ca4ff, 0x8c58b8ff, 0xa070ccff, 0xb484dcff, 0xc49cecff, 0xd4b0fcff,
        0x140084ff, 0x302098ff, 0x4c3cacff, 0x6858c0ff, 0x7c70d0ff, 0x9488e0ff, 0xa8a0ecff, 0xbcb4fcff,
        0x000088ff, 0x1c209cff, 0x3840b0ff, 0x505cc0ff, 0x6874d0ff, 0x7c8ce0ff, 0x90a4ecff, 0xa4b8fcff,
        0x00187cff, 0x1c3890ff, 0x3854a8ff, 0x5070bcff, 0x6888ccff, 0x7c9cdcff, 0x90b4ecff, 0xa4c8fcff,
        0x002c5cff, 0x1c4c78ff, 0x386890ff, 0x5084acff, 0x689cc0ff, 0x7cb4d4ff, 0x90cce8ff, 0xa4e0fcff,
        0x003c2cff, 0x1c5c48ff, 0x387c64ff, 0x509c80ff, 0x68b494ff, 0x7cd0acff, 0x90e4c0ff, 0xa4fcd4ff,
        0x003c00ff, 0x205c20ff, 0x407c40ff, 0x5c9c5cff, 0x74b474ff, 0x8cd08cff, 0xa4e4a4ff, 0xb8fcb8ff,
        0x143800ff, 0x345c1cff, 0x507c38ff, 0x6c9850ff, 0x84b468ff, 0x9ccc7cff, 0xb4e490ff, 0xc8fca4ff,
        0x2c3000ff, 0x4c501cff, 0x687034ff, 0x848c4cff, 0x9ca864ff, 0xb4c078ff, 0xccd488ff, 0xe0ec9cff,
        0x442800ff, 0x644818ff, 0x846830ff, 0xa08444ff, 0xb89c58ff, 0xd0b46cff, 0xe8cc7cff, 0xfce08cff
    };

    const int pal_colors[] = {
        0x000000ff, 0x282828ff, 0x505050ff, 0x747474ff, 0x949494ff, 0xb4b4b4ff, 0xd0d0d0ff, 0xecececff,
        0x000000ff, 0x282828ff, 0x505050ff, 0x747474ff, 0x949494ff, 0xb4b4b4ff, 0xd0d0d0ff, 0xecececff,
        0x805800ff, 0x947020ff, 0xa8843cff, 0xbc9c58ff, 0xccac70ff, 0xdcc084ff, 0xecd09cff, 0xfce0b0ff,
        0x445c00ff, 0x5c7820ff, 0x74903cff, 0x8cac58ff, 0xa0c070ff, 0xb0d484ff, 0xc4e89cff, 0xd4fcb0ff,
        0x703400ff, 0x885020ff, 0xa0683cff, 0xb48458ff, 0xc89870ff, 0xdcac84ff, 0xecc09cff, 0xfcd4b0ff,
        0x006414ff, 0x208034ff, 0x3c9850ff, 0x58b06cff, 0x70c484ff, 0x84d89cff, 0x9ce8b4ff, 0xb0fcc8ff,
        0x700014ff, 0x882034ff, 0xa03c50ff, 0xb4586cff, 0xc87084ff, 0xdc849cff, 0xec9cb4ff, 0xfcb0c8ff,
        0x005c5cff, 0x207474ff, 0x3c8c8cff, 0x58a4a4ff, 0x70b8b8ff, 0x84c8c8ff, 0x9cdcdcff, 0xb0ececff,
        0x70005cff, 0x842074ff, 0x943c88ff, 0xa8589cff, 0xb470b0ff, 0xc484c0ff, 0xd09cd0ff, 0xe0b0e0ff,
        0x003c70ff, 0x1c5888ff, 0x3874a0ff, 0x508cb4ff, 0x68a4c8ff, 0x7cb8dcff, 0x90ccecff, 0xa4e0fcff,
        0x580070ff, 0x6c2088ff, 0x803ca0ff, 0x9458b4ff, 0xa470c8ff, 0xb484dcff, 0xc49cecff, 0xd4b0fcff,
        0x002070ff, 0x1c3c88ff, 0x3858a0ff, 0x5074b4ff, 0x6888c8ff, 0x7ca0dcff, 0x90b4ecff, 0xa4c8fcff,
        0x3c0080ff, 0x542094ff, 0x6c3ca8ff, 0x8058bcff, 0x9470ccff, 0xa884dcff, 0xb89cecff, 0xc8b0fcff,
        0x000088ff, 0x20209cff, 0x3c3cb0ff, 0x5858c0ff, 0x7070d0ff, 0x8484e0ff, 0x9c9cecff, 0xb0b0fcff,
        0x000000ff, 0x282828ff, 0x505050ff, 0x747474ff, 0x949494ff, 0xb4b4b4ff, 0xd0d0d0ff, 0xecececff,
        0x000000ff, 0x282828ff, 0x505050ff, 0x747474ff, 0x949494ff, 0xb4b4b4ff, 0xd0d0d0ff, 0xecececff
    };

    const int secam_colors[] = {
        0x000000ff, 0x2121ffff, 0xf03c79ff, 0xff50ffff, 0x7fff00ff, 0x7fffffff, 0xffff3fff, 0xffffffff,
        0x000000ff, 0x2121ffff, 0xf03c79ff, 0xff50ffff, 0x7fff00ff, 0x7fffffff, 0xffff3fff, 0xffffffff,
        0x000000ff, 0x2121ffff, 0xf03c79ff, 0xff50ffff, 0x7fff00ff, 0x7fffffff, 0xffff3fff, 0xffffffff,
        0x000000ff, 0x2121ffff, 0xf03c79ff, 0xff50ffff, 0x7fff00ff, 0x7fffffff, 0xffff3fff, 0xffffffff,
        0x000000ff, 0x2121ffff, 0xf03c79ff, 0xff50ffff, 0x7fff00ff, 0x7fffffff, 0xffff3fff, 0xffffffff,
        0x000000ff, 0x2121ffff, 0xf03c79ff, 0xff50ffff, 0x7fff00ff, 0x7fffffff, 0xffff3fff, 0xffffffff,
        0x000000ff, 0x2121ffff, 0xf03c79ff, 0xff50ffff, 0x7fff00ff, 0x7fffffff, 0xffff3fff, 0xffffffff,
        0x000000ff, 0x2121ffff, 0xf03c79ff, 0xff50ffff, 0x7fff00ff, 0x7fffffff, 0xffff3fff, 0xffffffff,
        0x000000ff, 0x2121ffff, 0xf03c79ff, 0xff50ffff, 0x7fff00ff, 0x7fffffff, 0xffff3fff, 0xffffffff,
        0x000000ff, 0x2121ffff, 0xf03c79ff, 0xff50ffff, 0x7fff00ff, 0x7fffffff, 0xffff3fff, 0xffffffff,
        0x000000ff, 0x2121ffff, 0xf03c79ff, 0xff50ffff, 0x7fff00ff, 0x7fffffff, 0xffff3fff, 0xffffffff,
        0x000000ff, 0x2121ffff, 0xf03c79ff, 0xff50ffff, 0x7fff00ff, 0x7fffffff, 0xffff3fff, 0xffffffff,
        0x000000ff, 0x2121ffff, 0xf03c79ff, 0xff50ffff, 0x7fff00ff, 0x7fffffff, 0xffff3fff, 0xffffffff,
        0x000000ff, 0x2121ffff, 0xf03c79ff, 0xff50ffff, 0x7fff00ff, 0x7fffffff, 0xffff3fff, 0xffffffff,
        0x000000ff, 0x2121ffff, 0xf03c79ff, 0xff50ffff, 0x7fff00ff, 0x7fffffff, 0xffff3fff, 0xffffffff,
        0x000000ff, 0x2121ffff, 0xf03c79ff, 0xff50ffff, 0x7fff00ff, 0x7fffffff, 0xffff3fff, 0xffffffff
    };

    const int monochrome_colors[] = {
        0x000000ff, 0x282828ff, 0x505050ff, 0x747474ff, 0x949494ff, 0xb4b4b4ff, 0xd0d0d0ff, 0xecececff,
        0x000000ff, 0x282828ff, 0x505050ff, 0x747474ff, 0x949494ff, 0xb4b4b4ff, 0xd0d0d0ff, 0xecececff,
        0x000000ff, 0x282828ff, 0x505050ff, 0x747474ff, 0x949494ff, 0xb4b4b4ff, 0xd0d0d0ff, 0xecececff,
        0x000000ff, 0x282828ff, 0x505050ff, 0x747474ff, 0x949494ff, 0xb4b4b4ff, 0xd0d0d0ff, 0xecececff,
        0x000000ff, 0x282828ff, 0x505050ff, 0x747474ff, 0x949494ff, 0xb4b4b4ff, 0xd0d0d0ff, 0xecececff,
        0x000000ff, 0x282828ff, 0x505050ff, 0x747474ff, 0x949494ff, 0xb4b4b4ff, 0xd0d0d0ff, 0xecececff,
        0x000000ff, 0x282828ff, 0x505050ff, 0x747474ff, 0x949494ff, 0xb4b4b4ff, 0xd0d0d0ff, 0xecececff,
        0x000000ff, 0x282828ff, 0x505050ff, 0x747474ff, 0x949494ff, 0xb4b4b4ff, 0xd0d0d0ff, 0xecececff,
        0x000000ff, 0x282828ff, 0x505050ff, 0x747474ff, 0x949494ff, 0xb4b4b4ff, 0xd0d0d0ff, 0xecececff,
        0x000000ff, 0x282828ff, 0x505050ff, 0x747474ff, 0x949494ff, 0xb4b4b4ff, 0xd0d0d0ff, 0xecececff,
        0x000000ff, 0x282828ff, 0x505050ff, 0x747474ff, 0x949494ff, 0xb4b4b4ff, 0xd0d0d0ff, 0xecececff,
        0x000000ff, 0x282828ff, 0x505050ff, 0x747474ff, 0x949494ff, 0xb4b4b4ff, 0xd0d0d0ff, 0xecececff,
        0x000000ff, 0x282828ff, 0x505050ff, 0x747474ff, 0x949494ff, 0xb4b4b4ff, 0xd0d0d0ff, 0xecececff,
        0x000000ff, 0x282828ff, 0x505050ff, 0x747474ff, 0x949494ff, 0xb4b4b4ff, 0xd0d0d0ff, 0xecececff,
        0x000000ff, 0x282828ff, 0x505050ff, 0x747474ff, 0x949494ff, 0xb4b4b4ff, 0xd0d0d0ff, 0xecececff,
        0x000000ff, 0x282828ff, 0x505050ff, 0x747474ff, 0x949494ff, 0xb4b4b4ff, 0xd0d0d0ff, 0xecececff
    };

    if (tv_standard == TV_NTSC) return ntsc_colors[ color>>1 ];
    else if (tv_standard == TV_PAL) return pal_colors[ color>>1 ];
    else if (tv_standard == TV_SECAM) return secam_colors[ color>>1 ];
    else return monochrome_colors[ color>>1 ];
    
}


int video_player0_effective_x ( void) {
    return player0.x;
}

int video_player1_effective_x ( void) {
    return player1.x;
}

int video_missile0_effective_x ( void) {
    if (missile0.is_locked_on_player) {
        return (video_player0_effective_x() + (player0.size - missile0.size) / 2 ) % 160;
    }
    else return missile0.x;
}

int video_missile1_effective_x ( void) {
    if (missile1.is_locked_on_player) {
        return (video_player1_effective_x() + (player1.size - missile1.size) / 2 ) % 160;
    }
    else return missile1.x;
}

int video_ball_effective_x ( void) {
    return ball.x;
}