#ifndef _TTY_H
#define _TTY_H

#include "framebuffer.h"
#include "keyboard.h"
#include "stdint.h"

#define SCALE 16
#define WIDTH 680
#define HEIGHT 480

#define DIRECTION_UP ((1 << 4) | (0 << 0))
#define DIRECTION_DOWN ((1 << 4) | (2 << 0))
#define DIRECTION_RIGHT ((2 << 4) | (1 << 0))
#define DIRECTION_LEFT ((0 << 4) | (1 << 0))
#define QUEUE_SIZE 680
#define FPS 30
#define MAX_TTY 6

struct TtyBuffer {
    unsigned int width;
    unsigned int height;
    unsigned int pitch;  // bytes per line
    unsigned int isrgb;  // pixel format
    void* buffer;        // pointer to the buffer.
    unsigned int size;   // size of buffer in bytes
};

Listener<struct key_event*> tty_key_listener;

/**
 * @brief Initialize tty. Limited to six slots
 * @param none
 */
void init_tty();

/**
 * @brief Return a tty if one is available
 * @param none
 */
TtyBuffer* request_tty();

/**
 * @brief make the tty is use available
 * @param none
 */
void close_tty();

/**
 * @brief take key input and swap to corresponding console
 * @param none
 */
void change_tty(struct key_event* event);

void run_tty();
#endif