#include "keyboard.h"

#include "peripherals/dwc.h"
#include "printf.h"
#include "utils.h"

keyboard kbd;

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

void iterate_config(uint8_t *buffer, uint16_t length) {
    if (kbd.discovered) return;

    int index = 0;
    kbd.interface_num = 255;
    while (index < length) {
        int bLength = buffer[index + 0];
        int bDescriptorType = buffer[index + 1];

        if (bDescriptorType == 0x4) {
            int bInterfaceNumber = buffer[index + 2];
            int bInterfaceClass = buffer[index + 5];
            int bInterfaceSubClass = buffer[index + 6];
            int bInterfaceProtocol = buffer[index + 7];
            if (bInterfaceClass == 0x03 && bInterfaceSubClass == 0x01 &&
                bInterfaceProtocol == 0x01) {
                kbd.interface_num = bInterfaceNumber;
            }

        } else if (bDescriptorType == 0x5) {
            int bEndpointAddress = buffer[index + 2];
            int bmAttributes = buffer[index + 3];
            int wMaxPacketSize = buffer[index + 4] | ((uint16_t)buffer[index + 5] << 8);
            int bInterval = buffer[index + 6];
            if (bmAttributes == 0x3 && kbd.interface_num != 255) {
                kbd.bmAttributes = bmAttributes;
                kbd.in_ep = bEndpointAddress & (~(1 << 7));
                kbd.mps = wMaxPacketSize;
                kbd.interval = bInterval;
                kbd.discovered = true;
            }
        }
        index += bLength;
    }
}

void init_usb_session(usb_session *session) {
    session->device_address = kbd.device_address;
    session->mps = kbd.mps;
    session->in_ep_num = kbd.in_ep;
    session->channel = USB_CHANNEL(2);
}

char get_keyboard_input() {
    if (!kbd.discovered || !kbd.connected) return '\0';

    uint8_t buffer[8];
    usb_session session;
    init_usb_session(&session);

    if (usb_interrupt_in_transfer(&session, buffer, 8)) return '\0';
    return process_keyboard_report(buffer, 8);
}

int keyboard_detach() {
    kbd.connected = false;
}

int keyboard_attach(usb_session *session, usb_device_descriptor_t *device_descriptor,
                    usb_device_config_t *device_config) {
    uint8_t buffer[device_config->total_length];
    usb_get_device_config_descriptor(session, buffer, device_config->total_length);

    iterate_config(buffer, device_config->total_length);

    if (!kbd.discovered) {
        printf("failed to find correct device configurations.\n");
        return 1;
    }
    kbd.connected = true;

    usb_setup_packet_t setup;
    setup.bmRequestType = 0x21;
    setup.bRequest = HID_SET_PROTOCOL;
    setup.wValue = HID_BOOT_PROTOCOL;
    setup.wIndex = kbd.interface_num;
    setup.wLength = 0;

    kbd.device_address = session->device_address;

    init_usb_session(session);

    if (usb_handle_transfer(session, &setup, nullptr, 0)) {
        printf("set boot protocol failed\n");
        return 1;
    }

    return 0;
}