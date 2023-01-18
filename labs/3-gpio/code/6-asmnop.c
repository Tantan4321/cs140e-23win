// part 1: uses your GPIO code to blink a single LED connected to 
// pin 20.
//   - when run should blink 10 times.  
//   - you will have to restart the pi by pulling the usb connection out.
#include "rpi.h"

// define: dummy to immediately return and PUT32 as above.
void reboot(void) {
    const int PM_RSTC = 0x2010001c;
    const int PM_WDOG = 0x20100024;
    const int PM_PASSWORD = 0x5a000000;
    const int PM_RSTC_WRCFG_FULL_RESET = 0x00000020;
    int i;
    for(i = 0; i < 100000; i++)
        nop();
    PUT32(PM_WDOG, PM_PASSWORD | 1);
    PUT32(PM_RSTC, PM_PASSWORD | PM_RSTC_WRCFG_FULL_RESET);
    while(1);
}

void notmain(void) {
    int led = 20;
    gpio_set_output(led);
    for(int i = 0; i < 5000; i++) {
        gpio_set_on(led);
        delay_cycles(20000);
        gpio_set_off(led);
        delay_cycles(20000);
    }
    reboot();
}
