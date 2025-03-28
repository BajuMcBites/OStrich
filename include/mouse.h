#ifndef _MOUSE_H
#define _MOUSE_H

#include "dwc.h"
#include "usb_device.h"

/*
Use this function to poll the mouse and keyboard:
void poll_keyboard_mouse() {
    while (1) {
        wait_msec(100000); // 100ms
        if (get_mouse_input()) {
            printf("mouse err\n");
        }
        printf("mouse x: %d, y: %d, buttons: %x\n", usb_mouse.mouse_state.x,
usb_mouse.mouse_state.y, usb_mouse.mouse_state.buttons); char c = get_keyboard_input(); if (c ==
'\0') { printf("keyboard err\n");
        }
        printf("key read: %c\n", c);
    }
}
*/

struct mouse {
    usb_device device_state;
    struct mouse_state {
        uint8_t buttons;
        uint8_t x;
        uint8_t y;
    } mouse_state;
};

extern struct mouse usb_mouse;

int get_mouse_input();

#endif