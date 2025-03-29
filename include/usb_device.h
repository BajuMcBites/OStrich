#ifndef _USB_DEVICE_H
#define _USB_DEVICE_H
#include "dwc.h"
#include "listener.h"

#define HID_PROTOCOL_KEYBOARD 0x01
#define HID_PROTOCOL_MOUSE 0x02

#define HID_SET_PROTOCOL 0x0B
#define HID_BOOT_PROTOCOL 0x00

/*
When creating a new USB device, do this:
struct my_usb_device {
    struct usb_device usb_state;
    .... my_usb_device specific state.
}
*/
typedef struct {
    // General USB Device State
    bool discovered;
    bool connected;

    // DWC State
    uint8_t device_address;
    uint8_t interface_num;
    uint8_t bmAttributes;
    uint8_t in_ep;
    uint8_t out_ep;
    uint8_t interval_ms;
    uint8_t mps;
    uint8_t channel_num = UINT8_MAX;

    // HID State.
    uint8_t bInterfaceProtocol;

} usb_device;


void init_usb_session(usb_session *session, usb_device *device_state, int mps);
void iterate_config_for_hid(uint8_t *buffer, uint16_t length, usb_device *state,
                            int dev_bInterfaceProtocol);
int hid_device_attach(usb_session *session, usb_device_descriptor_t *device_descriptor,
                      usb_device_config_t *device_config, usb_device *device_state,
                      int dev_bInterfaceProtocol);
int hid_device_detach();

#endif