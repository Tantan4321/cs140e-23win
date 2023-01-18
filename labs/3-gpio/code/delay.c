#include "rpi.h"

void delay_cycles(unsigned ticks) {
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    while (ticks-- > 0)
    nop();
}

