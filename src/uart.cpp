#include "uart.h"

#include "gpio.h"
#include "peripherals/base.h"
#include "utils.h"

// Base addresses for UART0 and GPIO
#define MMIO_BASE (VA_START | 0x3F000000)
#define GPFSEL1 ((volatile unsigned int*)(MMIO_BASE + 0x200004))
#define GPPUD ((volatile unsigned int*)(MMIO_BASE + 0x200094))
#define GPPUDCLK0 ((volatile unsigned int*)(MMIO_BASE + 0x200098))
#define UART0_BASE (MMIO_BASE + 0x201000)
#define UART0_DR ((volatile unsigned int*)(UART0_BASE + 0x00))
#define UART0_FR ((volatile unsigned int*)(UART0_BASE + 0x18))
#define UART0_IBRD ((volatile unsigned int*)(UART0_BASE + 0x24))
#define UART0_FBRD ((volatile unsigned int*)(UART0_BASE + 0x28))
#define UART0_LCRH ((volatile unsigned int*)(UART0_BASE + 0x2C))
#define UART0_CR ((volatile unsigned int*)(UART0_BASE + 0x30))
#define UART0_IMSC ((volatile unsigned int*)(UART0_BASE + 0x38))
#define UART0_ICR ((volatile unsigned int*)(UART0_BASE + 0x44))

void uart_init(void) {
    const char* gpio_14 = "GPIO_14";
    const char* gpio_15 = "GPIO_15";

    gpio_request(14, gpio_14);
    gpio_request(15, gpio_15);

    gpio_set_direction(14, DIR_ALT0);
    gpio_set_direction(15, DIR_ALT0);

    gpio_pull(14, GPPUD_DISABLED);
    gpio_pull(15, GPPUD_DISABLED);

    // Disable UART0.
    put32(UART0_CR, 0);

    // Set integer & fractional part of baud rate
    put32(UART0_IBRD, 26);  // Integer part of baud rate divisor
    put32(UART0_FBRD, 3);   // Fractional part of baud rate divisor

    // Set data format and enable FIFOs
    put32(UART0_LCRH, (1 << 4) | (1 << 5) | (1 << 6));  // 8-bit, FIFO enabled

    // Enable UART0, transmit, and receive
    put32(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));  // UARTEN, TXE, RXE
}

char uart_getc(void) {
    // Wait until data is available
    while (get32(UART0_FR) & (1 << 4)) {
    }
    char c = (char)(get32(UART0_DR) & 0xFF);
    // If the character is a carriage return, translate it to a newline.
    if (c == '\r') {
        c = '\n';
    }
    return c;
}

void uart_putc(char c) {
    // Wait until transmit FIFO has space
    while (get32(UART0_FR) & 0x20);
    put32(UART0_DR, c);
    if (c == '\n') {
        put32(UART0_DR, '\r');
    }
}

void uart_puts(const char* str) {
    while (*str) {
        uart_putc(*str++);
    }
}

void uart_hex(unsigned int d) {
    unsigned int n;
    int c;
    for (c = 28; c >= 0; c -= 4) {
        // get highest tetrad
        n = (d >> c) & 0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n += n > 9 ? 0x37 : 0x30;
        uart_putc(n);
    }
}

void uart_putc_wrapper(void* p, char c) {
    (void)p;  // Unused parameter
    if (c == '\n') {
        uart_putc('\r');
    }
    uart_putc(c);
}
