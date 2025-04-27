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
    // auto temp = get_real_fb();
    // for (int i = 1; i < MAX_TTY; i++) {
    //     tty[i] = new Framebuffer;
    //     tty[i]->height = temp->height;
    //     tty[i]->isrgb = temp->height;
    //     tty[i]->pitch = temp->height;
    //     tty[i]->size = temp->height;
    //     tty[i]->width - temp->width;
    // }
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
 */
void change_tty(struct key_event *event) {
    // printf("IN CHANGE TTY\n");
    if (event->flags.released) return;
    // for (int i = 0; i < MAX_TTY; i++) {
    //     // if I have a dummy buffer, delete
    //     if (tty[i]->buffer != nullptr && tty[i]->buffer != get_real_fb()->buffer) {
    //         delete tty[i]->buffer;
    //     }
    //     tty[i]->buffer = new uint32_t;
    // }

    if (event->keycode == KEY_0) {
        printf("TERM 0\n");
        tty[0]->buffer = get_real_fb()->buffer;
        active = 0;
    } else if (event->keycode == KEY_1) {
        printf("TERM 1\n");
        tty[1]->buffer = get_real_fb()->buffer;
        active = 1;
    } else if (event->keycode == KEY_F3) {
        printf("TERM 2\n");
        tty[2]->buffer = get_real_fb()->buffer;
        active = 2;
    } else if (event->keycode == KEY_F4) {
        printf("TERM 3\n");
        tty[3]->buffer = get_real_fb()->buffer;
        active = 3;
    } else if (event->keycode == KEY_F5) {
        printf("TERM 4\n");
        tty[4]->buffer = get_real_fb()->buffer;
        active = 4;
    } else if (event->keycode == KEY_F6) {
        printf("TERM 5\n");
        tty[5]->buffer = get_real_fb()->buffer;
        active = 5;
    }
}

void run_tty() {
    int i = 0;
    while (1) {
        // printf("i: %d\n", i++);
        tty_key_listener = {.handler = (void (*)(key_event *))&change_tty, .next = nullptr};
        event_handler->register_listener(KEYBOARD_EVENT, &tty_key_listener);
        // event_handler->unregister_listener(KEYBOARD_EVENT, tty_key_listener._id);
        wait_msec(10000);
        // printf("post wait\n");
    }
    // create_event(run_tty);
}
