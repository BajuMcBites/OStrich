#ifndef _DWC_H
#define _DWC_H

#include <stdint.h>

typedef struct {
    uint8_t length;
    uint8_t descriptor_type;
    uint16_t total_length;
    uint8_t num_configurations;
    uint8_t configuration_value;
    uint8_t configuration_index;
    uint8_t bm_attributes;
    uint8_t max_power;
} __attribute__((packed, aligned(4))) usb_device_config_t;

typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bNbrPorts;
    uint16_t wHubCharacteristics;
    uint8_t bPwrOn2pwrGood;
    uint8_t bHubContrCurrent;
} __attribute__((packed, aligned(4))) usb_hub_descriptor_t;

// https://www.intel.com/content/www/us/en/programmable/hps/stratix-10/hps.html#agi1505406227399.html
typedef struct {
    union characteristics_u {
        uint32_t raw;
        struct characteristics {
            uint16_t mps : 11;
            uint8_t ep_num : 4;
            bool ep_dir : 1;
            bool _reserved : 1;
            bool low_speed : 1;
            uint8_t ep_type : 2;
            uint8_t ec : 2;
            uint8_t device_address : 7;
            bool odd_frame : 1;
            bool disable : 1;
            bool enable : 1;
        } __attribute__((packed, aligned(4))) st;
    } characteristics;
    uint32_t split_control;
    union interrupts_u {
        uint32_t raw;
        struct interrupts {
            bool transfer_complete : 1;
            bool halt : 1;
            bool abh_error : 1;
            bool stall : 1;
            bool negative_ack : 1;
            bool ack : 1;
            bool not_yet : 1;
            bool transaction_err : 1;
            bool babble_err : 1;
            bool frame_overrun : 1;
            bool data_tgl_err : 1;
            bool buffer_not_avail : 1;
            bool excessive_trans : 1;
            bool frame_list_rollover : 1;
            unsigned _reserved14_31 : 18;
        } __attribute__((packed, aligned(4))) st;
    } interrupt;
    uint32_t interrupt_mask;
    union transfer_size_u {
        uint32_t raw;
        struct transfer_size {
            uint32_t xfer_size : 19;
            uint16_t pck_cnt : 10;
            uint8_t pid : 2;
            bool do_ping : 1;
        } __attribute__((packed, aligned(4))) st;
    } transfer_size;
    // uint32_t transfer_size;
    uint64_t dma_address;
    uint32_t dma_buffer;
} __attribute__((packed, aligned(4))) host_channel;

typedef struct {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} __attribute__((packed, aligned(4))) usb_setup_packet_t;

typedef struct {
    uint16_t status;
    uint16_t change;
} usb_port_status_t;

typedef struct {
    uint8_t length;           //
    uint8_t descriptor_type;  //
    uint16_t usb_version;     //
    uint8_t device_class;     //
    uint8_t device_subclass;  //
    uint8_t device_protocol;  //
    uint8_t max_packet_size;  //
    uint16_t vendor_id;       //
    uint16_t product_id;      //
    uint16_t device_version;
    uint8_t manufacturer_index;
    uint8_t product_index;
    uint8_t serial_number_index;
    uint8_t num_configurations;
} __attribute__((packed, aligned(4))) usb_device_descriptor_t;

enum EndpointDirection { HostToDevice = 0, Out = 0, DeviceToHost = 1, In = 1 };

typedef struct {
    host_channel *channel;
    uint8_t device_address = 0;
    uint8_t port;
    uint8_t in_ep_num = 0;
    uint8_t out_ep_num = 0;
    uint8_t low_speed : 1 = false;
    uint16_t mps : 10 = 8;
} usb_session;

typedef struct {
    uint8_t num_ports;
    uint8_t device_address;
    uint8_t low_speed : 1 = 1;
} __attribute__((packed, aligned(4))) usb_hub;

typedef struct {
    usb_hub connected[64];
    uint8_t hub_count;
} usb_connected_hubs;

typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t bInterfaceSetting;
    uint8_t iInterface;
} usb_device_interface_config_t;
extern usb_connected_hubs hubs;

void usb_initialize();
void usb_interrupt_handler();
int get_interval_ms_for_interface(usb_session *, uint32_t);
int usb_get_device_config_descriptor(usb_session *session, uint8_t *buffer, uint16_t length = 18);
void usleep(unsigned int microseconds);
int usb_handle_transfer(usb_session *, usb_setup_packet_t *, uint8_t *, int);
int usb_interrupt_in_transfer(usb_session *session, uint8_t *buffer, int length);
void usb_irq_handler();

#endif