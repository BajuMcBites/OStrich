#include "mailbox.h"

#include "dcache.h"
#include "printf.h"
#include "stdint.h"

volatile unsigned int mbox[35] __attribute__((aligned(16), section(".mailbox")));

void mailbox_write(unsigned char channel, unsigned int value) {
    printf("Writing to mailbox: channel=%d, value=0x%x\n", channel, value);
    while (*MAILBOX_STATUS & MAILBOX_FULL) {
    }

    if (channel == MBOX_CH_PROP) {
        unsigned int phys_addr = gpu_convert_address((void*)(uintptr_t)value);
        printf("Converting VA 0x%x to PA 0x%x\n", value, phys_addr);
        value = phys_addr;
    }
    *MAILBOX_WRITE = (value & ~0xF) | (channel & 0xF);
}

unsigned int mailbox_read(unsigned char channel) {
    unsigned int response;
    while (1) {
        while (*MAILBOX_STATUS & MAILBOX_EMPTY);
        response = *MAILBOX_READ;
        printf("Read from mailbox: raw=0x%x, channel=%d\n", response, response & 0xF);
        if ((response & 0xF) == channel) {
            printf("Matched channel %d, returning 0x%x\n", channel, response & ~0xF);
            return response & ~0xF;
        }
    }
}

unsigned int gpu_convert_address(void* addr) {
    unsigned long virtual_addr = (unsigned long)addr;
    printf("Converting address VA: 0x%llx\n", virtual_addr);
    unsigned long phys_addr = virtual_addr & ~VA_START;
    printf("Physical address PA: 0x%llx\n", phys_addr);
    unsigned int gpu_addr = (unsigned int)(phys_addr & 0x3FFFFFFF);
    printf("GPU address: 0x%08x\n", gpu_addr);
    return gpu_addr;
}

unsigned int mbox_call(unsigned char ch) {
    printf("mbox array at VA: 0x%llx\n", (unsigned long)&mbox[0]);
    clean_dcache_range((void*)mbox, 36 * sizeof(unsigned int));

    unsigned int r = gpu_convert_address((void*)&mbox[0]);
    printf("Converting to PA=0x%08x\n", r);

    mailbox_write(ch, r);
    unsigned int response = mailbox_read(ch);
    invalidate_dcache_range((void*)mbox, 36 * sizeof(unsigned int));

    return response;
}
