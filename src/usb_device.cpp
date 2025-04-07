#include "usb_device.h"

#include "dwc.h"
#include "event.h"
#include "keyboard.h"
#include "libk.h"
#include "peripherals/dwc.h"
#include "printf.h"

void init_usb_session(usb_session *session, usb_device *device_state, int mps) {
    session->device_address = device_state->device_address;
    session->mps = mps;
    session->in_ep_num = device_state->in_ep;
    if (device_state->channel_num == 0) device_state->channel_num = request_new_usb_channel_num();
    session->channel = USB_CHANNEL(device_state->channel_num);
}

void iterate_config_for_hid(uint8_t *buffer, uint16_t length, usb_device *device_state,
                            int dev_bInterfaceProtocol) {
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
                device_state->bmAttributes = bmAttributes;
                device_state->in_ep =
                    bEndpointAddress & (~(1 << 7));  // Turn off bit 8 to make it an input endpoint.
                device_state->mps = wMaxPacketSize;
                device_state->interval_ms = bInterval;
                device_state->discovered = true;
            }
        }
        index += bLength;
    }
}
int hid_device_attach(usb_session *session, usb_device_descriptor_t *device_descriptor,
                      usb_device_config_t *device_config, usb_device *device_state,
                      int dev_bInterfaceProtocol) {
    printf("Attaching device with interface protocol: %d\n", dev_bInterfaceProtocol);
    if (device_state->connected || device_state->discovered) {
        printf("Device already connected/discovered.\n");
        return 0;
    }

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
    setup.bmRequestType = 0x21 /* Class-specific request, Host to Device */;
    setup.bRequest = HID_SET_PROTOCOL;
    setup.wValue = HID_BOOT_PROTOCOL;
    setup.wIndex = device_state->interface_num;
    setup.wLength = 0 /* don't change this */;

    device_state->device_address = session->device_address;

    init_usb_session(session, device_state, 8 /* don't change this */);

    int ret = usb_handle_transfer(session, &setup, nullptr, 0 /* don't change this */);
    if (ret) {
        printf("set boot protocol failed: %d\n", ret);
        return 1;
    }

    return 0;
}

int hid_device_dettach(usb_device *device_state) {
    device_state->connected = false;
}