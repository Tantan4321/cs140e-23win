// implement:
//  void uart_init(void)
//
//  int uart_can_get8(void);
//  int uart_get8(void);
//
//  int uart_can_put8(void);
//  void uart_put8(uint8_t c);
//
//  int uart_tx_is_empty(void) {
//
// see that hello world works.
//
//
#include "rpi.h"

#define BAUD_REG_VAL 270

// see broadcomm documents for magic addresses.
enum {
    AUX_BASE = 0x20215000,
    AUX_ENB = (AUX_BASE + 0x04),  // p. 9 disable uart
    AUX_IO = (AUX_BASE + 0x40),  // p. 11 writing data
    AUX_IER = (AUX_BASE + 0x44),  // p. 12 disable interrupts
    AUX_IIR = (AUX_BASE + 0x48),  // p. 13 clearing rx and tx FIFO
    AUX_LCR = (AUX_BASE + 0x4C),  // p. 14 set to put UART in 8-bit mode
    AUX_LSR = (AUX_BASE + 0x54),  // p. 15 Can tx? see if "empty"
    AUX_CNTL  = (AUX_BASE + 0x60),  // p. 17 enable disable Uart tx rx
    AUX_STAT = (AUX_BASE + 0x64), // p. 18 see rx tx status
    AUX_BAUD = (AUX_BASE + 0x68)  // p. 19 write baud rate
};

// called first to setup uart to 8n1 115200  baud,
// no interrupts.
//  - you will need memory barriers, use <dev_barrier()>
//
//  later: should add an init that takes a baud rate.
void uart_init(void) {
    // set RX pin
    gpio_set_function(GPIO_RX, GPIO_FUNC_ALT5);
    // set TX pin
    gpio_set_function(GPIO_TX, GPIO_FUNC_ALT5);

    // Enable miniUART
    unsigned val = GET32(AUX_ENB);
    unsigned mask = 0x01;
    val &= ~mask;
    val |= 0x01;
    PUT32(AUX_ENB, val);

    dev_barrier();

    // Immediately disable Rx/Tx
    val = GET32(AUX_CNTL);
    mask = 0x02;
    val &= ~mask;
    val |= 0x0;
    PUT32(AUX_CNTL, val);

    // Clear TX and RX FIFO
    val = GET32(AUX_IIR);
    mask = 0b110;
    val &= ~mask;
    val |= 0b110;
    PUT32(AUX_IIR, val);

    // Disable interrupts
    val = GET32(AUX_IER);
    mask = 0x02;
    val &= ~mask;
    val |= 0b00;
    PUT32(AUX_IER, val);

    // Set UART to 8n1
    PUT32(AUX_LCR, 0b11);

    // Set baudrate to 115200
    PUT32(AUX_BAUD, (uint16_t)BAUD_REG_VAL);

    // Enable Rx/Tx
    val = GET32(AUX_CNTL);
    mask = 0x02;
    val &= ~mask;
    val |= 0x02;
    PUT32(AUX_CNTL, val);


}

// disable the uart.
void uart_disable(void) {

}


// returns one byte from the rx queue, if needed
// blocks until there is one.
int uart_get8(void) {
	return 0;
}

// 1 = space to put at least one byte, 0 otherwise.
int uart_can_put8(void) {
    return 0;
}

// put one byte on the tx qqueue, if needed, blocks
// until TX has space.
// returns < 0 on error.
int uart_put8(uint8_t c) {
    return 0;
}

// simple wrapper routines useful later.

// 1 = at least one byte on rx queue, 0 otherwise
int uart_has_data(void) {
    todo("must implement\n");
}

// return -1 if no data, otherwise the byte.
int uart_get8_async(void) { 
    if(!uart_has_data())
        return -1;
    return uart_get8();
}

// 1 = tx queue empty, 0 = not empty.
int uart_tx_is_empty(void) {
    unimplemented();
}

// flush out all bytes in the uart --- we use this when 
// turning it off / on, etc.
void uart_flush_tx(void) {
    while(!uart_tx_is_empty())
        ;
}
