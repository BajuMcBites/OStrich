#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "atomic.h"
#include "dwc.h"
#include "usb_device.h"
#include "usb_device.h"

#define HID_SET_PROTOCOL 0x0B
#define HID_BOOT_PROTOCOL 0x00

#define LEFT_CTRL 0
#define LEFT_SHIFT 1
#define LEFT_ALT 2
#define LEFT_GUI 3
#define RIGHT_CTRL 4
#define RIGHT_SHIFT 5
#define RIGHT_ALT 6
#define RIGHT_GUI 7

struct key_event {
    uint8_t modifiers;
    uint8_t keycode;
    struct key_flags {
        bool pressed : 1;
        bool released : 1;
    } flags;
};

struct keyboard {
    usb_device device_state;
};

extern struct keyboard usb_kbd;

uint64_t get_keyboard_input();
void keyboard_loop();

#endif