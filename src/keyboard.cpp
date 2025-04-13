#include "keyboard.h"

#include "event.h"
#include "peripherals/dwc.h"
#include "printf.h"
#include "usb_device.h"
#include "utils.h"

keyboard usb_kbd;

uint64_t get_keyboard_input() {
    if (!usb_kbd.device_state.discovered || !usb_kbd.device_state.connected) {
        return '\0';
    }

    uint8_t buffer[8];
    usb_session session;
    init_usb_session(&session, &usb_kbd.device_state, usb_kbd.device_state.mps);

    if (usb_interrupt_in_transfer(&session, buffer, 8)) {
        return '\0';
    }
    return *((uint64_t *)buffer);
}

void keyboard_loop() {
    uint64_t input, prior;
    uint8_t keyboard_state[256];
    for (int i = 0; i < 256; i++) keyboard_state[i] = 0x00;

    struct key_event events[12];
    uint8_t event_cnt = 0;
    uint8_t modifiers, prior_modifiers, keycode;

    while (1) {
        input = get_keyboard_input();
        modifiers = input & 0xFF;

        if (input != 0x00) {
            for (int i = 2; i < 8; i++) {
                keycode = (input >> (i << 3)) & 0xFF;
                if (keycode != 0x00) {
                    if (keyboard_state[keycode] == 0) {
                        keyboard_state[keycode] = 0b01;

                        events[event_cnt].modifiers = modifiers;
                        events[event_cnt].keycode = keycode;
                        events[event_cnt].flags.pressed = true;
                        events[event_cnt].flags.released = false;

                        event_cnt++;
                    } else {
                        keyboard_state[keycode] |= 0b10;
                    }
                }
            }
        }
        if (prior != 0x00) {
            for (int i = 2; i < 8; i++) {
                keycode = (prior >> (i << 3)) & 0xFF;
                if (keycode != 0x00) {
                    if ((keyboard_state[keycode] & 0b11) == 0b01) {
                        keyboard_state[keycode] = 0b00;

                        events[event_cnt].modifiers = prior_modifiers;
                        events[event_cnt].keycode = keycode;
                        events[event_cnt].flags.pressed = false;
                        events[event_cnt].flags.released = true;

                        event_cnt++;
                    }
                    keyboard_state[keycode] &= ~0b10;
                }
            }
        }

        for (int i = 0; i < event_cnt; i++) {
            get_event_handler().handle_event(KEYBOARD_EVENT, &events[i]);
        }

        prior = input;
        prior_modifiers = modifiers;
        event_cnt = 0;
    }
}
