
#include <stdint.h>
#include <stdbool.h>

#include "atari2600emulator.h"

/*
    0284    INTIM   11111111  Timer output (read only)
    0285    INSTAT  11......  Timer Status (read only, undocumented)
    0294    TIM1T   11111111  set 1 clock interval (838 nsec/interval)
    0295    TIM8T   11111111  set 8 clock interval (6.7 usec/interval)
    0296    TIM64T  11111111  set 64 clock interval (53.6 usec/interval)
    0297    T1024T  11111111  set 1024 clock interval (858.2 usec/interval)
*/


int timer_value;
int timer_prescaler = 1;
bool timer_underflow_bit7;
bool timer_underflow_bit6;


uint8_t timer_INTIM_read ( void ) {
    int result;
    if (timer_value >= 0) {
        result = timer_value / timer_prescaler;
    }
    else {
        result = (timer_value % 256) + 256;
    }
    timer_underflow_bit7 = false;
    timer_value = result * timer_prescaler;
    return result;
}



uint8_t timer_INSTAT_read ( void ) {
    uint8_t result = (timer_underflow_bit7<<7) | (timer_underflow_bit6<<6);
    timer_underflow_bit6 = 0;
    return result;
}



void timer_TIM1T_write  ( uint8_t value ) {
    timer_value = value;
    timer_prescaler = 1;
    timer_underflow_bit7 = false;
}



void timer_TIM8T_write  ( uint8_t value ) {
    timer_value = value * 8;
    timer_prescaler = 8;
    timer_underflow_bit7 = false;
}



void timer_TIM64T_write ( uint8_t value ) {
    timer_value = value * 64;
    timer_prescaler = 64;
    timer_underflow_bit7 = false;
}



void timer_T1024T_write ( uint8_t value ) {
    timer_value = value * 1024;
    timer_prescaler = 1024;
    timer_underflow_bit7 = false;
}



void timer_tick( void ) {
    if (timer_value == 0) {
        timer_underflow_bit6 = true;
        timer_underflow_bit7 = true;
    }
    timer_value--;
}




