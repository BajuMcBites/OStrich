#include "tty.h"

#include "dwc.h"
#include "event.h"
#include "frame_buff_list.h"
#include "framebuffer.h"
#include "libk.h"
#include "listener.h"
#include "peripherals/timer.h"
#include "peripherals/usb_hid_keys.h"
#include "printf.h"
#include "timer.h"

static FrameBufferLinkedList tty;
static Listener<struct key_event *> tty_key_listener;

static SpinLock lock;

static unsigned int *dummy_buffers[6] = {nullptr};
static unsigned int *dummy_buffer = nullptr;

int active = -1;  // index of window in foreground
// Framebuffer *tty[MAX_TTY] = {nullptr};
bool taken[MAX_TTY] = {false};
/**
 * @brief Initialize tty. Limited to six slots
 * @param none
 */
void init_tty() {
    lock.lock();
    tty.insert(get_kernel_fb());

    taken[0] = true;
    dummy_buffers[0] = new unsigned int[tty.getHead()->size / sizeof(unsigned int)];
    dummy_buffer = new unsigned int[tty.getHead()->size / sizeof(unsigned int)];
    auto temp = get_real_fb();
    // for (int i = 1; i < MAX_TTY; i++) {
    //     tty[i] = new Framebuffer;
    //     tty[i]->height = temp->height;
    //     tty[i]->isrgb = temp->isrgb;
    //     tty[i]->pitch = temp->pitch;
    //     tty[i]->size = temp->size;
    //     tty[i]->width = temp->width;
    //     dummy_buffers[i] = dummy_buffers[0];
    // }
    active = 0;
    lock.unlock();
}

/**
 * @brief Return a tty if one is available
 * @param none
 */
Framebuffer *request_tty() {
    lock.lock();
    Framebuffer *buff = new Framebuffer;
    auto real_fb = get_real_fb();
    buff->height = real_fb->height;
    buff->isrgb = real_fb->isrgb;
    buff->pitch = real_fb->pitch;
    buff->size = real_fb->size;
    buff->width = real_fb->width;

    tty.getHead()->buffer = dummy_buffer;
    buff->buffer = real_fb->buffer;
    tty.insert(buff);
    fb_blank(BLUE);
    lock.unlock();
    return buff;
}

/**
 * @brief get this threads's FB and remove from list
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
    lock.lock();
    if (event->flags.released) {
        lock.unlock();
        return;
    }

    if (event->keycode == KEY_F2) {
        // move forward
        tty.getHead()->buffer = dummy_buffer;
        tty.moveHeadForward();
        tty.getHead()->buffer = get_real_fb()->buffer;
        fb_blank(WHITE);
    } else if (KEY_F1) {
        // move back
        tty.getHead()->buffer = dummy_buffer;
        tty.moveHeadBack();
        tty.getHead()->buffer = get_real_fb()->buffer;
        fb_blank(WHITE);
    }
    lock.unlock();
}

/**
 * @brief Setup and run TTY system, register keyboard event handler.
 */
void run_tty() {
    // Initialize key listener
    tty_key_listener = {.handler = (void (*)(key_event *))&change_tty, .next = nullptr};

    // Register listener for KEYBOARD_EVENT
    event_handler->register_listener(KEYBOARD_EVENT, &tty_key_listener);
}
