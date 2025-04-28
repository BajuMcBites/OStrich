#include <stdint.h>
#include "custom_syscalls.h"

#define FRAME_WIDTH 800
#define FRAME_HEIGHT 600
#define FRAMEBUFFER_SIZE (FRAME_WIDTH * FRAME_HEIGHT * 4)

uint32_t test_frame[FRAME_WIDTH * FRAME_HEIGHT];

void create_test_frame(int frame_counter) {
    for (int y = 0; y < FRAME_HEIGHT; y++) {
        for (int x = 0; x < FRAME_WIDTH; x++) {
            uint32_t color =
                ((x^y) + frame_counter) & 0xff;  // Shift color with frame_counter
            test_frame[y * FRAME_WIDTH + x] =
                0xFF000000 | ((color * 0x10000 + color * 0x100 + color));  // Force alpha to 0xFF
        }
    }
}

int main() {
    int frame_counter = 0;
    while (1) {
        create_test_frame(frame_counter);
        sys_draw_frame((void*)test_frame);
        frame_counter++;
    }
    return 0;
}
