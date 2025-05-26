// clang src/frontend.c src/cpu.c src/memory.c src/video.c src/timer.c src/audio.c src/input.c -lraylib -lm -g && ./a.out

#include <raylib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "atari2600emulator.h"

// #define RAYGUI_IMPLEMENTATION
// #include "raygui.h"


TV tv = { .x=160, .y=0, .standard=TV_NTSC };

int frames_so_far = 0;
int color_clocks_this_frame = 0;
int cpuCooldown = 2;

Color rgbaToRaylib( int rgba ) {
    return (Color) {
            rgba>>24 & 0xFF,
            rgba>>16 & 0xFF,
            rgba>>8 & 0xFF,
            rgba & 0xFF,
    };
}


int prepare_game( char *gamefile_path, int TV_standard, int cartridge_type ) {
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
    tv.standard = TV_standard;

    cpu_go_to_reset_vector();
    memory_init_random();
    video_init_objects();
    return 0;
}


void draw_frame(void) {
    for (int y= 0; y<WINDOW_HEIGHT_PX; y++) {
        for (int x = 0; x<160; x++) {
            DrawRectangle(4*x, 2*y, 4, 2, tv.pixels[y][x]);
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
    for (int y= 0; y<WINDOW_HEIGHT_PX; y++) {
        for (int x = 0; x<160; x++) {
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
    sprintf(buf, "%c%c%c%c%c%c%c%c", player0.sprite[0]?'1':'0', 
                                   player0.sprite[1]?'1':'0', 
                                   player0.sprite[2]?'1':'0', 
                                   player0.sprite[3]?'1':'0', 
                                   player0.sprite[4]?'1':'0', 
                                   player0.sprite[5]?'1':'0',
                                   player0.sprite[6]?'1':'0',
                                   player0.sprite[7]?'1':'0' );
    DrawTextEx(font, buf, (Vector2){36,433}, fontSize, 9, fontColor);
    sprintf(buf, "%08hhb", player0.value_that_will_become_sprite);
    DrawTextEx(font, buf, (Vector2){36,454}, fontSize, 9, fontColor);
    sprintf(buf, "%c", player0.is_vertically_delayed?'1':'0' );
    DrawTextEx(font, buf, (Vector2){186,455}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%c", player0.is_mirrored?'1':'0' );
    DrawTextEx(font, buf, (Vector2){347,433}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%4d", player0.x );
    DrawTextEx(font, buf, (Vector2){226,432}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%1hhX", debug_nusiz0 );
    DrawTextEx(font, buf, (Vector2){311,453}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%1hhX", debug_hmp0 );
    DrawTextEx(font, buf, (Vector2){311,432}, fontSize, fontSpacing, fontColor);
    
    // player1
    sprintf(buf, "%c%c%c%c%c%c%c%c", player1.sprite[0]?'1':'0', 
                                   player1.sprite[1]?'1':'0', 
                                   player1.sprite[2]?'1':'0', 
                                   player1.sprite[3]?'1':'0', 
                                   player1.sprite[4]?'1':'0', 
                                   player1.sprite[5]?'1':'0',
                                   player1.sprite[6]?'1':'0',
                                   player1.sprite[7]?'1':'0' );
    DrawTextEx(font, buf, (Vector2){36,481}, fontSize, 9, fontColor);
    sprintf(buf, "%08hhb", player1.value_that_will_become_sprite);
    DrawTextEx(font, buf, (Vector2){36,502}, fontSize, 9, fontColor);
    sprintf(buf, "%c", player1.is_vertically_delayed?'1':'0' );
    DrawTextEx(font, buf, (Vector2){186,503}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%c", player1.is_mirrored?'1':'0' );
    DrawTextEx(font, buf, (Vector2){347,481}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%4d", player1.x );
    DrawTextEx(font, buf, (Vector2){226,480}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%1hhX", debug_nusiz1 );
    DrawTextEx(font, buf, (Vector2){311,501}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%1hhX", debug_hmp1 );
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
    sprintf(buf, "%1hhX", debug_hmm0 );
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
    sprintf(buf, "%1hhX", debug_hmm1 );
    DrawTextEx(font, buf, (Vector2){178,548}, fontSize, fontSpacing, fontColor);
    
    // ball
    sprintf(buf, "%c", ball.sprite[0]?'1':'0' );
    DrawTextEx(font, buf, (Vector2){36,569}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%d", (int)log2(ball.size) );
    DrawTextEx(font, buf, (Vector2){249,569}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%c", ball.is_vertically_delayed?'1':'0' );
    DrawTextEx(font, buf, (Vector2){67,591}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%4d", ball.x );
    DrawTextEx(font, buf, (Vector2){107,568}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%1hhX", debug_hmbl );
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
    DrawTextEx(font, buf, (Vector2){9,311}, fontSize, fontSpacing, fontColor);

    // vblank0
    sprintf(buf, "%c", vblank?'1':'0' );
    DrawTextEx(font, buf, (Vector2){102,310}, fontSize, fontSpacing, fontColor);
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
    sprintf(buf, "%d", tv.y );
    DrawTextEx(font, buf, (Vector2){521, 26}, fontSize, fontSpacing, fontColor);
    int pixel_pos = tv.x<160 ? tv.x : tv.x-228;
    sprintf(buf, "%d", pixel_pos );
    DrawTextEx(font, buf, (Vector2){556, 68}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%d", pixel_pos+68 );
    DrawTextEx(font, buf, (Vector2){556, 89}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%d", cpu.cycle );
    DrawTextEx(font, buf, (Vector2){745, 5}, fontSize, fontSpacing, fontColor);
    sprintf(buf, "%d", cpuCooldown );
    DrawTextEx(font, buf, (Vector2){913, 5}, fontSize, fontSpacing, fontColor);
    
    
    
}

void tick_atari(void) {

    static bool prev_vsync_value;
        
    // determine what pixel to draw
    int rgba = video_calculate_pixel(tv.x);
    Color color = rgbaToRaylib(rgba);
    if (tv.x < 160 && tv.y < 262) {
        tv.pixels[tv.y][tv.x] = color;
    }

    // collisions
    video_calculate_collisions(tv.x);
    
    // move to the next pixel on the screen
    tv.x++;
    color_clocks_this_frame++;
    
    if (tv.x==160 && wsync) {
        wsync = false;
        cpuCooldown = 3;
    }
    else if (tv.x==228) {
        tv.x=0; tv.y++;
    }
    if( prev_vsync_value && !vsync ) {
        tv.x=0; tv.y=0;
        frames_so_far++;
        color_clocks_this_frame = 0;
    }

    // vsync
    prev_vsync_value = vsync;

    // execute an instruction when cpu is ready
    if (cpuCooldown==0 && !wsync) {
        cpu_run_for_one_machine_cycle();
        cpuCooldown = 3;
    }

    cpuCooldown--;
    if (cpuCooldown<0) cpuCooldown = 0;

}

int main(void) {

    // printf("%d\n", (int)log2(0) );
    // printf("%d\n", (int)log2(1.5) );
    // printf("%d\n", (int)log2(-1) );

    // return 0;



    #ifdef ATARI_DEBUG_MODE
        InitWindow(1156, 673, "");
    #else
        InitWindow(WINDOW_WIDTH_PX*4, WINDOW_HEIGHT_PX*2, "");
    #endif

    int er = prepare_game("/home/korsan/proj/atari2600emu/atari_tests/example11.a26", TV_NTSC, CARTRIDGE_4K);
    if (er) return 1;

    SetTargetFPS(60);

    input_mode = INPUT_JOYSTICK;

    // RenderTexture2D target = LoadRenderTexture(virtualWidth, virtualHeight);


    while ( !WindowShouldClose()) {

        BeginDrawing();

            #ifdef ATARI_DEBUG_MODE
                // advance one color cycle
                if( IsKeyPressed(KEY_ONE) || IsKeyPressedRepeat(KEY_ONE) ) {
                    tick_atari();
                }
                // advance one instruction
                else if( IsKeyPressed(KEY_TWO) || IsKeyPressedRepeat(KEY_TWO) ) {
                    do {
                        tick_atari();
                    } while( !(cpuCooldown == 2 && cpu.cycle == 1 && !wsync) );
                }
                // advance one scanline
                else if( IsKeyPressed(KEY_THREE) || IsKeyPressedRepeat(KEY_THREE) ) {
                    do {
                        tick_atari();
                    } while( !(tv.x == 0) ); // cooldown might not be 2 here
                }
                // advance one frame
                else if( IsKeyPressed(KEY_FOUR) || IsKeyPressedRepeat(KEY_FOUR) ) {
                    do {
                        tick_atari();
                    } while( !(tv.y == 0 && tv.x == 0) );
                }
                
                // for (int i = 0; i<WINDOW_WIDTH_PX*WINDOW_HEIGHT_PX; i++) {
                //     tick_atari();
                // }
                draw_debug_frame();
                input_register_inputs();
            #else
                for (int i = 0; i<62000; i++) {
                    tick_atari();
                }
                draw_frame();
                input_register_inputs();
            #endif
            
            
            
            // GuiToggle((Rectangle){ WINDOW_WIDTH_PX*2-35, 5, 30, 30 }, "#142#", &settings_window_is_shown);
            
            // if (settings_window_is_shown) {
            //     int result = GuiMessageBox((Rectangle){ 85, 70, 250, 100 },
            //         "#191#Message Box", "Hi! This is a message!", "Nice;Cool");

            // }
            // DrawLine(0, 37*2, WINDOW_WIDTH_PX*2, 37*2, RED);
            // DrawLine(0, (37+192)*2, WINDOW_WIDTH_PX*2, (37+192)*2, RED);
        EndDrawing();
    }

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