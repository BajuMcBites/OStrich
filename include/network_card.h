#ifndef _NETWORK_CARD_H
#define _NETWORK_CARD_H

#include <stdint.h>

#include "dwc.h"

#define USB_DESC_TYPE_INTERFACE 0x04
#define USB_DESC_TYPE_ENDPOINT 0x05

#define USB_ENDPOINT_TYPE_MASK 0x03
#define USB_ENDPOINT_TYPE_BULK 0x02

#define USB_ENDPOINT_DIR_MASK 0x80
#define USB_ENDPOINT_DIR_IN 0x80
#define TOTAL_CONFIG_ELEMENTS 4

#define MAX_BUFFER_SIZE 1514

typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
} __attribute__((packed)) EndpointDescriptor;

void initialize_network_card(usb_session *, usb_device_descriptor_t *, usb_device_config_t *);
void send_packet(usb_session *session, uint8_t *data, size_t length);
int receive_packet(usb_session *session, uint8_t *buffer, size_t buffer_len);
uint8_t *get_mac_address();

#endif