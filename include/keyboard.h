#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "atomic.h"
#include "dwc.h"
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

struct keyboard {
    usb_device device_state;
    struct key_map {
        SpinLock lock;
        uint8_t bitmap[32];
    } keymap;
};

extern struct keyboard usb_kbd;

char get_keyboard_input();
int keyboard_attach(usb_session *session, usb_device_descriptor_t *device_descriptor,
                    usb_device_config_t *device_config);
int keyboard_detach();
#endif