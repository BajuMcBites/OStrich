#ifndef _MOUSE_H
#define _MOUSE_H

#include "dwc.h"
#include "usb_device.h"

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