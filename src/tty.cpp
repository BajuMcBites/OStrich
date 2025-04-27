#include "tty.h"

#include "dwc.h"
#include "framebuffer.h"
#include "libk.h"
#include "listener.h"
#include "peripherals/timer.h"
#include "peripherals/usb_hid_keys.h"
#include "printf.h"
#include "timer.h"

// int fb_pitch;
// int fb_size;
uint32_t *real_buffer;
Framebuffer *fb;
int active = -1;
TtyBuffer *tty[MAX_TTY] = {nullptr};

/**
 * @brief Initialize tty. Limited to six slots
 * @param none
 */
void init_tty() {
    real_buffer = (uint32_t *)fb_get()->buffer;  // set pointer to actual frame buffer
    fb = fb_get();
    for (int i = 0; i < MAX_TTY; i++) {
        tty[i] = new TtyBuffer;
    }
    active = 0;
}

/**
 * @brief Return a tty if one is available
 * @param none
 */
TtyBuffer *request_tty() {
}

/**
 * @brief make the tty is use available
 * @param none
 */
void close_tty() {
}

/**
 * @brief make the tty is use available
 * @param none
 */
void change_tty(struct key_event *event) {
    if (event->flags.released) return;
    for (int i = 0; i < MAX_TTY; i++) {
        // if I have a dummy buffer, delete
        if (tty[i]->buffer != nullptr && tty[i]->buffer != fb_get()->buffer) {
            delete tty[i]->buffer;
        }
        tty[i]->buffer = new uint32_t;
    }

    if (event->keycode == KEY_F1) {
        tty[0]->buffer = fb_get()->buffer;
    } else if (event->keycode == KEY_F2) {
        tty[1]->buffer = fb_get()->buffer;
    } else if (event->keycode == KEY_F3) {
        tty[2]->buffer = fb_get()->buffer;
    } else if (event->keycode == KEY_F4) {
        tty[3]->buffer = fb_get()->buffer;
    } else if (event->keycode == KEY_F5) {
        tty[4]->buffer = tty[3]->buffer = fb_get()->buffer;
    } else if (event->keycode == KEY_F6) {
        tty[5]->buffer = tty[3]->buffer = fb_get()->buffer;
    }
}

void run_tty() {
    tty_key_listener = {.handler = (void (*)(key_event *))&change_tty, .next = nullptr};

    event_handler->register_listener(KEYBOARD_EVENT, &tty_key_listener);
    fb_clear(0xFFFFFFFF);
}
