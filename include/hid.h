#ifndef _HID_H
#define _HID_H

#include <stdint.h>

#include "dwc.h"
#include "peripherals/dwc.h"

// HID Descriptor Codes
#define USB_HID_DESCRIPTOR_TYPE 0x21
#define USB_HID_REPORT_DESCRIPTOR_TYPE 0x22

// HID request types.
#define HID_DEVICE_TO_HOST \
    SET_REQUEST_TYPE(TRANSFER_DIRECTION_DEVICE_TO_HOST, RECIPIENT_INTERFACE, REQUEST_TYPE_CLASS)
#define HID_HOST_TO_DEVICE \
    SET_REQUEST_TYPE(TRANSFER_DIRECTION_HOST_TO_DEVICE, RECIPIENT_INTERFACE, REQUEST_TYPE_CLASS)

// HID class-specific requests
#define USB_HID_GET_REPORT 0x01
#define USB_HID_GET_IDLE 0x02
#define USB_HID_GET_PROTOCOL 0x03
#define USB_HID_SET_REPORT 0x09
#define USB_HID_SET_IDLE 0x0A
#define USB_HID_SET_PROTOCOL 0x0B

// HID Interface Class Codes
#define USB_HID_INTERFACE_CLASS 0x03
#define USB_HID_INTERFACE_SUBCLASS_BOOT 0x01
#define USB_HID_INTERFACE_PROTOCOL_MOUSE 0x02

// HID endpoint types
#define USB_HID_ENDPOINT_TYPE_INTERRUPT 0x03
#define USB_HID_IN_ENDPOINT 0x80

// Standard mouse button definitions
#define MOUSE_BUTTON_LEFT (1 << 0)
#define MOUSE_BUTTON_RIGHT (1 << 1)
#define MOUSE_BUTTON_MIDDLE (1 << 2)

// HID descriptor structure
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdHID;
    uint8_t bCountryCode;
    uint8_t bNumDescriptors;
    uint8_t bDescriptorType2;
    uint16_t wDescriptorLength;
} __attribute__((packed)) usb_hid_descriptor_t;

// Mouse report structure (based on standard boot protocol)
typedef struct {
    uint8_t buttons;
    int8_t x;
    int8_t y;
    int8_t wheel;  // Optional, not in all mice
} __attribute__((packed)) mouse_report_t;

// Mouse device structure
typedef struct {
    usb_session *session;
    uint8_t interface_num;
    uint8_t endpoint_address;
    uint32_t interval_ms;
    mouse_report_t report;
    bool connected;
    uint16_t max_packet_size;
} mouse_device_t;

// Mouse driver functions
int mouse_detect(usb_session *session, usb_device_descriptor_t *dev_desc,
                 usb_device_config_t *config);
int mouse_get_report(mouse_device_t *mouse);
int mouse_set_protocol(mouse_device_t *mouse, uint8_t protocol);
int mouse_set_idle(mouse_device_t *mouse, uint8_t duration, uint8_t report_id);
void mouse_process_input(mouse_device_t *mouse);

// Mouse event handlers
void mouse_handle_movement(int8_t x, int8_t y, int8_t wheel, uint8_t buttons);

// External variables
extern mouse_device_t USB_MOUSE;

#endif  // _HID_H