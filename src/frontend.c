// gcc src/frontend.c src/cpu.c src/memory.c src/video.c src/timer.c src/audio.c src/input.c -lraylib -lm -g && ./a.out

#include <raylib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include "atari2600emulator.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"


const int windowWidthPx = 160;
const int windowHeightPx = 262;
int tv_x;
int tv_y;

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
    tv_standard = TV_standard; // cant come up with a better way of doing this

    cpu_go_to_reset_vector();
    return 0;
}

void draw_frame(void) {

    static int cpuCooldown = 0;
    static bool prev_vsync_value;
    for (int i = 0; i<windowWidthPx*windowHeightPx; i++) {
        
        // determine what pixel to draw and draw it
        int rgba = video_calculate_pixel(tv_x);
        Color color = rgbaToRaylib(rgba);
        DrawRectangle(2*tv_x, 2*tv_y, 2, 2, color);

        // collisions
        video_calculate_collisions(tv_x);
        
        // move to the next pixel on the screen
        tv_x++;
        if (tv_x==160) {
            wsync = false;
            // hmove_is_active = false;
        }
        else if (tv_x==228) {
            tv_x=0; tv_y++;
        }
        if( prev_vsync_value && !vsync ) {
            tv_x=0; tv_y=0;
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
}

int main(void) {

    
    
    InitWindow(windowWidthPx*2, windowHeightPx*2, "");
    int er = prepare_game("/home/korsan/proj/atari2600emu/atari_tests/missiles.a26", TV_NTSC, CARTRIDGE_4K);
    if (er) return 1;

    SetTargetFPS(60);

    bool settings_window_is_shown = false;
    input_mode = INPUT_JOYSTICK;

    while ( !WindowShouldClose()) {

        BeginDrawing();

            // DrawRectangle(0, 0, windowWidthPx*2, windowHeightPx*2, BLACK);
            draw_frame();
            input_register_inputs();
            
            
            // GuiToggle((Rectangle){ windowWidthPx*2-35, 5, 30, 30 }, "#142#", &settings_window_is_shown);
            
            // if (settings_window_is_shown) {
            //     int result = GuiMessageBox((Rectangle){ 85, 70, 250, 100 },
            //         "#191#Message Box", "Hi! This is a message!", "Nice;Cool");

            // }
            // DrawLine(0, 37*2, windowWidthPx*2, 37*2, RED);
            // DrawLine(0, (37+192)*2, windowWidthPx*2, (37+192)*2, RED);
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