#include "usb_device.h"

#include "dwc.h"
#include "keyboard.h"
#include "libk.h"
#include "peripherals/dwc.h"
#include "printf.h"

EventHandler *event_handler;
int CHANNEL_NUM = 2;

void init_usb_session(usb_session *session, usb_device *device_state) {
    session->device_address = device_state->device_address;
    session->mps = device_state->mps;
    session->in_ep_num = device_state->in_ep;
    if (device_state->channel_num == UINT8_MAX) device_state->channel_num = CHANNEL_NUM++;
    K::assert(device_state->channel_num < 8, "too many channels");  // uh oh.
    session->channel = USB_CHANNEL(device_state->channel_num);
}

void iterate_config_for_hid(uint8_t *buffer, uint16_t length, usb_device *device_state,
                            int dev_bInterfaceProtocol) {
    if (device_state->discovered) return;

    int index = 0;
    device_state->interface_num = 255;
    while (index < length) {
        int bLength = buffer[index + 0];
        int bDescriptorType = buffer[index + 1];

        if (bDescriptorType == 0x4 /* interface descriptor */) {
            int bInterfaceNumber = buffer[index + 2];
            int bInterfaceClass = buffer[index + 5];
            int bInterfaceSubClass = buffer[index + 6];
            int bInterfaceProtocol = buffer[index + 7];
            if (bInterfaceClass == 0x03 /* HID */ &&
                bInterfaceSubClass == 0x01 /* boot subclass */ &&
                bInterfaceProtocol == dev_bInterfaceProtocol) {
                device_state->interface_num = bInterfaceNumber;
            }

        } else if (bDescriptorType == 0x5 /* endpoint descriptor */) {
            int bEndpointAddress = buffer[index + 2];
            int bmAttributes = buffer[index + 3];
            int wMaxPacketSize = buffer[index + 4] | ((uint16_t)buffer[index + 5] << 8);
            int bInterval = buffer[index + 6];
            if (bmAttributes == 0x3 /* interrupt */ && device_state->interface_num != 255) {
                printf("found expected endpoint attributes.\n");
                device_state->bmAttributes = bmAttributes;
                device_state->in_ep =
                    bEndpointAddress & (~(1 << 7));  // Turn off bit 8 to make it an input endpoint.
                device_state->mps = wMaxPacketSize;
                device_state->interval = bInterval;
                device_state->discovered = true;
            }
        }
        index += bLength;
    }
}
int hid_device_attach(usb_session *session, usb_device_descriptor_t *device_descriptor,
                      usb_device_config_t *device_config, usb_device *device_state,
                      int dev_bInterfaceProtocol) {
    uint8_t buffer[device_config->total_length];
    usb_get_device_config_descriptor(session, buffer, device_config->total_length);

    iterate_config_for_hid(buffer, device_config->total_length, device_state,
                           dev_bInterfaceProtocol);

    if (!device_state->discovered) {
        printf("failed to find correct device configurations.\n");
        return 1;
    }

    device_state->connected = true;

    usb_setup_packet_t setup;
    setup.bmRequestType = 0x21 /* Host to Device, something else? */;
    setup.bRequest = HID_SET_PROTOCOL;
    setup.wValue = HID_BOOT_PROTOCOL;
    setup.wIndex = device_state->interface_num;
    setup.wLength = 0;

    device_state->device_address = session->device_address;

    init_usb_session(session, device_state);

    int ret = usb_handle_transfer(session, &setup, nullptr, 0);
    if (ret) {
        printf("set boot protocol failed: %d\n", ret);
        return 1;
    }

    printf("detected HID device!\n");

    return 0;
}

int hid_device_dettach(usb_device *device_state) {
    device_state->connected = false;
}

#include "event.h"

// void handle_key_event(key_event *event) {
//     printf("handling keyboard event! modifiers = 0x%08x, keycode = 0x%08x\n", event->modifiers,
//            event->keycode);
// }

void init_hid_devices() {
    printf("init_hid_devices\n");
    event_handler = new EventHandler{};

    // struct Listener<struct key_event *> *listener =
    //     (Listener<struct key_event *> *)kmalloc(sizeof(Listener<struct key_event *>));

    // listener->handler = (void (*)(key_event *))handle_key_event;

    // struct key_event event;
    // event.modifiers = 0x69;
    // event.keycode = 0x96;

    // // listener->handler(&event);

    // eventHandler->register_listener(KEYBOARD_EVENT, listener);
    // eventHandler->handle_event(KEYBOARD_EVENT, &event);
}
