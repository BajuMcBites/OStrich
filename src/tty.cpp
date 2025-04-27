#include "tty.h"

#include "dwc.h"
#include "event.h"
#include "framebuffer.h"
#include "libk.h"
#include "listener.h"
#include "peripherals/timer.h"
#include "peripherals/usb_hid_keys.h"
#include "printf.h"
#include "timer.h"

static Listener<struct key_event *> tty_key_listener;

static unsigned int *dummy_buffers[6] = {nullptr};

int active = -1;  // index of window in foreground
Framebuffer *tty[MAX_TTY] = {nullptr};
bool taken[MAX_TTY] = {false};
/**
 * @brief Initialize tty. Limited to six slots
 * @param none
 */
void init_tty() {
    tty[0] = get_kernel_fb();  // kernel gets position 0 to print should be the shell later?
    taken[0] = true;
    dummy_buffers[0] = new unsigned int[tty[0]->size / sizeof(unsigned int)];
    auto temp = get_real_fb();
    for (int i = 1; i < MAX_TTY; i++) {
        tty[i] = new Framebuffer;
        tty[i]->height = temp->height;
        tty[i]->isrgb = temp->isrgb;
        tty[i]->pitch = temp->pitch;
        tty[i]->size = temp->size;
        tty[i]->width = temp->width;
        dummy_buffers[i] = dummy_buffers[0];
    }
    active = 0;
}

/**
 * @brief Return a tty if one is available
 * @param none
 */
Framebuffer *request_tty() {
    // ew linear search
    for (int i = 0; i < MAX_TTY; i++) {
        if (!taken[i]) return tty[i];
    }
    return nullptr;
}

/**
 * @brief make the tty in use available
 * @param none
 */
void close_tty() {
}

/**
 * @brief make the tty is use available
 * @param none
 *
 * this is code is really ugly ik but it is 3am and I cannot
 */
void change_tty(struct key_event *event) {
    // printf("IN CHANGE TTY\n");
    if (event->flags.released) return;

    if (event->keycode == KEY_F1) {
        printf("TERM 0\n");
        tty[0]->buffer = get_real_fb()->buffer;
        active = 0;

        for (int i = 0; i < MAX_TTY; i++) {
            if (i == active) continue;
            tty[i]->buffer = dummy_buffers[i];
        }
        fb_blank(WHITE);
    } else if (event->keycode == KEY_F2) {
        printf("TERM 1\n");
        tty[1]->buffer = get_real_fb()->buffer;
        active = 1;
        for (int i = 0; i < MAX_TTY; i++) {
            if (i == active) continue;
            tty[i]->buffer = dummy_buffers[i];
        }
        fb_blank(BLUE);
    } else if (event->keycode == KEY_F3) {
        printf("TERM 2\n");
        tty[2]->buffer = get_real_fb()->buffer;
        active = 2;
        for (int i = 0; i < MAX_TTY; i++) {
            if (i == active) continue;
            tty[i]->buffer = dummy_buffers[i];
        }
        fb_blank(RED);
    } else if (event->keycode == KEY_F4) {
        printf("TERM 3\n");
        tty[3]->buffer = get_real_fb()->buffer;
        active = 3;
        for (int i = 0; i < MAX_TTY; i++) {
            if (i == active) continue;
            tty[i]->buffer = dummy_buffers[i];
        }
        fb_blank(YELLOW);
    } else if (event->keycode == KEY_F5) {
        printf("TERM 4\n");
        tty[4]->buffer = get_real_fb()->buffer;
        active = 4;
        for (int i = 0; i < MAX_TTY; i++) {
            if (i == active) continue;
            tty[i]->buffer = dummy_buffers[i];
        }
        fb_blank(GREEN);
    } else if (event->keycode == KEY_F6) {
        printf("TERM 5\n");
        tty[5]->buffer = get_real_fb()->buffer;
        active = 5;
        for (int i = 0; i < MAX_TTY; i++) {
            if (i == active) continue;
            tty[i]->buffer = dummy_buffers[i];
        }
        fb_blank(BLACK);
    }
    // event_handler->unregister_listener(KEYBOARD_EVENT, tty_key_listener._id);
}

/**
 * @brief Setup and run TTY system, register keyboard event handler.
 */
void run_tty() {
    // Initialize key listener
    tty_key_listener = {.handler = (void (*)(key_event *))&change_tty, .next = nullptr};

    // Register listener for KEYBOARD_EVENT
    event_handler->register_listener(KEYBOARD_EVENT, &tty_key_listener);

    // event_handler->unregister_listener(KEYBOARD_EVENT, tty_key_listener._id);
}
