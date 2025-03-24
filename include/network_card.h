#ifndef _NETWORK_CARD_H
#define _NETWORK_CARD_H

#include <stdint.h>

#include "dwc.h"

/* USB request codes for RNDIS */
#define SEND_ENCAPSULATED_COMMAND 0x00
#define GET_ENCAPSULATED_RESPONSE 0x01

/* USB class, subclass, protocol (RNDIS specific) */
#define RNDIS_USB_CLASS 0xE0
#define RNDIS_USB_SUBCLASS 0x01
#define RNDIS_USB_PROTOCOL 0x03

/* RNDIS message types */
#define RNDIS_INITIALIZE_MSG 0x00000002
#define RNDIS_INITIALIZE_CMPLT 0x80000002
#define RNDIS_SET_MSG 0x00000005
#define RNDIS_SET_CMPLT 0x80000005
#define RNDIS_QUERY_MSG 0x00000004
#define RNDIS_QUERY_CMPLT 0x80000004
#define RNDIS_KEEPALIVE_MSG 0x00000008
#define RNDIS_KEEPALIVE_CMPLT 0x80000008
#define RNDIS_PACKET_MSG 0x00000001

/* RNDIS OIDs (Object IDs) */
#define OID_GEN_CURRENT_PACKET_FILTER 0x0001010E
#define OID_802_3_CURRENT_ADDRESS 0x01010102

/* Packet filter types */
#define PACKET_TYPE_DIRECTED 0x00000001
#define PACKET_TYPE_MULTICAST 0x00000002
#define PACKET_TYPE_BROADCAST 0x00000004

#define USB_DESC_TYPE_INTERFACE 0x04
#define USB_DESC_TYPE_ENDPOINT 0x05

#define USB_ENDPOINT_TYPE_MASK 0x03
#define USB_ENDPOINT_TYPE_BULK 0x02

#define USB_ENDPOINT_DIR_MASK 0x80
#define USB_ENDPOINT_DIR_IN 0x80
#define USB_ENDPOINT_DIR_OUT 0x00
#define MAX_ETHERNET_FRAME_SIZE 1522
#define TOTAL_CONFIG_ELEMENTS 4

#define USB_REQUEST_GET_DESCRIPTOR 0x06
#define USB_DESCRIPTOR_TYPE_STRING 0x03

#define USB_REQ_CLASS_INTERFACE 0x21
#define OID_802_3_PERMANENT_ADDRESS 0x01010101
#define REQUEST_TYPE_HOST_TO_DEVICE 0x80
#define REQUEST_TYPE_DEVICE_TO_HOST 0xC0

#define MAX_BUFFER_SIZE 1514

typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
} __attribute__((packed)) EndpointDescriptor;

// RNDIS Initialize Message
typedef struct __attribute__((packed, aligned(4))) {
    uint32_t MessageType;
    uint32_t MessageLength;
    uint32_t RequestID;
    uint32_t MajorVersion;
    uint32_t MinorVersion;
    uint32_t MaxTransferSize;
} RNDIS_InitializeMessage;

// RNDIS Initialize Complete
typedef struct __attribute__((packed, aligned(4))) {
    uint32_t MessageType;
    uint32_t MessageLength;
    uint32_t RequestID;
    uint32_t Status;
    uint32_t MajorVersion;
    uint32_t MinorVersion;
    uint32_t DeviceFlags;
    uint32_t Medium;
    uint32_t MaxPacketsPerTransfer;
    uint32_t MaxTransferSize;
    uint32_t PacketAlignmentFactor;
    uint32_t AFListOffset;
    uint32_t AFListSize;
} RNDIS_InitializeComplete;

// RNDIS Set Message
typedef struct __attribute__((packed, aligned(4))) {
    uint32_t MessageType;
    uint32_t MessageLength;
    uint32_t RequestID;
    uint32_t OID;
    uint32_t InformationBufferLength;
    uint32_t InformationBufferOffset;
    uint32_t Reserved;
    uint32_t PacketFilter;
} RNDIS_SetMessage;

// RNDIS Query Message (optional, e.g., for MAC address)
typedef struct __attribute__((packed, aligned(4))) {
    uint32_t MessageType;
    uint32_t MessageLength;
    uint32_t RequestID;
    uint32_t OID;
    uint32_t InformationBufferLength;
    uint32_t InformationBufferOffset;
    uint32_t Reserved;
} RNDIS_QueryMessage;

void initialize_network_card(usb_session *, usb_device_descriptor_t *, usb_device_config_t *);
void send_packet(usb_session *session, uint8_t *data, size_t length);
int receive_packet(usb_session *session, uint8_t *buffer, size_t buffer_len);
uint8_t *get_mac_address();

#endif