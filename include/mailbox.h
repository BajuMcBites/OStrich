#ifndef _MAILBOX_H
#define _MAILBOX_H

#include "peripherals/base.h"
#include "printf.h"

// Define mailbox base address (for example, on the Pi3)
#define MMIO_BASE       (VA_START | 0x3F000000)
#define MAILBOX_BASE    (MMIO_BASE + 0xB880)

// Verify alignment of mailbox registers
static_assert((MAILBOX_BASE & 0xF) == 0, "Mailbox base must be 16-byte aligned");

// Define mailbox registers (offsets from MAILBOX_BASE)
#define MAILBOX_READ    ((volatile unsigned int*)(MAILBOX_BASE + 0x0))
#define MAILBOX_STATUS  ((volatile unsigned int*)(MAILBOX_BASE + 0x18))
#define MAILBOX_WRITE   ((volatile unsigned int*)(MAILBOX_BASE + 0x20))

// Mailbox status flags
#define MAILBOX_FULL    0x80000000  // Write register is full
#define MAILBOX_EMPTY   0x40000000  // Read register is empty

// Mailbox channels
#define MBOX_CH_POWER   0
#define MBOX_CH_FB      1
#define MBOX_CH_VUART   2
#define MBOX_CH_VCHIQ   3
#define MBOX_CH_LEDS    4
#define MBOX_CH_BTNS    5
#define MBOX_CH_TOUCH   6
#define MBOX_CH_COUNT   7
#define MBOX_CH_PROP    8

// Mailbox tags
#define MBOX_REQUEST    0
#define MBOX_TAG_LAST   0

// Videocore tags
#define MBOX_TAG_SETPHYSIZE     0x48003
#define MBOX_TAG_SETVIRSIZE     0x48004
#define MBOX_TAG_SETDEPTH       0x48005
#define MBOX_TAG_SETPIXELORDER  0x48006
#define MBOX_TAG_GETFB          0x40001
#define MBOX_TAG_GETPITCH       0x40008

// Global mailbox buffer
extern volatile unsigned int mbox[35] __attribute__((aligned(16), section(".mailbox")));

#ifdef __cplusplus
extern "C" {
#endif

extern void clear_mailbox(void);
extern void mailbox_write(unsigned char channel, unsigned int value);
extern unsigned int mailbox_read(unsigned char channel);
extern unsigned int mbox_call(unsigned char ch);
extern unsigned int gpu_convert_address(void* addr);

#ifdef __cplusplus
}
#endif

#endif // _MAILBOX_H
