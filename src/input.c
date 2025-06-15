
#include <stdint.h>
#include <raylib.h>

#include "atari2600emulator.h"

/*
    0280    SWCHA   11111111  Port A; input or output  (read or write)
    0281    SWACNT  11111111  Port A DDR, 0= input, 1=output
    0282    SWCHB   11111111  Port B; console switches (read only)
    0283    SWBCNT  11111111  Port B DDR (hardwired as input)

    38      INPT0   1.......  read pot port
    39      INPT1   1.......  read pot port
    3A      INPT2   1.......  read pot port
    3B      INPT3   1.......  read pot port
    3C      INPT4   1.......  read input
    3D      INPT5   1.......  read input
*/

uint8_t swacnt;
uint8_t swbcnt;
uint8_t swcha;
bool reset_btn_pressed;
bool select_btn_pressed;
bool color_on = 1;
bool p0_expert_on;
bool p1_expert_on;
int input_mode; // joystick, paddle etc
bool vblank6, vblank7;
uint8_t inpt0=0x80, inpt1=0x80, inpt2=0x80, inpt3=0x80;
uint8_t inpt4=0x80, inpt5=0x80; // these are latched, so must be saved
Joystick joystick0, joystick1;
Paddle paddle0, paddle1, paddle2, paddle3;
Keypad keypad0, keypad1;

uint8_t input_INPT0_read ( void ) {
    return inpt0;
}



uint8_t input_INPT1_read ( void ) {
    return inpt1;
}



uint8_t input_INPT2_read ( void ) {
    return inpt2;
}



uint8_t input_INPT3_read ( void ) {
    return inpt3;
}



uint8_t input_INPT4_read ( void ) {
    return inpt4;
}


// latch
uint8_t input_INPT5_read ( void ) {
    return inpt5;
    // if (input_mode == INPUT_JOYSTICK) {
    //     return (!joystick1.btn_pressed)<<7;
    // }
    // else if (input_mode == INPUT_PADDLE) {
    //     //
    // }
    // else if (input_mode == INPUT_KEYPAD) {
    //     //
    // }
    // return 0;
}


//
uint8_t input_SWCHA_read ( void ) {
    return swcha & ~swacnt;
    /* karnaugh map:
    swacnt  swcha   |   new_swcha
    ------  -----   |   ---------
       0      0     |       0
       0      1     |       1
       1      0     |       0
       1      1     |       0
    */
}



uint8_t input_SWACNT_read ( void ) {
    return swacnt;
}



uint8_t input_SWCHB_read ( void ) {
    uint8_t val = p1_expert_on<<7
        | p0_expert_on<<6
        | 1<<5
        | 1<<4
        | color_on<<3
        | 1<<2
        | !select_btn_pressed<<1
        | !reset_btn_pressed;
    return val & ~swbcnt;
}



uint8_t input_SWBCNT_read ( void ) {
    return swbcnt;
}



void input_SWCHA_write ( uint8_t value ) {
    swcha = swacnt & value | ~swacnt & swcha;
    /* karnaugh map:
    swacnt  swcha   value   |   new_swcha
    ------  -----   -----   |   ---------
       0      0       0     |       0
       0      0       1     |       0
       0      1       0     |       1
       0      1       1     |       1
       1      0       0     |       0
       1      0       1     |       1
       1      1       0     |       0
       1      1       1     |       1
    */
    return;
}



void input_SWACNT_write ( uint8_t value ) {
    swacnt = value;
    return;
}



void input_SWCHB_write ( uint8_t value ) {
    return; // hardwired as input
}



void input_SWBCNT_write ( uint8_t value ) {
    swbcnt = value;
    return;
}





float input_paddle_load_amount(Paddle p) {
    float scanlines = (p.pod_angle + 135) / 270.0;
    return 100.0 / (scanlines*380*228 + 1);
}

void input_register_inputs(void) {
    joystick0.up_pressed = IsKeyDown(KEY_W);
    joystick0.down_pressed = IsKeyDown(KEY_S);
    joystick0.left_pressed = IsKeyDown(KEY_A);
    joystick0.right_pressed = IsKeyDown(KEY_D);
    joystick0.btn_pressed = IsKeyDown(KEY_F);
    joystick0.btn2_pressed = IsKeyDown(KEY_C);
    joystick0.btn3_pressed = IsKeyDown(KEY_V);
    joystick1.up_pressed = IsKeyDown(KEY_I);
    joystick1.down_pressed = IsKeyDown(KEY_K);
    joystick1.left_pressed = IsKeyDown(KEY_J);
    joystick1.right_pressed = IsKeyDown(KEY_L);
    joystick1.btn_pressed = IsKeyDown(KEY_SEMICOLON);
    joystick1.btn2_pressed = IsKeyDown(KEY_PERIOD);
    joystick1.btn3_pressed = IsKeyDown(KEY_SLASH);

    paddle0.btn_pressed = IsKeyDown(KEY_A);
    paddle1.btn_pressed = IsKeyDown(KEY_S);
    paddle2.btn_pressed = IsKeyDown(KEY_J);
    paddle3.btn_pressed = IsKeyDown(KEY_K);
    if (IsKeyDown(KEY_D)) paddle0.pod_angle -= 1;
    if (IsKeyDown(KEY_F)) paddle0.pod_angle += 1;
    if (IsKeyDown(KEY_C)) paddle1.pod_angle -= 1;
    if (IsKeyDown(KEY_V)) paddle1.pod_angle += 1;
    if (IsKeyDown(KEY_L)) paddle2.pod_angle -= 1;
    if (IsKeyDown(KEY_SEMICOLON)) paddle2.pod_angle += 1;
    if (IsKeyDown(KEY_PERIOD)) paddle3.pod_angle -= 1;
    if (IsKeyDown(KEY_SLASH)) paddle3.pod_angle += 1;
    if (paddle0.pod_angle<-135) paddle0.pod_angle = -135;
    if (paddle1.pod_angle<-135) paddle1.pod_angle = -135;
    if (paddle2.pod_angle<-135) paddle2.pod_angle = -135;
    if (paddle3.pod_angle<-135) paddle3.pod_angle = -135;
    if (paddle0.pod_angle>135) paddle0.pod_angle = 135;
    if (paddle1.pod_angle>135) paddle1.pod_angle = 135;
    if (paddle2.pod_angle>135) paddle2.pod_angle = 135;
    if (paddle3.pod_angle>135) paddle3.pod_angle = 135;

    keypad0.key_pressed[0][0] = IsKeyDown(KEY_ONE);
    keypad0.key_pressed[0][1] = IsKeyDown(KEY_TWO);
    keypad0.key_pressed[0][2] = IsKeyDown(KEY_THREE);
    keypad0.key_pressed[1][0] = IsKeyDown(KEY_Q);
    keypad0.key_pressed[1][1] = IsKeyDown(KEY_W);
    keypad0.key_pressed[1][2] = IsKeyDown(KEY_E);
    keypad0.key_pressed[2][0] = IsKeyDown(KEY_A);
    keypad0.key_pressed[2][1] = IsKeyDown(KEY_S);
    keypad0.key_pressed[2][2] = IsKeyDown(KEY_D);
    keypad0.key_pressed[3][0] = IsKeyDown(KEY_Z);
    keypad0.key_pressed[3][1] = IsKeyDown(KEY_X);
    keypad0.key_pressed[3][2] = IsKeyDown(KEY_C);
    keypad1.key_pressed[0][0] = IsKeyDown(KEY_SEVEN);
    keypad1.key_pressed[0][1] = IsKeyDown(KEY_EIGHT);
    keypad1.key_pressed[0][2] = IsKeyDown(KEY_NINE);
    keypad1.key_pressed[1][0] = IsKeyDown(KEY_U);
    keypad1.key_pressed[1][1] = IsKeyDown(KEY_I);
    keypad1.key_pressed[1][2] = IsKeyDown(KEY_O);
    keypad1.key_pressed[2][0] = IsKeyDown(KEY_J);
    keypad1.key_pressed[2][1] = IsKeyDown(KEY_K);
    keypad1.key_pressed[2][2] = IsKeyDown(KEY_L);
    keypad1.key_pressed[3][0] = IsKeyDown(KEY_M);
    keypad1.key_pressed[3][1] = IsKeyDown(KEY_COMMA);
    keypad1.key_pressed[3][2] = IsKeyDown(KEY_PERIOD);

    if (input_mode==INPUT_JOYSTICK) {
        swcha = !joystick0.right_pressed<<7
            | !joystick0.left_pressed<<6
            | !joystick0.down_pressed<<5
            | !joystick0.up_pressed<<4
            | !joystick1.right_pressed<<3
            | !joystick1.left_pressed<<2
            | !joystick1.down_pressed<<1
            | !joystick1.up_pressed;
        
        if (vblank7) {
            inpt0 = inpt1 = inpt2 = inpt3 = 0;
        }
        else {
            inpt0 = joystick0.btn2_pressed<<7;
            inpt1 = joystick0.btn3_pressed<<7;
            inpt2 = joystick1.btn2_pressed<<7;
            inpt3 = joystick1.btn3_pressed<<7;
        }
        if (vblank6) { // latched input
            inpt4 &= !joystick0.btn_pressed<<7;
            inpt5 &= !joystick1.btn_pressed<<7;
        }
        else {
            inpt4 = !joystick0.btn_pressed<<7;
            inpt5 = !joystick1.btn_pressed<<7;
        }
    }
    else if (input_mode==INPUT_PADDLE) {
        swcha = !paddle0.btn_pressed<<7
            | !paddle1.btn_pressed<<6
            | 0<<5
            | 0<<4
            | !paddle2.btn_pressed<<3
            | !paddle3.btn_pressed<<2
            | 0<<1
            | 0;

        if (vblank7) {
            paddle0.pod_loading_percent = 0;
            paddle1.pod_loading_percent = 0;
            paddle2.pod_loading_percent = 0;
            paddle3.pod_loading_percent = 0;
        }
        else {
            paddle0.pod_loading_percent += input_paddle_load_amount(paddle0);
            paddle1.pod_loading_percent += input_paddle_load_amount(paddle1);
            paddle2.pod_loading_percent += input_paddle_load_amount(paddle2);
            paddle3.pod_loading_percent += input_paddle_load_amount(paddle3);
        }

        inpt0 = (paddle0.pod_loading_percent>=100)<<7;
        inpt1 = (paddle1.pod_loading_percent>=100)<<7;
        inpt2 = (paddle2.pod_loading_percent>=100)<<7;
        inpt3 = (paddle3.pod_loading_percent>=100)<<7;

        if (paddle0.pod_loading_percent>=100) paddle0.pod_loading_percent = 100;
        if (paddle1.pod_loading_percent>=100) paddle1.pod_loading_percent = 100;
        if (paddle2.pod_loading_percent>=100) paddle2.pod_loading_percent = 100;
        if (paddle3.pod_loading_percent>=100) paddle3.pod_loading_percent = 100;


    }
    else if (input_mode==INPUT_KEYPAD) {
        bool keypad1_row0_selected = (swacnt & ~swcha)>>0 & 1;
        bool keypad1_row1_selected = (swacnt & ~swcha)>>1 & 1;
        bool keypad1_row2_selected = (swacnt & ~swcha)>>2 & 1;
        bool keypad1_row3_selected = (swacnt & ~swcha)>>3 & 1;
        bool keypad0_row0_selected = (swacnt & ~swcha)>>4 & 1;
        bool keypad0_row1_selected = (swacnt & ~swcha)>>5 & 1;
        bool keypad0_row2_selected = (swacnt & ~swcha)>>6 & 1;
        bool keypad0_row3_selected = (swacnt & ~swcha)>>7 & 1;
        
        if (vblank7) {
            inpt0 = inpt1 = inpt2 = inpt3 = 0;
        }
        else {
            inpt0 = !(  keypad0_row0_selected && keypad0.key_pressed[0][0] ||
                        keypad0_row1_selected && keypad0.key_pressed[1][0] ||
                        keypad0_row2_selected && keypad0.key_pressed[2][0] ||
                        keypad0_row3_selected && keypad0.key_pressed[3][0])<<7;
            inpt1 = !(  keypad0_row0_selected && keypad0.key_pressed[0][1] ||
                        keypad0_row1_selected && keypad0.key_pressed[1][1] ||
                        keypad0_row2_selected && keypad0.key_pressed[2][1] ||
                        keypad0_row3_selected && keypad0.key_pressed[3][1])<<7;
            inpt2 = !(  keypad1_row0_selected && keypad1.key_pressed[0][0] ||
                        keypad1_row1_selected && keypad1.key_pressed[1][0] ||
                        keypad1_row2_selected && keypad1.key_pressed[2][0] ||
                        keypad1_row3_selected && keypad1.key_pressed[3][0])<<7;
            inpt3 = !(  keypad1_row0_selected && keypad1.key_pressed[0][1] ||
                        keypad1_row1_selected && keypad1.key_pressed[1][1] ||
                        keypad1_row2_selected && keypad1.key_pressed[2][1] ||
                        keypad1_row3_selected && keypad1.key_pressed[3][1])<<7;
        }
        if (vblank6) { // latched input
            inpt4 &= !( keypad0_row0_selected && keypad0.key_pressed[0][2] ||
                        keypad0_row1_selected && keypad0.key_pressed[1][2] ||
                        keypad0_row2_selected && keypad0.key_pressed[2][2] ||
                        keypad0_row3_selected && keypad0.key_pressed[3][2])<<7;
            inpt5 &= !( keypad1_row0_selected && keypad1.key_pressed[0][2] ||
                        keypad1_row1_selected && keypad1.key_pressed[1][2] ||
                        keypad1_row2_selected && keypad1.key_pressed[2][2] ||
                        keypad1_row3_selected && keypad1.key_pressed[3][2])<<7;
        }
        else {
            inpt4 = !(  keypad0_row0_selected && keypad0.key_pressed[0][2] ||
                        keypad0_row1_selected && keypad0.key_pressed[1][2] ||
                        keypad0_row2_selected && keypad0.key_pressed[2][2] ||
                        keypad0_row3_selected && keypad0.key_pressed[3][2])<<7;
            inpt5 = !(  keypad1_row0_selected && keypad1.key_pressed[0][2] ||
                        keypad1_row1_selected && keypad1.key_pressed[1][2] ||
                        keypad1_row2_selected && keypad1.key_pressed[2][2] ||
                        keypad1_row3_selected && keypad1.key_pressed[3][2])<<7;
        }
    }
}


