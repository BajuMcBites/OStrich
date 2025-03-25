#include "keyboard.h"

#include "peripherals/dwc.h"
#include "printf.h"
#include "usb_device.h"
#include "utils.h"

keyboard usb_kbd;

char keycode_to_ascii(uint8_t keycode, int shift) {
    if (keycode < 128) {
        if (keycode == 0x28) {
            return '\n';
        } else if (keycode == 0x2C) {
            return ' ';
        } else if (keycode >= 0x4 && keycode <= 0x1D) {
            return shift ? 'A' + (keycode - 0x4) : 'a' + (keycode - 0x4);
        } else if (keycode >= 0x1E && keycode <= 0x27) {
            return '1' + (keycode - 0x1E);
        }
    }
    return '\0';
}

// uint8_t handle_keycode(uint8_t modifiers, uint8_t code) {
//     keyboard::key_map *keymap = kbd.key_map;
//     keymap.lock.lock();

//     uint8_t index = code >> 3;
//     uint8_t offset = code % 8;

//     uint8_t old = keymap->bitmap[index] & (1 << offset);
//     if (old) {
//         // status = pressed, changing to released
//         keymap->bitmap[index] &= ~(1 << offset);
//     } else {
//         // status = released, changing to pressed
//         keymap->bitmap[index] |= (1 << offset);
//     }

//     keymap.lock.unlock();
//     return !old;
// }

char process_keyboard_report(uint8_t *report, int len) {
    if (len < 8) return '\0';  // Standard report is 8 bytes

    uint8_t modifier = report[0];
    uint8_t shift = (modifier & (LEFT_SHIFT | RIGHT_SHIFT)) ? 1 : 0;

    char random = '\0';
    for (int i = 2; i < 8; i++) {
        uint8_t keycode = report[i];

        if (keycode < 0x4 || keycode > 0x1D) continue;
        // fix by keeping track of all key state, this just so happens to work
        random = keycode_to_ascii(keycode, shift);
    }
    return random;
}

char get_keyboard_input() {
    if (!kbd.device_state.discovered || !kbd.device_state.connected) return '\0';

    uint8_t buffer[8];
    usb_session session;
    init_usb_session(&session, &kbd.device_state, kbd.device_state.mps);

    if (usb_interrupt_in_transfer(&session, buffer, 8)) return '\0';
    return process_keyboard_report(buffer, 8);
}
