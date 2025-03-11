#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "mailbox.h"


#define COLOR_BLACK   0x00000000
#define COLOR_WHITE   0xFFFFFFFF

struct Framebuffer {
    unsigned int width;
    unsigned int height;
    unsigned int pitch;       // bytes per line
    unsigned int isrgb;       // pixel format
    void* buffer;            // pointer to the buffer
    unsigned int size;       // size of buffer in bytes
};

int fb_init(void);

Framebuffer* fb_get(void);

void fb_clear(unsigned int color);
void fb_print(int x, int y, const char* str, unsigned int color);

void fb_draw_image(int x, int y, const unsigned int* image_data, int width, int height);

void fb_draw_scaled_image(int x, int y, const unsigned int* image_data, 
                         int width, int height, int scale);
void test_display(void);

void init_animation(void);
void update_animation(void);

#endif 