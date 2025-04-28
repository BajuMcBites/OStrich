#ifndef _TTY_H
#define _TTY_H

#include "framebuffer.h"
#include "keyboard.h"
#include "stdint.h"

#define SCALE 16
#define WIDTH 680
#define HEIGHT 480

#define QUEUE_SIZE 680
#define MAX_TTY 6

/**
 * @brief Initialize tty. Limited to six slots
 * @param none
 */
void init_tty();

/**
 * @brief Return a tty if one is available
 * @param none
 */
Framebuffer* request_tty();

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