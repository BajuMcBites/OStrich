#ifndef _DWC_H
#define _DWC_H

#include <stdint.h>

typedef struct {
    uint8_t length;
    uint8_t descriptor_type;
    uint16_t total_length;
    uint8_t num_interfaces;
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
    uint8_t out_toggle = 0;
    uint8_t in_toggle = 0;
    uint16_t mps : 10 = 8;
    bool out_data : 1 = false;
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
    union ctrl_u {
        struct out_ctrl {
            uint16_t mps : 11;
            uint8_t reserved0 : 4;
            bool usb_active_endpoint : 1;
            bool dpid : 1;
            bool nak_status : 1;
            uint8_t ep_type : 2;
            bool snp : 1;
            bool stall : 1;
            uint8_t reserved1 : 4;
            bool clear_nak : 1;
            bool set_nak : 1;
            bool set_d0pid : 1;
            bool set_d1pid : 1;
            bool disabled : 1;
            bool enabled : 1;
        } out_ctrl;
        struct in_ctrl {
            uint8_t mps : 2;
            uint16_t _reserved2_14 : 13;
            bool usb_active_endpoint : 1;
            bool _reserved16 : 1;
            bool nak_status : 1;
            uint8_t ep_type : 2;
            bool snp : 1;
            bool stall : 1;
            uint8_t reserved1 : 4;
            bool clear_nak : 1;
            bool set_nak : 1;
            uint8_t _reserved28_29 : 2;
            bool disabled : 1;
            bool enabled : 1;
        } in_ctrl;
    } control;
    uint32_t _reserved4_7;
    union interrupts_u {
        struct out_interrupts {
            bool transfer_complete : 1;
            bool ep_disabled : 1;
            bool abh_error : 1;
            bool setup_done : 1;
            bool out_token_recv : 1;
            bool status_recv : 1;
            bool b2b_setup_recv : 1;
            bool out_packet_err : 1;
            bool buffer_not_avail : 1;
            bool packet_drop_status : 1;
            bool babble_err : 1;
            bool nak_int : 1;
            bool nyet_int : 1;
            bool setup_packet_recv : 1;
            unsigned _reserved14_31 : 18;
        } __attribute__((packed)) out_int;
        struct in_interrupts {
            bool transfer_complete : 1;
            bool ep_disabled : 1;
            bool abh_error : 1;
            bool timeout : 1;
            bool in_token_recv : 1;  // @4
            bool recv_mismatch : 1;
            bool in_ep_nak : 1;
            bool tx_fifo_empty : 1;
            bool tx_fifo_underrun : 1;
            bool buffer_not_avail : 1;  // @9
            bool _reservered : 1;
            bool packet_drop_status : 1;  // @11
            bool babble_err : 1;
            bool nak : 1;
            bool nyet : 1;
            unsigned _reserved14_31 : 18;
        } __attribute__((packed)) in_int;
    };
    union transfer_size_u {
        struct out_transfer_size {
            uint32_t xfer_size : 19;
            uint16_t packet_count : 10;
            uint8_t rx_dpid : 2;
            uint16_t _reserved31 : 1;
        } __attribute__((packed)) out_tranfser_size;
        struct in_transfer_size {
            uint8_t xfer_size : 7;
            uint16_t _reserved7_15 : 9;
            uint8_t _resvered16_18 : 3;
            uint8_t packet_count : 2;
            uint16_t _reserved21_31 : 11;
        } __attribute__((packed)) in_tranfser_size;
    };

    uint32_t dma_address;
    uint32_t _reserved8_b;
    uint32_t dma_buffer;
} __attribute__((packed)) endpoint;

typedef struct {
    uint32_t buffer_address;
    uint32_t length;
    uint32_t status;
} transfer_descriptor_t;

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

int usb_handle_transfer(usb_session *, usb_setup_packet_t *, uint8_t *, int);
int usb_receive_bulk(usb_session *, uint8_t *, size_t);
int usb_send_bulk(usb_session *, uint8_t *, size_t);
int usb_get_configuration(usb_session *session, uint8_t *buffer);
int usb_get_device_config_descriptor(usb_session *session, uint8_t *buffer, uint16_t length = 18);
int usb_get_device_descriptor(usb_session *, uint8_t *);
int usb_set_configuration(usb_session *, uint8_t);
int usb_receive_data(usb_session *session, uint8_t *buffer, uint16_t length);
int usb_interrupt_in_transfer(usb_session *session, uint8_t *buffer, int length);
int request_new_usb_channel_num();
int handle_transaction(usb_session *session, uint8_t stage);
int usb_send_token_in(usb_session *session, uint8_t *buffer);
int usb_receive_in_data(usb_session *session, uint8_t *buffer, size_t length);
int usb_send_setup_packet(usb_session *session, usb_setup_packet_t *setup, int length);

#endif