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
#include "rpi-interrupts.h"

// see broadcomm documents for magic addresses.
enum {
    GPIO_BASE = 0x20200000,
    gpio_set0  = (GPIO_BASE + 0x1C),
    gpio_clr0  = (GPIO_BASE + 0x28),
    gpio_lev0  = (GPIO_BASE + 0x34),
    gpio_pud = (GPIO_BASE + 0x94),
    gpio_pudclk0 = (GPIO_BASE + 0x98),
    GPIO_GPREN0 = (GPIO_BASE + 0x4c),
    GPIO_GPFEN0 = (GPIO_BASE + 0x58),
    GPIO_GPEDS0 = (GPIO_BASE + 0x40)
};

static void OR32(uint32_t addr, uint32_t val) {
    dev_barrier();
    PUT32(addr, GET32(addr) | val);
    dev_barrier();
}

// set <pin> to be an output pin.
//
// note: fsel0, fsel1, fsel2 are contiguous in memory, so you
// can (and should) use array calculations!
void gpio_set_output(unsigned pin) {
    gpio_set_function(pin, GPIO_FUNC_OUTPUT);
}

// set GPIO <pin> on.
void gpio_set_on(unsigned pin) {
    if(pin >= 32)
        return;

    PUT32(gpio_set0, (1<<pin));
}

// set GPIO <pin> off
void gpio_set_off(unsigned pin) {
    if(pin >= 32)
        return;

    PUT32(gpio_clr0, (1<<pin));
}

// set <pin> to <v> (v \in {0,1})
void gpio_write(unsigned pin, unsigned v) {
    if(pin >= 32)
        return;

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
    gpio_set_function(pin, GPIO_FUNC_INPUT);
}

// return the value of <pin>
int gpio_read(unsigned pin) {
    if(pin >= 32)
        return -1;

    unsigned v = 0;
    unsigned lev = GET32(gpio_lev0);
    if (lev & (1<<pin)) v = 1;

    return DEV_VAL32(v);
}

/**
 * Activate the pullup register on GPIO <pin>.
 *
 * GPIO <pin> must be an input pin.
 */
void gpio_set_pullup(unsigned pin){
    if(pin >= 32)
        return;

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
    if(pin >= 32)
        return;

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
    if(pin >= 32)
        return;

    PUT32(gpio_pud, 0b00);
    delay_cycles(150);
    PUT32(gpio_pudclk0, (1 << pin));
    delay_cycles(150);
    PUT32(gpio_pudclk0, (0 << pin));
}

void gpio_set_function(unsigned pin, gpio_func_t func) {
    if(pin >= 32 || func < 0 || func > 7)
        return;
    unsigned gpio_fsel =  (GPIO_BASE + 0x04 * (pin / 10));
    unsigned value = GET32(gpio_fsel);
    unsigned mask = (0x07 << (3 * (pin % 10)));
    value &= ~mask;
    value |= (func << (3 * (pin % 10)));
    PUT32(gpio_fsel, value);
}


void set_gpio_int(unsigned gpio_int) {
    PUT32(Enable_IRQs_2, (1 << (gpio_int - 32)));
}

// returns 1 if there is currently a GPIO_INT0 interrupt,
// 0 otherwise.
//
// note: we can only get interrupts for <GPIO_INT0> since the
// (the other pins are inaccessible for external devices).
int gpio_has_interrupt(void) {
    unsigned val = GET32(IRQ_pending_2) & (1 << (GPIO_INT0 - 32));
    if (val > 0) return DEV_VAL32(1);
    else return DEV_VAL32(0);
}

// p97 set to detect rising edge (0->1) on <pin>.
// as the broadcom doc states, it  detects by sampling based on the clock.
// it looks for "011" (low, hi, hi) to suppress noise.  i.e., its triggered only
// *after* a 1 reading has been sampled twice, so there will be delay.
// if you want lower latency, you should us async rising edge (p99)
void gpio_int_rising_edge(unsigned pin) {
    if(pin >= 32) return;
    OR32(GPIO_GPREN0, (1 << pin));
    set_gpio_int(GPIO_INT0);
    dev_barrier();
}

// p98: detect falling edge (1->0).  sampled using the system clock.
// similarly to rising edge detection, it suppresses noise by looking for
// "100" --- i.e., is triggered after two readings of "0" and so the
// interrupt is delayed two clock cycles.   if you want  lower latency,
// you should use async falling edge. (p99)
void gpio_int_falling_edge(unsigned pin) {
    if(pin >= 32) return;
    OR32(GPIO_GPFEN0, (1 << pin));
    set_gpio_int(GPIO_INT0);
    dev_barrier();
}

// p96: a 1<<pin is set in EVENT_DETECT if <pin> triggered an interrupt.
// if you configure multiple events to lead to interrupts, you will have to
// read the pin to determine which caused it.
int gpio_event_detected(unsigned pin) {
    if(pin >= 32) return DEV_VAL32(0);
    dev_barrier();
    unsigned event = GET32(GPIO_GPEDS0) & (1<<pin);
    dev_barrier();
    if (event > 0) return DEV_VAL32(1);
    else return DEV_VAL32(0);
}

// p96: have to write a 1 to the pin to clear the event.
void gpio_event_clear(unsigned pin) {
    if(pin >= 32) return;
    dev_barrier();
    PUT32(GPIO_GPEDS0, (1<<pin));
    dev_barrier();
}