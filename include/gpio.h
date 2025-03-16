/*
 * Copyright (C) 2018 bzt (bztsrc@github)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef GPIO_H
#define GPIO_H
#define MMIO_BASE 0x3F000000

#define MAX_GPIO_PINS 192  // https://www.pi4j.com/1.2/pins/model-3b-rev1.html

#define GPFSEL0 ((volatile unsigned int *)(MMIO_BASE + 0x00200000))
#define GPFSEL1 ((volatile unsigned int *)(MMIO_BASE + 0x00200004))
#define GPFSEL2 ((volatile unsigned int *)(MMIO_BASE + 0x00200008))
#define GPFSEL3 ((volatile unsigned int *)(MMIO_BASE + 0x0020000C))
#define GPFSEL4 ((volatile unsigned int *)(MMIO_BASE + 0x00200010))
#define GPFSEL5 ((volatile unsigned int *)(MMIO_BASE + 0x00200014))
#define GPSET0 ((volatile unsigned int *)(MMIO_BASE + 0x0020001C))
#define GPSET1 ((volatile unsigned int *)(MMIO_BASE + 0x00200020))
#define GPCLR0 ((volatile unsigned int *)(MMIO_BASE + 0x00200028))
#define GPLEV0 ((volatile unsigned int *)(MMIO_BASE + 0x00200034))
#define GPLEV1 ((volatile unsigned int *)(MMIO_BASE + 0x00200038))
#define GPEDS0 ((volatile unsigned int *)(MMIO_BASE + 0x00200040))
#define GPEDS1 ((volatile unsigned int *)(MMIO_BASE + 0x00200044))
#define GPHEN0 ((volatile unsigned int *)(MMIO_BASE + 0x00200064))
#define GPHEN1 ((volatile unsigned int *)(MMIO_BASE + 0x00200068))
#define GPPUD ((volatile unsigned int *)(MMIO_BASE + 0x00200094))
#define GPPUDCLK0 ((volatile unsigned int *)(MMIO_BASE + 0x00200098))
#define GPPUDCLK1 ((volatile unsigned int *)(MMIO_BASE + 0x0020009C))

#define DIR_INPUT 0b000   // GPIO pin configured as Input
#define DIR_OUTPUT 0b001  // GPIO pin configured as Output
#define DIR_ALT0 0b100    // eg. UART, SPI, I2C
#define DIR_ALT1 0b101    // eg. PWM, SPI
#define DIR_ALT2 0b110    // eg. I2S, SPI
#define DIR_ALT3 0b111    // eg. SPI, UART

#define GPPUD_DISABLED 0b00
#define GPPUD_PULL_DOWN 0b01
#define GPPUD_PULL_UP 0b10
#define GPPUD_RESERVED 0b11

#define GPIO_DELAY 150  // microseconds

#include "stdint.h"

typedef struct {
    char pin_name[25];
    bool in_use;
    unsigned high;
    unsigned direction;
    unsigned value;
    unsigned pull;
} gpio_pin_t;

void gpio_init(void);
int gpio_set_direction(unsigned, unsigned);
int gpio_set_value(unsigned, unsigned);
int gpio_request(unsigned, const char *);
int gpio_pull(unsigned, unsigned);
int gpio_set_high(unsigned, unsigned);
void gpio_print_state(void);

// void gpio_set_direction(uint32_t pin, uint32_t direction);
// void gpio_set_pull(uint32_t pin, uint32_t pull_type);
// void gpio_set_function(uint32_t pin, uint32_t function);
#endif
