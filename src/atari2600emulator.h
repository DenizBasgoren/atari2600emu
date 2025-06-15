
#ifndef _ATARI2600EMULATOR_HEADER_
#define _ATARI2600EMULATOR_HEADER_

    #include <stdint.h>
    #include <stdbool.h>
    #include <raylib.h>


    typedef struct {
        uint8_t a;
        uint8_t x;
        uint8_t y;
        uint8_t s;
        uint16_t pc;
        bool flag_c;
        bool flag_z;
        bool flag_i;
        bool flag_d;
        bool flag_v;
        bool flag_n;
        bool is_killed;
        uint8_t opcode; // current opcode
        int cycle; // current machine cycle
        uint8_t temp1, temp2, temp3, temp4;
    } Cpu;

    extern Cpu cpu;

    typedef struct {
        int type;
        uint8_t *raw;

        union {
            struct {
                int current_bank;
            } F8;
            struct {
                int current_bank;
            } F6;
            struct {
                int current_bank;
            } F4;
            struct {
                uint8_t extra_ram[0x400];
            } CV;
            struct {
                int current_bank;
            } FE;
            struct {
                int current_lower_bank;
                int current_middle_bank;
                int current_upper_bank;
            } E0;
            struct {
                int current_bank;
                uint8_t extra_ram[256];
            } FA;
            struct {
                int current_lower_bank;
                int current_upper_bank;
                uint8_t extra_ram[2048];
            } E7;
            struct {
                int current_bank;
            } F0;
        };
    } Cartridge;

    // 13-bit address space (8 kilobytes)
    typedef uint8_t Memory [ 128 ];

    enum { TV_NTSC, TV_PAL, TV_SECAM, TV_MONOCHROME };
    enum { VIDEO_PRIORITY_NORMAL, VIDEO_PRIORITY_SCORE, VIDEO_PRIORITY_HIGH };
    enum { CARTRIDGE_2K, CARTRIDGE_4K, CARTRIDGE_CV, CARTRIDGE_F8, CARTRIDGE_F6, CARTRIDGE_F4, CARTRIDGE_FE,
           CARTRIDGE_E0, CARTRIDGE_FA, CARTRIDGE_E7, CARTRIDGE_F0 };
    enum { INPUT_JOYSTICK, INPUT_PADDLE, INPUT_KEYPAD };

    void memory_init_random( void );
    uint8_t memory_read( uint16_t address );
    void memory_write( uint16_t address, uint8_t value );
    uint8_t memory_access_cartridge( uint16_t address, uint8_t value );

    extern Cartridge cartridge;


    uint8_t cpu_get_flags_as_integer( void );
    void cpu_set_flags_from_integer( uint8_t value);
    void cpu_update_nz_flags( const uint8_t value);
    void cpu_update_cv_flags( const uint8_t arg1, const uint8_t arg2, const bool carry);
    void cpu_update_cv_flags_subtraction( const uint8_t arg1, const uint8_t arg2, const bool carry);
    void cpu_push_to_stack( uint8_t value);
    uint8_t cpu_pop_from_stack( void );
    void cpu_add_in_bcd_mode( uint8_t arg2 );
    void cpu_subtract_in_bcd_mode( uint8_t arg2 );
    void cpu_run_for_one_machine_cycle(void);
    void cpu_go_to_reset_vector();

    uint8_t timer_INTIM_read ( void );
    uint8_t timer_INSTAT_read ( void );
    void timer_TIM1T_write  ( uint8_t value );
    void timer_TIM8T_write  ( uint8_t value );
    void timer_TIM64T_write ( uint8_t value );
    void timer_T1024T_write ( uint8_t value );
    void timer_tick ( void );
    extern unsigned char timer_primary_value;
    extern int timer_secondary_value;
    extern int timer_prescaler;
    extern bool timer_underflow_bit7;
    extern bool timer_underflow_bit6;


    typedef struct {
        int x; // 0..160
        int hm_value; // 0..15
        bool main_sprite[8];
        bool vdel_sprite[8];
        int color; // rgba 32bit
        bool is_mirrored;
        bool collides_with_player1;
        bool collides_with_missile0;
        bool collides_with_missile1;
        bool collides_with_ball;
        bool collides_with_playfield;
        bool is_vertically_delayed;
        bool is_reset_this_scanline;
        // uint8_t value_that_will_become_sprite;
        int copies; // 1 2 3
        int distance_between_copies; // 16 32 64
        int size; // 8 16 32
    } Player0;

    typedef struct {
        int x; // 0..160
        int hm_value; // 0..15
        bool main_sprite[8];
        bool vdel_sprite[8];
        int color; // rgba 32bit
        bool is_mirrored;
        bool collides_with_missile0;
        bool collides_with_missile1;
        bool collides_with_ball;
        bool collides_with_playfield;
        bool is_vertically_delayed;
        bool is_reset_this_scanline;
        // uint8_t value_that_will_become_sprite;
        int copies; // 1 2 3
        int distance_between_copies; // 16 32 64
        int size; // 8 16 32
    } Player1;

    typedef struct {
        int x; // 0..160
        int hm_value; // 0..15
        int size; // 1 2 4 8
        bool sprite[1];
        int color; // rgba 32bit
        bool collides_with_missile1;
        bool collides_with_ball;
        bool collides_with_playfield;
        bool is_reset_this_scanline;
        int copies; // 1 2 3
        int distance_between_copies; // 16 32 64
        bool is_locked_on_player;
    } Missile0;

    typedef struct {
        int x; // 0..160
        int hm_value; // 0..15
        int size; // 1 2 4 8
        bool sprite[1];
        int color; // rgba 32bit
        bool collides_with_ball;
        bool collides_with_playfield;
        bool is_reset_this_scanline;
        int copies; // 1 2 3
        int distance_between_copies; // 16 32 64
        bool is_locked_on_player;
    } Missile1;

    typedef struct {
        int x; // 0..160
        int hm_value; // 0..15
        int size; // 1 2 4 8
        bool main_sprite[1];
        bool vdel_sprite[1];
        int color; // rgba 32bit
        bool collides_with_playfield;
        bool is_vertically_delayed;
        bool is_reset_this_scanline;
        // uint8_t value_that_will_become_sprite;
    } Ball;

    typedef struct {
        bool is_reflected;
        bool sprite[20];
        int color_left_half; // rgba 32bit
        int color_right_half; // rgba 32bit
        int color_background; // rgba 32bit
    } Playfield;

    extern Player0 player0;
    extern Player1 player1;
    extern Missile0 missile0;
    extern Missile1 missile1;
    extern Ball ball;
    extern Playfield playfield;

    extern bool vsync;
    extern bool vblank;
    extern bool wsync;
    extern int video_priority;
    extern uint8_t debug_nusiz0, debug_nusiz1;


    #define SCANLINE_WIDTH 160
    #define SCANLINE_COUNT 312

    #define NTSC_TICKS_PER_FRAME 59659 /*   3.579545 MHz / 60    */
    #define PAL_TICKS_PER_FRAME 70937 /*   3.546894 MHz / 50    */

    typedef Color Scanline[SCANLINE_WIDTH];

    typedef struct {
        Scanline pixels[SCANLINE_COUNT];
        int x;
        int y;
        int standard;
    } TV;

    extern TV tv;

    void video_init_objects( void );
    void video_VSYNC_write ( uint8_t value );
    void video_VBLANK_write ( uint8_t value );
    void video_WSYNC_write ( void );
    void video_RSYNC_write ( void );
    void video_NUSIZ0_write ( uint8_t value );
    void video_NUSIZ1_write ( uint8_t value );
    void video_COLUP0_write ( uint8_t value );
    void video_COLUP1_write ( uint8_t value );
    void video_COLUPF_write ( uint8_t value );
    void video_COLUBK_write ( uint8_t value );
    void video_CTRLPF_write ( uint8_t value );
    void video_REFP0_write ( uint8_t value );
    void video_REFP1_write ( uint8_t value );
    void video_PF0_write ( uint8_t value );
    void video_PF1_write ( uint8_t value );
    void video_PF2_write ( uint8_t value );
    void video_RESP0_write ( void );
    void video_RESP1_write ( void );
    void video_RESM0_write ( void );
    void video_RESM1_write ( void );
    void video_RESBL_write ( void );
    void video_GRP0_write ( uint8_t value );
    void video_GRP1_write ( uint8_t value );
    void video_ENAM0_write ( uint8_t value );
    void video_ENAM1_write ( uint8_t value );
    void video_ENABL_write ( uint8_t value );
    void video_HMP0_write ( uint8_t value );
    void video_HMP1_write ( uint8_t value );
    void video_HMM0_write ( uint8_t value );
    void video_HMM1_write ( uint8_t value );
    void video_HMBL_write ( uint8_t value );
    void video_VDELP0_write ( uint8_t value );
    void video_VDELP1_write ( uint8_t value );
    void video_VDELBL_write ( uint8_t value );
    void video_RESMP0_write ( uint8_t value );
    void video_RESMP1_write ( uint8_t value );
    void video_HMOVE_write ( void );
    void video_HMCLR_write ( void );
    void video_CXCLR_write ( void );
    uint8_t video_CXM0P_read ( void );
    uint8_t video_CXM1P_read ( void );
    uint8_t video_CXP0FB_read ( void );
    uint8_t video_CXP1FB_read ( void );
    uint8_t video_CXM0FB_read ( void );
    uint8_t video_CXM1FB_read ( void );
    uint8_t video_CXBLPF_read ( void );
    uint8_t video_CXPPMM_read ( void );
    void video_calculate_collisions( int x);
    int video_calculate_pixel( int x );
    int video_decode_color( uint8_t color, int tv_standard );
    bool video_playfield_occupies_x( int x );
    bool video_ball_occupies_x( int x );
    bool video_missile0_occupies_x( int x );
    bool video_missile1_occupies_x( int x );
    void video_calculate_effective_player_sprite( bool final_sprite[32], bool init_sprite[8], int size, bool is_mirrored );
    bool video_player0_occupies_x( int x);
    bool video_player1_occupies_x( int x);
    int video_player0_effective_x ( void);
    int video_player1_effective_x ( void);
    int video_missile0_effective_x ( void);
    int video_missile1_effective_x ( void);
    int video_ball_effective_x ( void);


    typedef struct {
        bool up_pressed;
        bool down_pressed;
        bool right_pressed;
        bool left_pressed;
        bool btn_pressed;
        bool btn2_pressed;
        bool btn3_pressed;
    } Joystick;
    /*
        W             I
        A S D F       J K L ;
            C V           . /
    */

    typedef struct {
        bool btn_pressed;
        int pod_angle; // -135..+135
        float pod_loading_percent; // 0..100
    } Paddle;
    /*
        A S D F       J K L ;
            C V           . /
    */

    typedef struct {
        bool key_pressed[4][3];
    } Keypad;
    /*
        1 2 3         7 8 9
        Q W E         U I O
        A S D         J K L
        Z X C         M , .
    */

    extern bool vblank6, vblank7;
    extern Joystick joystick0, joystick1;
    extern Paddle paddle0, paddle1, paddle2, paddle3;
    extern Keypad keypad0, keypad1;
    extern int input_mode;
    extern uint8_t swcha, swacnt, swbcnt;
    extern uint8_t inpt0, inpt1, inpt2, inpt3;
    extern uint8_t inpt4, inpt5; // these are latched, so must be saved



    uint8_t input_INPT0_read ( void );
    uint8_t input_INPT1_read ( void );
    uint8_t input_INPT2_read ( void );
    uint8_t input_INPT3_read ( void );
    uint8_t input_INPT4_read ( void );
    uint8_t input_INPT5_read ( void );
    uint8_t input_SWCHA_read ( void );
    uint8_t input_SWACNT_read ( void );
    uint8_t input_SWCHB_read ( void );
    uint8_t input_SWBCNT_read ( void );
    void input_SWCHA_write ( uint8_t value );
    void input_SWACNT_write ( uint8_t value );
    void input_SWCHB_write ( uint8_t value );
    void input_SWBCNT_write ( uint8_t value );
    float input_paddle_load_amount(Paddle p);
    void input_register_inputs(void);




    void tick_delayed_writes(void);
    void write_after_delay( void* fn, bool withArg, uint8_t optionalArg, int delayInColorClocks );

    
    typedef struct {
        int volume; // 0..15
        int frequency; // 0..31
        int mode; // 0..15

        int poly4;
        int poly5;
        int poly9;
        unsigned int counter;
    } AudioChannel;

    extern AudioChannel left, right;
    extern AudioStream stream;
    void audio_AUDC0_write ( uint8_t value );
    void audio_AUDC1_write ( uint8_t value );
    void audio_AUDF0_write ( uint8_t value );
    void audio_AUDF1_write ( uint8_t value );
    void audio_AUDV0_write ( uint8_t value );
    void audio_AUDV1_write ( uint8_t value );
    void audio_init( void );

#endif