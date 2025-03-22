#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "mailbox.h"


#define BLACK   0xFF000000
#define WHITE   0xFFFFFFFF
#define RED     0xFFFF0000
#define GREEN   0xFF00FF00
#define BLUE    0xFF0000FF
#define CYAN    0xFFFFFF00
#define YELLOW  0xFF00FFFF
#define PURPLE  0xFFFF00FF
#define GRAY    0xFF808080

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
void fb_print(const char* str, unsigned int color);

void fb_draw_image(int x, int y, const unsigned int* image_data, int width, int height);

void fb_draw_scaled_image(int x, int y, const unsigned int* image_data, 
                         int width, int height, int scale);

void draw_char(int x, int y, char c, unsigned int color);

void init_animation(void);
void update_animation(void);

#endif 