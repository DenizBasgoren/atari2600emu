
#include <stdint.h>
#include <stdbool.h>

#include "atari2600emulator.h"


Memory memory;
Cartridge cartridge;


uint8_t memory_read( uint16_t address ) {
    bool A12 = address>>12 & 1;
    bool A9 = address>>9 & 1;
    bool A7 = address>>7 & 1;

    if ( !A12 && !A7 ) {
        // TIA read
        switch ( address & 0xF) {
            case 0: return video_CXM0P_read();
            case 1: return video_CXM1P_read();
            case 2: return video_CXP0FB_read();
            case 3: return video_CXP1FB_read();
            case 4: return video_CXM0FB_read();
            case 5: return video_CXM1FB_read();
            case 6: return video_CXBLPF_read();
            case 7: return video_CXPPMM_read();
            case 8: return input_INPT0_read();
            case 9: return input_INPT1_read();
            case 10: return input_INPT2_read();
            case 11: return input_INPT3_read();
            case 12: return input_INPT4_read();
            case 13: return input_INPT5_read();
        }
    }
    else if ( !A12 && !A9 && A7 ) {
        // RAM read
        return memory[address & 0x7F];
    }
    else if ( !A12 && A9 && A7 ) {
        // PIA read
        switch (address & 0x7) {
            case 0: return input_SWCHA_read();
            case 1: return input_SWACNT_read();
            case 2: return input_SWCHB_read();
            case 3: return input_SWBCNT_read();
            case 4: return timer_INTIM_read();
            case 5: return timer_INSTAT_read();
            case 6: return timer_INTIM_read();
            case 7: return timer_INSTAT_read();
        }
    }

    return memory_access_cartridge(address, 0);
}


void memory_write( uint16_t address, uint8_t value ) {
    bool A12 = address>>12 & 1;
    bool A9 = address>>9 & 1;
    bool A7 = address>>7 & 1;

    if ( !A12 && !A7 ) {
        // TIA write
        switch ( address & 0x3F) {
            case 0x00: return video_VSYNC_write(value); 
            case 0x01: return video_VBLANK_write(value); 
            case 0x02: return video_WSYNC_write(); 
            case 0x03: return video_RSYNC_write(); 
            case 0x04: return video_NUSIZ0_write(value); 
            case 0x05: return video_NUSIZ1_write(value); 
            case 0x06: return video_COLUP0_write(value); 
            case 0x07: return video_COLUP1_write(value); 
            case 0x08: return video_COLUPF_write(value); 
            case 0x09: return video_COLUBK_write(value); 
            case 0x0A: return video_CTRLPF_write(value); 
            case 0x0B: return video_REFP0_write(value); 
            case 0x0C: return video_REFP1_write(value); 
            case 0x0D: return video_PF0_write(value); 
            case 0x0E: return video_PF1_write(value); 
            case 0x0F: return video_PF2_write(value); 
            case 0x10: return video_RESP0_write(); 
            case 0x11: return video_RESP1_write(); 
            case 0x12: return video_RESM0_write(); 
            case 0x13: return video_RESM1_write(); 
            case 0x14: return video_RESBL_write(); 
            case 0x15: return ; // audio shit
            case 0x16: return ; 
            case 0x17: return ; 
            case 0x18: return ; 
            case 0x19: return ; 
            case 0x1A: return ; 
            case 0x1B: return video_GRP0_write(value); 
            case 0x1C: return video_GRP1_write(value); 
            case 0x1D: return video_ENAM0_write(value); 
            case 0x1E: return video_ENAM1_write(value); 
            case 0x1F: return video_ENABL_write(value); 
            case 0x20: return video_HMP0_write(value); 
            case 0x21: return video_HMP1_write(value); 
            case 0x22: return video_HMM0_write(value); 
            case 0x23: return video_HMM1_write(value); 
            case 0x24: return video_HMBL_write(value); 
            case 0x25: return video_VDELP0_write(value); 
            case 0x26: return video_VDELP1_write(value); 
            case 0x27: return video_VDELBL_write(value); 
            case 0x28: return video_RESMP0_write(value); 
            case 0x29: return video_RESMP1_write(value); 
            case 0x2A: return video_HMOVE_write(); 
            case 0x2B: return video_HMCLR_write(); 
            case 0x2C: return video_CXCLR_write(); 
        }
    }
    else if ( !A12 && !A9 && A7 ) {
        // RAM write
        memory[address & 0x7F] = value;
    }
    else if ( !A12 && A9 && A7 ) {
        // PIA write
        switch (address & 0x7) {
            case 0: return input_SWCHA_write(value);
            case 1: return input_SWACNT_write(value);
            case 2: return input_SWCHB_write(value);
            case 3: return input_SWBCNT_write(value);
            case 4: return timer_TIM1T_write(value);
            case 5: return timer_TIM8T_write(value);
            case 6: return timer_TIM64T_write(value);
            case 7: return timer_T1024T_write(value);
        }
    }

    memory_access_cartridge(address, value);
}


    

uint8_t memory_access_cartridge( uint16_t address, uint8_t value ) {
    bool A15 = address>>15 & 1;
    bool A14 = address>>14 & 1;
    bool A13 = address>>13 & 1;
    bool A12 = address>>12 & 1;
    bool A11 = address>>11 & 1;
    bool A10 = address>>10 & 1;
    bool A9 = address>>9 & 1;

    switch (cartridge.type) {
        case CARTRIDGE_2K:
        {
            if (!A12) return 0;
            return cartridge.raw[address & 0x7FF];
        }

        case CARTRIDGE_4K:
        {
            if (!A12) return 0;
            return cartridge.raw[address & 0xFFF];
        }

        
        case CARTRIDGE_CV:
        {
            if (!A12) return 0;
            if (A11) return cartridge.raw[address & 0x7FF];
            if (A10) return cartridge.CV.extra_ram[address & 0x3FF] = value;
            return cartridge.CV.extra_ram[address & 0x3FF];
        }

        case CARTRIDGE_F8:
        {
            if (!A12) return 0;
            if ( (address & 0x1FFF) == 0x1FF8 ) cartridge.F8.current_bank = 0;
            if ( (address & 0x1FFF) == 0x1FF9 ) cartridge.F8.current_bank = 1;
            if ( cartridge.F8.current_bank == 0) return cartridge.raw[0x0000 + address & 0xFFF];
            if ( cartridge.F8.current_bank == 1) return cartridge.raw[0x1000 + address & 0xFFF];
            return 0;
        }

        case CARTRIDGE_F6:
        {
            if (!A12) return 0;
            if ( (address & 0x1FFF) == 0x1FF6 ) cartridge.F6.current_bank = 0;
            if ( (address & 0x1FFF) == 0x1FF7 ) cartridge.F6.current_bank = 1;
            if ( (address & 0x1FFF) == 0x1FF8 ) cartridge.F6.current_bank = 2;
            if ( (address & 0x1FFF) == 0x1FF9 ) cartridge.F6.current_bank = 3;
            if ( cartridge.F6.current_bank == 0) return cartridge.raw[0x0000 + address & 0xFFF];
            if ( cartridge.F6.current_bank == 1) return cartridge.raw[0x1000 + address & 0xFFF];
            if ( cartridge.F6.current_bank == 2) return cartridge.raw[0x2000 + address & 0xFFF];
            if ( cartridge.F6.current_bank == 3) return cartridge.raw[0x3000 + address & 0xFFF];
            return 0;
        }

        case CARTRIDGE_F4:
        {
            if (!A12) return 0;
            if ( (address & 0x1FFF) == 0x1FF4 ) cartridge.F4.current_bank = 0;
            if ( (address & 0x1FFF) == 0x1FF5 ) cartridge.F4.current_bank = 1;
            if ( (address & 0x1FFF) == 0x1FF6 ) cartridge.F4.current_bank = 2;
            if ( (address & 0x1FFF) == 0x1FF7 ) cartridge.F4.current_bank = 3;
            if ( (address & 0x1FFF) == 0x1FF8 ) cartridge.F4.current_bank = 4;
            if ( (address & 0x1FFF) == 0x1FF9 ) cartridge.F4.current_bank = 5;
            if ( (address & 0x1FFF) == 0x1FFA ) cartridge.F4.current_bank = 6;
            if ( (address & 0x1FFF) == 0x1FFB ) cartridge.F4.current_bank = 7;
            if ( cartridge.F4.current_bank == 0) return cartridge.raw[0x0000 + address & 0xFFF];
            if ( cartridge.F4.current_bank == 1) return cartridge.raw[0x1000 + address & 0xFFF];
            if ( cartridge.F4.current_bank == 2) return cartridge.raw[0x2000 + address & 0xFFF];
            if ( cartridge.F4.current_bank == 3) return cartridge.raw[0x3000 + address & 0xFFF];
            if ( cartridge.F4.current_bank == 4) return cartridge.raw[0x4000 + address & 0xFFF];
            if ( cartridge.F4.current_bank == 5) return cartridge.raw[0x5000 + address & 0xFFF];
            if ( cartridge.F4.current_bank == 6) return cartridge.raw[0x6000 + address & 0xFFF];
            if ( cartridge.F4.current_bank == 7) return cartridge.raw[0x7000 + address & 0xFFF];
            return 0;
        }

        case CARTRIDGE_FE:
        {
            if (!A12) return 0;
            if (A15 && A14) cartridge.FE.current_bank = A13;
            if ( cartridge.FE.current_bank == 0) return cartridge.raw[0x0000 + address & 0xFFF];
            if ( cartridge.FE.current_bank == 1) return cartridge.raw[0x1000 + address & 0xFFF];
            return 0;
        }

        case CARTRIDGE_E0:
        {
            if (!A12) return 0;
            int a = address & 0x1FFF;
            if (a>=0x1FE0 && a<=0x1FE7) cartridge.E0.current_lower_bank = a - 0x1FE0;
            if (a>=0x1FE8 && a<=0x1FEF) cartridge.E0.current_middle_bank = a - 0x1FE8;
            if (a>=0x1FF0 && a<=0x1FF7) cartridge.E0.current_upper_bank = a - 0x1FF0;
            if (a>=0x1000 && a<=0x13FF) return cartridge.raw[cartridge.E0.current_lower_bank * 0x400 + a-0x1000];
            if (a>=0x1400 && a<=0x17FF) return cartridge.raw[cartridge.E0.current_middle_bank * 0x400 + a-0x1400];
            if (a>=0x1800 && a<=0x1BFF) return cartridge.raw[cartridge.E0.current_upper_bank * 0x400 + a-0x1800];
            if (a>=0x1C00 && a<=0x1FFF) return cartridge.raw[7 * 0x400 + a-0x1C00];
            return 0;
        }

        case CARTRIDGE_3F:
        {
            if (!A12) return 0;
        }

        case CARTRIDGE_FA:
        {
            if (!A12) return 0;
            int a = address & 0x1FFF;
            if ( a == 0x1FF8 ) cartridge.FA.current_bank = 0;
            if ( a == 0x1FF9 ) cartridge.FA.current_bank = 1;
            if ( a == 0x1FFA ) cartridge.FA.current_bank = 2;
            if ( a>=0x1000 && a<=0x10FF ) return cartridge.FA.extra_ram[ a-0x1000 ] = value;
            if ( a>=0x1100 && a<=0x11FF ) return cartridge.FA.extra_ram[ a-0x1100 ];
            return cartridge.raw[ cartridge.FA.current_bank * 0x1000 + a-0x1000 ];
        }

        case CARTRIDGE_E7:
        {
            if (!A12) return 0;
            int a = address & 0x1FFF;
            if ( a == 0x1FE0 ) cartridge.E7.current_lower_bank = 0;
            if ( a == 0x1FE1 ) cartridge.E7.current_lower_bank = 1;
            if ( a == 0x1FE2 ) cartridge.E7.current_lower_bank = 2;
            if ( a == 0x1FE3 ) cartridge.E7.current_lower_bank = 3;
            if ( a == 0x1FE4 ) cartridge.E7.current_lower_bank = 4;
            if ( a == 0x1FE5 ) cartridge.E7.current_lower_bank = 5;
            if ( a == 0x1FE6 ) cartridge.E7.current_lower_bank = 6;
            if ( a == 0x1FE7 ) cartridge.E7.current_lower_bank = 7;
            if ( a == 0x1FE8 ) cartridge.E7.current_upper_bank = 0;
            if ( a == 0x1FE9 ) cartridge.E7.current_upper_bank = 1;
            if ( a == 0x1FEA ) cartridge.E7.current_upper_bank = 2;
            if ( a == 0x1FEB ) cartridge.E7.current_upper_bank = 3;
            if ( a>=0x1000 && a<=0x17FF && cartridge.E7.current_lower_bank!=7) {
                return cartridge.raw[ cartridge.E7.current_lower_bank * 0x800 + a-0x1000 ];
            }
            if ( a>=0x1000 && a<=0x13FF && cartridge.E7.current_lower_bank==7 ) {
                return cartridge.E7.extra_ram[ a-0x1000 ] = value;
            }
            if ( a>=0x1400 && a<=0x17FF && cartridge.E7.current_lower_bank==7 ) {
                return cartridge.E7.extra_ram[ a-0x1400 ];
            }
            if ( a>=0x1800 && a<=0x18FF ) {
                return cartridge.E7.extra_ram[ 0x400 + cartridge.E7.current_upper_bank*0x100 + a-0x1800] = value;
            }
            if ( a>=0x1900 && a<=0x19FF ) {
                return cartridge.E7.extra_ram[ 0x400 + cartridge.E7.current_upper_bank*0x100 + a-0x1900];
            }
            if ( a>=0x1A00 && a<=0x1FFF ) {
                return cartridge.raw[ 0x3A00 + a-0x1A00 ];
            }
            return 0;
        }

        case CARTRIDGE_F0:
        {
            if (!A12) return 0;
            int a = address & 0x1FFF;
            if (a==0x1FF0) {
                cartridge.F0.current_bank++;
                cartridge.F0.current_bank %= 16;
            }
            return cartridge.raw[ cartridge.F0.current_bank * 0x1000 + a-0x1000 ];
        }

        case CARTRIDGE_UA:
        {
            if (!A12) return 0;
        }


    }

    return 0;
}