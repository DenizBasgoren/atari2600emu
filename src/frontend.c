// clang src/frontend.c src/cpu.c src/memory.c src/video.c src/timer.c src/audio.c src/input.c -lraylib -lm -g && ./a.out

#include <raylib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "atari2600emulator.h"

// #define RAYGUI_IMPLEMENTATION
// #include "raygui.h"


TV tv = { .x=160, .y=0 };

bool is_debug_mode;
bool is_game_paused = true;
int frames_so_far = 0;
int color_clocks_this_frame = 0;
unsigned long long color_clocks_in_total = 0;

Color rgbaToRaylib( int rgba ) {
    return (Color) {
            rgba>>24 & 0xFF,
            rgba>>16 & 0xFF,
            rgba>>8 & 0xFF,
            rgba & 0xFF,
    };
}


int prepare_game( char *gamefile_path, int TV_standard, int cartridge_type, int Input_mode, bool Is_debug_mode ) {
    int fd = open( gamefile_path, O_RDONLY);
    if (fd == -1) {
        perror("Can't open the rom file.\n");
        return 1;
    }
    static uint8_t rom[1000 * 1000] = {0};
    int bytes_read = read(fd, rom, 1000*1000);
    if ( bytes_read<=0 || bytes_read % 0x800 ) {
        perror("File doesn't seem to be an Atari2600 rom.");
        return 1;
    }

    memset( &cartridge, 0, sizeof(Cartridge) );
    cartridge.raw = rom;
    cartridge.type = cartridge_type;
    cartridge.F8.current_bank = 1;
    tv.standard = TV_standard;
    input_mode = Input_mode;
    is_debug_mode = Is_debug_mode;

    if( is_debug_mode ) {
        InitWindow(1156, 673, "");
    }
    else if (TV_standard==TV_NTSC) {
        InitWindow(SCANLINE_WIDTH*4, 198*2, "");
        SetTargetFPS( 60 );
    }
    else { // PAL, SECAM
        InitWindow(SCANLINE_WIDTH*4, 228*2, "");
        SetTargetFPS( 50 );
    }
    
    cpu_go_to_reset_vector();
    memory_init_random();
    video_init_objects();
    audio_init();
    return 0;
}


void draw_frame(void) {
    if (tv.standard == TV_NTSC ) {
        for (int y= 37; y<37+198; y++) {
            for (int x = 0; x<SCANLINE_WIDTH; x++) {
                DrawRectangle(4*x, 2*(y-37), 4, 2, tv.pixels[y][x]);
            }
        }
    }
    else { // PAL, SECAM
        for (int y= 45; y<45+228; y++) {
            for (int x = 0; x<SCANLINE_WIDTH; x++) {
                DrawRectangle(4*x, 2*(y-45), 4, 2, tv.pixels[y][x]);
            }
        }
    }
}


extern Memory memory;

void draw_debug_frame(void) {
    static Texture2D template;
    if (!template.width) { // template is not initialized
        template = LoadTexture("/home/korsan/proj/atari2600emu/src/dbg_template.png");
    }
    static Font font;
    if (!font.glyphCount) {
        font = LoadFont("/home/korsan/proj/atari2600emu/src/GNU_Unifont_Modified.otf");
    }

    char buf[50] = {0};
    DrawTexture(template, 0, 0, BLUE);

    const int fontSize = 17;
    const int fontSpacing = 0;
    const Color fontColor = WHITE;

    // screen
    for (int y= 0; y<SCANLINE_COUNT; y++) {
        for (int x = 0; x<SCANLINE_WIDTH; x++) {
            DrawRectangle(2*x, y, 2, 1, tv.pixels[y][x]);
        }
    }
   

    for (int i = 0; i<128; i++) {
        const int x_padding = 5;
        int y_coord = 142 + 17 * (i / 16);
        int x_coord = 637 + 26 * (i % 16) + x_padding;
        sprintf(buf, "%02hhX", memory[i]);
        DrawTextEx(font, buf, (Vector2){x_coord, y_coord}, fontSize, fontSpacing, fontColor);
    }

    // sprintf(buf, "%01hhX", memory[i]);
    DrawTextEx(font, (player0 .collides_with_player1)   ?"1":"0", (Vector2){475,328}, fontSize, fontSpacing, fontColor );
    DrawTextEx(font, (player0 .collides_with_missile0)  ?"1":"0", (Vector2){448,328}, fontSize, fontSpacing, fontColor );
    DrawTextEx(font, (player0 .collides_with_missile1)  ?"1":"0", (Vector2){421,328}, fontSize, fontSpacing, fontColor );
    DrawTextEx(font, (player0 .collides_with_ball)      ?"1":"0", (Vector2){394,328}, fontSize, fontSpacing, fontColor );
    DrawTextEx(font, (player0 .collides_with_playfield) ?"1":"0", (Vector2){367,328}, fontSize, fontSpacing, fontColor );
    DrawTextEx(font, (player1 .collides_with_missile0)  ?"1":"0", (Vector2){448,348}, fontSize, fontSpacing, fontColor );
    DrawTextEx(font, (player1 .collides_with_missile1)  ?"1":"0", (Vector2){421,348}, fontSize, fontSpacing, fontColor );
    DrawTextEx(font, (player1 .collides_with_ball)      ?"1":"0", (Vector2){394,348}, fontSize, fontSpacing, fontColor );
    DrawTextEx(font, (player1 .collides_with_playfield) ?"1":"0", (Vector2){367,348}, fontSize, fontSpacing, fontColor );
    DrawTextEx(font, (missile0.collides_with_missile1)  ?"1":"0", (Vector2){421,368}, fontSize, fontSpacing, fontColor );
    DrawTextEx(font, (missile0.collides_with_ball)      ?"1":"0", (Vector2){394,368}, fontSize, fontSpacing, fontColor );
    DrawTextEx(font, (missile0.collides_with_playfield) ?"1":"0", (Vector2){367,368}, fontSize, fontSpacing, fontColor );
    DrawTextEx(font, (missile1.collides_with_ball)      ?"1":"0", (Vector2){394,388}, fontSize, fontSpacing, fontColor );
    DrawTextEx(font, (missile1.collides_with_playfield) ?"1":"0", (Vector2){367,388}, fontSize, fontSpacing, fontColor );
    DrawTextEx(font, (ball    .collides_with_playfield) ?"1":"0", (Vector2){367,408}, fontSize, fontSpacing, fontColor );

    // pc
    sprintf(buf, "%04hX", cpu.pc);
    DrawTextEx(font, buf, (Vector2){636, 5}, fontSize, fontSpacing, fontColor);
    // sp
    sprintf(buf, "%02hhX", cpu.s);
    DrawTextEx(font, buf, (Vector2){636, 25}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%03hhu", cpu.s);
    DrawTextEx(font, buf, (Vector2){691, 25}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%08hhb", cpu.s);
    DrawTextEx(font, buf, (Vector2){745, 25}, fontSize, fontSpacing, fontColor);
    // a
    sprintf(buf, "%02hhX", cpu.a);
    DrawTextEx(font, buf, (Vector2){636, 42}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%03hhu", cpu.a);
    DrawTextEx(font, buf, (Vector2){691, 42}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%08hhb", cpu.a);
    DrawTextEx(font, buf, (Vector2){745, 42}, fontSize, fontSpacing, fontColor);
    // x
    sprintf(buf, "%02hhX", cpu.x);
    DrawTextEx(font, buf, (Vector2){636, 59}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%03hhu", cpu.x);
    DrawTextEx(font, buf, (Vector2){691, 59}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%08hhb", cpu.x);
    DrawTextEx(font, buf, (Vector2){745, 59}, fontSize, fontSpacing, fontColor);
    // y
    sprintf(buf, "%02hhX", cpu.y);
    DrawTextEx(font, buf, (Vector2){636, 76}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%03hhu", cpu.y);
    DrawTextEx(font, buf, (Vector2){691, 76}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%08hhb", cpu.y);
    DrawTextEx(font, buf, (Vector2){745, 76}, fontSize, fontSpacing, fontColor);
    // flags
    sprintf(buf, "%c%c--%c%c%c%c", cpu.flag_n?'N':'n', 
                                   cpu.flag_v?'V':'v', 
                                   cpu.flag_d?'D':'d', 
                                   cpu.flag_i?'I':'i', 
                                   cpu.flag_z?'Z':'z', 
                                   cpu.flag_c?'C':'c' );
    DrawTextEx(font, buf, (Vector2){636,96}, fontSize, 9, fontColor);
    
    // player0
    sprintf(buf, "%c%c%c%c%c%c%c%c", player0.main_sprite[0]?'1':'0', 
                                   player0.main_sprite[1]?'1':'0', 
                                   player0.main_sprite[2]?'1':'0', 
                                   player0.main_sprite[3]?'1':'0', 
                                   player0.main_sprite[4]?'1':'0', 
                                   player0.main_sprite[5]?'1':'0',
                                   player0.main_sprite[6]?'1':'0',
                                   player0.main_sprite[7]?'1':'0' );
    DrawTextEx(font, buf, (Vector2){36,433}, fontSize, 9, fontColor);
    sprintf(buf, "%c%c%c%c%c%c%c%c", player0.vdel_sprite[0]?'1':'0', 
                                   player0.vdel_sprite[1]?'1':'0', 
                                   player0.vdel_sprite[2]?'1':'0', 
                                   player0.vdel_sprite[3]?'1':'0', 
                                   player0.vdel_sprite[4]?'1':'0', 
                                   player0.vdel_sprite[5]?'1':'0',
                                   player0.vdel_sprite[6]?'1':'0',
                                   player0.vdel_sprite[7]?'1':'0' );
    DrawTextEx(font, buf, (Vector2){36,454}, fontSize, 9, fontColor);
    sprintf(buf, "%c", player0.is_vertically_delayed?'1':'0' );
    DrawTextEx(font, buf, (Vector2){186,455}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%c", player0.is_mirrored?'1':'0' );
    DrawTextEx(font, buf, (Vector2){347,433}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%4d", player0.x );
    DrawTextEx(font, buf, (Vector2){226,432}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%1hhX", debug_nusiz0 );
    DrawTextEx(font, buf, (Vector2){311,453}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%1X", player0.hm_value );
    DrawTextEx(font, buf, (Vector2){311,432}, fontSize, fontSpacing, fontColor);
    
    // player1
    sprintf(buf, "%c%c%c%c%c%c%c%c", player1.main_sprite[0]?'1':'0', 
                                   player1.main_sprite[1]?'1':'0', 
                                   player1.main_sprite[2]?'1':'0', 
                                   player1.main_sprite[3]?'1':'0', 
                                   player1.main_sprite[4]?'1':'0', 
                                   player1.main_sprite[5]?'1':'0',
                                   player1.main_sprite[6]?'1':'0',
                                   player1.main_sprite[7]?'1':'0' );
    DrawTextEx(font, buf, (Vector2){36,481}, fontSize, 9, fontColor);
    sprintf(buf, "%c%c%c%c%c%c%c%c", player1.vdel_sprite[0]?'1':'0', 
                                   player1.vdel_sprite[1]?'1':'0', 
                                   player1.vdel_sprite[2]?'1':'0', 
                                   player1.vdel_sprite[3]?'1':'0', 
                                   player1.vdel_sprite[4]?'1':'0', 
                                   player1.vdel_sprite[5]?'1':'0',
                                   player1.vdel_sprite[6]?'1':'0',
                                   player1.vdel_sprite[7]?'1':'0' );
    DrawTextEx(font, buf, (Vector2){36,502}, fontSize, 9, fontColor);
    sprintf(buf, "%c", player1.is_vertically_delayed?'1':'0' );
    DrawTextEx(font, buf, (Vector2){186,503}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%c", player1.is_mirrored?'1':'0' );
    DrawTextEx(font, buf, (Vector2){347,481}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%4d", player1.x );
    DrawTextEx(font, buf, (Vector2){226,480}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%1hhX", debug_nusiz1 );
    DrawTextEx(font, buf, (Vector2){311,501}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%1X", player1.hm_value );
    DrawTextEx(font, buf, (Vector2){311,480}, fontSize, fontSpacing, fontColor);
    
    // missile0
    sprintf(buf, "%c", missile0.sprite[0]?'1':'0' );
    DrawTextEx(font, buf, (Vector2){36,529}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%c", missile0.is_locked_on_player?'1':'0' );
    DrawTextEx(font, buf, (Vector2){285,529}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%d", (int)log2(missile0.size) );
    DrawTextEx(font, buf, (Vector2){249,529}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%4d", missile0.x );
    DrawTextEx(font, buf, (Vector2){107,528}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%1X", missile0.hm_value );
    DrawTextEx(font, buf, (Vector2){178,528}, fontSize, fontSpacing, fontColor);
    

    // missile1
    sprintf(buf, "%c", missile1.sprite[0]?'1':'0' );
    DrawTextEx(font, buf, (Vector2){36,549}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%c", missile1.is_locked_on_player?'1':'0' );
    DrawTextEx(font, buf, (Vector2){285,549}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%d", (int)log2(missile1.size) );
    DrawTextEx(font, buf, (Vector2){249,549}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%4d", missile1.x );
    DrawTextEx(font, buf, (Vector2){107,548}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%1X", missile1.hm_value );
    DrawTextEx(font, buf, (Vector2){178,548}, fontSize, fontSpacing, fontColor);
    
    // ball
    sprintf(buf, "%c", ball.main_sprite[0]?'1':'0' );
    DrawTextEx(font, buf, (Vector2){36,569}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%c", ball.vdel_sprite[0]?'1':'0' );
    DrawTextEx(font, buf, (Vector2){36,590}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%d", (int)log2(ball.size) );
    DrawTextEx(font, buf, (Vector2){249,569}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%c", ball.is_vertically_delayed?'1':'0' );
    DrawTextEx(font, buf, (Vector2){67,591}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%4d", ball.x );
    DrawTextEx(font, buf, (Vector2){107,568}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%1X", ball.hm_value );
    DrawTextEx(font, buf, (Vector2){178,568}, fontSize, fontSpacing, fontColor);
    
    
    // playfield
    sprintf(buf, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
        playfield.sprite[0]?'1':'0', 
        playfield.sprite[1]?'1':'0', 
        playfield.sprite[2]?'1':'0', 
        playfield.sprite[3]?'1':'0', 
        playfield.sprite[4]?'1':'0', 
        playfield.sprite[5]?'1':'0',
        playfield.sprite[6]?'1':'0',
        playfield.sprite[7]?'1':'0', 
        playfield.sprite[8]?'1':'0', 
        playfield.sprite[9]?'1':'0', 
        playfield.sprite[10]?'1':'0', 
        playfield.sprite[11]?'1':'0', 
        playfield.sprite[12]?'1':'0',
        playfield.sprite[13]?'1':'0',
        playfield.sprite[14]?'1':'0', 
        playfield.sprite[15]?'1':'0', 
        playfield.sprite[16]?'1':'0', 
        playfield.sprite[17]?'1':'0', 
        playfield.sprite[18]?'1':'0', 
        playfield.sprite[19]?'1':'0' );
    DrawTextEx(font, buf, (Vector2){42,627}, fontSize, 9, fontColor);

    // playfield is reflected
    sprintf(buf, "%c", playfield.is_reflected?'1':'0' );
    DrawTextEx(font, buf, (Vector2){36,649}, fontSize, fontSpacing, fontColor);
    // playfield is in score mode
    sprintf(buf, "%c", video_priority==VIDEO_PRIORITY_SCORE ?'1':'0' );
    DrawTextEx(font, buf, (Vector2){137,649}, fontSize, fontSpacing, fontColor);
    // playfield is in priority mode
    sprintf(buf, "%c", video_priority==VIDEO_PRIORITY_HIGH ?'1':'0' );
    DrawTextEx(font, buf, (Vector2){220,649}, fontSize, fontSpacing, fontColor);

    // vsync
    sprintf(buf, "%c", vsync?'1':'0' );
    DrawTextEx(font, buf, (Vector2){348,281}, fontSize, fontSpacing, fontColor);

    // vblank0
    sprintf(buf, "%c", vblank?'1':'0' );
    DrawTextEx(font, buf, (Vector2){441,280}, fontSize, fontSpacing, fontColor);
    // vblank6
    sprintf(buf, "%c", vblank6?'1':'0' );
    DrawTextEx(font, buf, (Vector2){831,540}, fontSize, fontSpacing, fontColor);
    // vblank7
    sprintf(buf, "%c", vblank7?'1':'0' );
    DrawTextEx(font, buf, (Vector2){831,560}, fontSize, fontSpacing, fontColor);

    // inpt0..inpt5
    sprintf(buf, "%02hhX", inpt0);
    DrawTextEx(font, buf, (Vector2){867, 481}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%02hhX", inpt1);
    DrawTextEx(font, buf, (Vector2){867, 498}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%02hhX", inpt2);
    DrawTextEx(font, buf, (Vector2){1042, 481}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%02hhX", inpt3);
    DrawTextEx(font, buf, (Vector2){1042, 498}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%02hhX", inpt4);
    DrawTextEx(font, buf, (Vector2){867, 515}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%02hhX", inpt5);
    DrawTextEx(font, buf, (Vector2){1042, 515}, fontSize, fontSpacing, fontColor);
    
    // swacnt
    sprintf(buf, "%08hhb", swacnt);
    DrawTextEx(font, buf, (Vector2){654, 348}, fontSize, 9, fontColor);
    // swbcnt
    sprintf(buf, "%08hhb", swbcnt);
    DrawTextEx(font, buf, (Vector2){654, 420}, fontSize, 9, fontColor);
    // swcha (written value)
    sprintf(buf, "%08hhb", swcha);
    DrawTextEx(font, buf, (Vector2){654, 328}, fontSize, 9, fontColor);
    // swcha (read value)
    extern uint8_t input_SWCHA_read ( void );
    sprintf(buf, "%08hhb", input_SWCHA_read() );
    DrawTextEx(font, buf, (Vector2){654, 368}, fontSize, 9, fontColor);
    // swchb (written value)
    DrawTextEx(font, "--------", (Vector2){654, 400}, fontSize, 9, fontColor);
    // swchb (read value)
    extern uint8_t input_SWCHB_read ( void );
    sprintf(buf, "%08hhb", input_SWCHB_read() );
    DrawTextEx(font, buf, (Vector2){654, 440}, fontSize, 9, fontColor);
    
    // frames
    sprintf(buf, "%d", frames_so_far );
    DrawTextEx(font, buf, (Vector2){538, 6}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%d", color_clocks_this_frame/3 );
    DrawTextEx(font, buf, (Vector2){405, 5}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%lld", color_clocks_in_total/3 );
    DrawTextEx(font, buf, (Vector2){378, 68}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%d", tv.x>=160 ? tv.y+1 : tv.y );
    DrawTextEx(font, buf, (Vector2){521, 26}, fontSize, fontSpacing, fontColor);
    int pixel_pos = tv.x<160 ? tv.x : tv.x-228;
    sprintf(buf, "%d", pixel_pos );
    DrawTextEx(font, buf, (Vector2){556, 68}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%d", pixel_pos+68 );
    DrawTextEx(font, buf, (Vector2){556, 89}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%d", cpu.cycle );
    DrawTextEx(font, buf, (Vector2){745, 5}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%lld", color_clocks_in_total%3 );
    DrawTextEx(font, buf, (Vector2){913, 5}, fontSize, fontSpacing, fontColor);
    
    // timer
    sprintf(buf, "x" );
    if (timer_prescaler==1)
        DrawTextEx(font, buf, (Vector2){654, 485}, fontSize, fontSpacing, fontColor);
    if (timer_prescaler==8)
        DrawTextEx(font, buf, (Vector2){654, 502}, fontSize, fontSpacing, fontColor);
    if (timer_prescaler==64)
        DrawTextEx(font, buf, (Vector2){654, 519}, fontSize, fontSpacing, fontColor);
    if (timer_prescaler==1024)
        DrawTextEx(font, buf, (Vector2){654, 536}, fontSize, fontSpacing, fontColor);
    
    sprintf(buf, "%x", timer_primary_value );
    DrawTextEx(font, buf, (Vector2){654, 562}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%x", timer_secondary_value );
    DrawTextEx(font, buf, (Vector2){654, 579}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%c%c", timer_underflow_bit7?'1':'0', timer_underflow_bit6?'1':'0' );
    DrawTextEx(font, buf, (Vector2){654, 596}, fontSize, fontSpacing, fontColor);
    
    // audio
    sprintf(buf, "%2x", left.frequency );
    DrawTextEx(font, buf, (Vector2){373, 167}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%1x", left.mode );
    DrawTextEx(font, buf, (Vector2){382, 189}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%1x", left.volume );
    DrawTextEx(font, buf, (Vector2){382, 211}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%2x", right.frequency );
    DrawTextEx(font, buf, (Vector2){400, 167}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%1x", right.mode );
    DrawTextEx(font, buf, (Vector2){400, 189}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%1x", right.volume );
    DrawTextEx(font, buf, (Vector2){400, 211}, fontSize, fontSpacing, fontColor);
    
    
}

void tick_atari(void) {

    static bool prev_vsync_value;
        
    // determine what pixel to draw
    int rgba = video_calculate_pixel(tv.x);
    Color color = rgbaToRaylib(rgba);
    if (tv.x < SCANLINE_WIDTH && tv.y < SCANLINE_COUNT) {
        tv.pixels[tv.y][tv.x] = color;
    }

    // collisions
    video_calculate_collisions(tv.x);
    
    // move to the next pixel on the screen
    tv.x++;
    color_clocks_this_frame++;
    
    
    if (tv.x==228) {
        tv.x=0; tv.y++;
        player0.is_reset_this_scanline = false;
        player1.is_reset_this_scanline = false;
        missile0.is_reset_this_scanline = false;
        missile1.is_reset_this_scanline = false;
        ball.is_reset_this_scanline = false;
    }
    

    // vsync
    prev_vsync_value = vsync;

    // execute an instruction when cpu is ready
    if (color_clocks_in_total % 3 == 2) {
        timer_tick();
        if (!wsync) {
            cpu_run_for_one_machine_cycle();
        }
    }
    tick_delayed_writes();

    if (tv.x==160 && wsync) {
        wsync = false;
    }

    color_clocks_in_total++;

    // tv.y==SCANLINE_COUNT part is optional but it emulates CRT TV rolling effect
    if( (prev_vsync_value && !vsync) || tv.y==SCANLINE_COUNT ) {
        tv.y=0;
        frames_so_far++;
        color_clocks_this_frame = 0;
    }

}

int main(void) {

    

    int er = prepare_game("/home/korsan/proj/atari2600emu/atari_tests/example19.a26", TV_NTSC, CARTRIDGE_4K, INPUT_JOYSTICK, false); // normally CARTRIDGE_4K
    if (er) return 1;


    const int TICKS_PER_FRAME = tv.standard == TV_NTSC ? NTSC_TICKS_PER_FRAME : PAL_TICKS_PER_FRAME;

    while ( !WindowShouldClose()) {

        BeginDrawing();

            if ( is_debug_mode ) {
                // advance one color cycle
                if( IsKeyPressed(KEY_ONE) || IsKeyPressedRepeat(KEY_ONE) ) {
                    tick_atari();
                }
                // advance one instruction
                else if( IsKeyPressed(KEY_TWO) || IsKeyPressedRepeat(KEY_TWO) ) {
                    do {
                        tick_atari();
                    } while( !(color_clocks_in_total%3==0 && cpu.cycle == 1 && !wsync) );
                }
                // advance one scanline
                else if( IsKeyPressed(KEY_THREE) || IsKeyPressedRepeat(KEY_THREE) ) {
                    do {
                        tick_atari();
                    } while( !(tv.x == 160) );
                }
                // advance one frame
                else if( IsKeyPressed(KEY_FOUR) || IsKeyPressedRepeat(KEY_FOUR) ) {
                    do {
                        tick_atari();
                    } while( !(color_clocks_this_frame==0) );
                }
                // pause - resume
                else if (IsKeyPressed(KEY_FIVE) || IsKeyPressedRepeat(KEY_FIVE) || !is_game_paused) {
                    for (int i = 0; i<TICKS_PER_FRAME; i++) {
                        tick_atari();
                    }
                    if (IsKeyPressed(KEY_FIVE) || IsKeyPressedRepeat(KEY_FIVE)) {
                        is_game_paused = !is_game_paused;
                    }
                }
                
                draw_debug_frame();
                input_register_inputs();
            }
            else {
                for (int i = 0; i<TICKS_PER_FRAME; i++) {
                    tick_atari();
                }
                draw_frame();
                input_register_inputs();
            }
            
            
            
            // GuiToggle((Rectangle){ WINDOW_WIDTH_PX*2-35, 5, 30, 30 }, "#142#", &settings_window_is_shown);
            
            // if (settings_window_is_shown) {
            //     int result = GuiMessageBox((Rectangle){ 85, 70, 250, 100 },
            //         "#191#Message Box", "Hi! This is a message!", "Nice;Cool");

            // }
            // DrawLine(0, 37*2, WINDOW_WIDTH_PX*2, 37*2, RED);
            // DrawLine(0, (37+192)*2, WINDOW_WIDTH_PX*2, (37+192)*2, RED);
        EndDrawing();
    }

    StopAudioStream(stream);
    UnloadAudioStream(stream);   // Close raw audio stream and delete buffers from RAM
    CloseAudioDevice();

    CloseWindow();

    return 0;
}


// horizontal: 160+68 = 228
// vertical: 192+30+3+37=262 ntsc
//           228+36+3+45=312 pal


// calcpixel(x) depends on
/*
hmvoeisactive
tvstd
videopriority



*/