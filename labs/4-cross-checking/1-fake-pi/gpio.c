/*
 * Implement the following routines to set GPIO pins to input or output,
 * and to read (input) and write (output) them.
 *
 * DO NOT USE loads and stores directly: only use GET32 and PUT32
 * to read and write memory.  Use the minimal number of such calls.
 *
 * See rpi.h in this directory for the definitions.
 */
#include "rpi.h"

// see broadcomm documents for magic addresses.
enum {
    GPIO_BASE = 0x20200000,
    gpio_set0  = (GPIO_BASE + 0x1C),
    gpio_clr0  = (GPIO_BASE + 0x28),
    gpio_lev0  = (GPIO_BASE + 0x34),
    gpio_pud = (GPIO_BASE + 0x94),
    gpio_pudclk0 = (GPIO_BASE + 0x98)
};

//
// Part 1 implement gpio_set_on, gpio_set_off, gpio_set_output
//

// set <pin> to be an output pin.
//
// note: fsel0, fsel1, fsel2 are contiguous in memory, so you
// can (and should) use array calculations!
void gpio_set_output(unsigned pin) {
    if(pin >= 32)
        return;
    unsigned gpio_fsel =  (GPIO_BASE + 0x04 * (pin / 10));
    unsigned value = GET32(gpio_fsel);
    unsigned mask = (0x07 << (3 * (pin % 10)));
    value &= ~mask;
    value |= (0x01 << (3 * (pin % 10)));
    PUT32(gpio_fsel, value);
}

// set GPIO <pin> on.
void gpio_set_on(unsigned pin) {
    if(pin >= 32)
        return;
  // implement this
    PUT32(gpio_set0, (1<<pin));
}

// set GPIO <pin> off
void gpio_set_off(unsigned pin) {
    if(pin >= 32)
        return;
  // implement this
    PUT32(gpio_clr0, (1<<pin));
}

// set <pin> to <v> (v \in {0,1})
void gpio_write(unsigned pin, unsigned v) {
    if(v)
        gpio_set_on(pin);
    else
        gpio_set_off(pin);
}

//
// Part 2: implement gpio_set_input and gpio_read
//

// set <pin> to input.
void gpio_set_input(unsigned pin) {
    unsigned gpio_fsel =  (GPIO_BASE + 0x04 * (pin / 10));
    unsigned value = GET32(gpio_fsel);
    unsigned mask = (0x07 << (3 * (pin % 10)));
    value &= ~mask;
    value |= (0x00 << (3 * (pin % 10)));
    PUT32(gpio_fsel, value);
}

// return the value of <pin>
int gpio_read(unsigned pin) {
    unsigned v = 0;

    unsigned lev = GET32(gpio_lev0);
    if (lev & (1<<pin)) {
        v = 1;
    }

    return v;
}

/**
 * Activate the pullup register on GPIO <pin>.
 *
 * GPIO <pin> must be an input pin.
 */
void gpio_set_pullup(unsigned pin){
    PUT32(gpio_pud, 0b10);
    delay_cycles(150);
    PUT32(gpio_pudclk0, (1 << pin));
    delay_cycles(150);
    PUT32(gpio_pudclk0, (0 << pin));
}

/**
 * Activate the pulldown register on GPIO <pin>.
 *
 * GPIO <pin> must be an input pin.
 */
void gpio_set_pulldown(unsigned pin) {
    PUT32(gpio_pud, 0b01);
    delay_cycles(150);
    PUT32(gpio_pudclk0, (1 << pin));
    delay_cycles(150);
    PUT32(gpio_pudclk0, (0 << pin));
}

/**
 * Deactivate both the pullup and pulldown registers on GPIO <pin>.
 *
 * GPIO <pin> must be an input pin.
 */
void gpio_pud_off(unsigned pin){
    PUT32(gpio_pud, 0b00);
    delay_cycles(150);
    PUT32(gpio_pudclk0, (1 << pin));
    delay_cycles(150);
    PUT32(gpio_pudclk0, (0 << pin));
}