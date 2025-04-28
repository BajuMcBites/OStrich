#include "network_card.h"

#include <stdint.h>

#include "arp.h"
#include "dhcp.h"
#include "dwc.h"
#include "event.h"
#include "ipv4.h"
#include "libk.h"
#include "net_stack.h"
#include "peripherals/dwc.h"
#include "printf.h"
#include "timer.h"

// Variables to store bulk endpoints
uint8_t control_interface;
uint8_t data_interface;
uint8_t bulk_in_ep = 0;
uint8_t bulk_out_ep = 0;
uint16_t bulk_in_mps = 0;
uint16_t bulk_out_mps = 0;
uint8_t mac_address[6];

void process_packet(usb_session *session, uint8_t *buffer, size_t len) {
    PacketBufferParser parser(buffer, len);

    auto frame = parser.pop<EthernetFrame>();
    switch (frame->ethertype.get()) {
        case 0x0806:  // ARP
            handle_arp_packet(session, &parser);
            break;
        case 0x0800:  // IPv4
            handle_ipv4_packet(session, &parser);
            break;
    }
}

void send_packet(uint8_t *data, size_t length) {
    if (length > MAX_BUFFER_SIZE) {
        printf("Error: Packet too large to send.\n");
        return;
    }

    usb_session *session = network_usb_send_session(nullptr);
    usb_send_bulk(session, data, length);
}

int receive_packet(uint8_t *buffer, size_t buffer_len) {
    return usb_receive_bulk(network_usb_recv_session(nullptr), buffer, buffer_len);
}

int parse_config(uint8_t *descriptor, size_t length) {
    size_t offset = 0;
    uint8_t current_interface = 0xFF;
    uint8_t current_class = 0x00;
    uint8_t current_subclass = 0x00;

    int found = 0;

    while (offset < length) {
        uint8_t bLength = descriptor[offset];
        uint8_t bDescriptorType = descriptor[offset + 1];

        if (bLength == 0) break;  // Safety
        if (bDescriptorType == USB_DESC_TYPE_INTERFACE) {
            current_interface = descriptor[offset + 2];
            current_class = descriptor[offset + 5];
            current_subclass = descriptor[offset + 6];
            if (current_class == 0x02) {
                control_interface = current_interface;
                found += 1;
            } else if (current_class == 0x0A) {
                data_interface = current_interface;
                found += 1;
            }
        }

        if (bDescriptorType == USB_DESC_TYPE_ENDPOINT && current_interface == 1 &&
            current_class == 0x0A) {
            EndpointDescriptor *ep = (EndpointDescriptor *)(descriptor + offset);

            if ((ep->bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_BULK) {
                if ((ep->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_DIR_IN) {
                    bulk_in_ep = ep->bEndpointAddress;
                    bulk_in_ep &= ~(0x80);  // Clear the direction bit
                    bulk_in_mps = ep->wMaxPacketSize;
                    found += 1;
                } else {
                    bulk_out_ep = ep->bEndpointAddress;
                    bulk_out_mps = ep->wMaxPacketSize;
                    found += 1;
                }
            }
        }
        offset += bLength;
    }

    return found;
}

void __debug_usb_print_dev_config_desc(usb_device_config_t *device_config) {
    printf("Device Configuration Descriptior: \n");
    printf(
        "\tlength = 0x%x\n\tdescriptor_type = 0x%x\n\ttotal_length = 0x%x\n\t"
        "num_interfaces = 0x%x\n\tconfiguration_value = 0x%x\n\tconfiguration_index = "
        "0x%x\n\tbm_attributes = 0x%x\n\tmax_power = 0x%x\n",
        device_config->length, device_config->descriptor_type, device_config->total_length,
        device_config->num_interfaces, device_config->configuration_value,
        device_config->configuration_index, device_config->bm_attributes, device_config->max_power);
}

uint8_t *get_mac_address() {
    return mac_address;
}

uint8_t __attribute__((aligned(8))) ethernet_buffer[1514];

int cdc_ecm_get_mac(usb_session *session) {
    uint8_t buffer[32];
    usb_setup_packet_t setup;
    setup.bmRequestType = 0x80;  // Standard, Device-to-Host
    setup.bRequest = 0x06;
    setup.wValue = (0x03 << 8) | 3;  // hardcoded for QEMU emulate device
    setup.wIndex = 0x0409;           // Language ID (English - US)
    setup.wLength = 64;

    if (usb_handle_transfer(session, &setup, buffer, 32)) {
        printf("Error getting MAC address\n");
        return -1;
    }

    uint8_t bLength = buffer[0];
    uint8_t bDescriptorType = buffer[1];

    uint8_t mac_idx = 0;
    for (int i = 2; i < bLength; i += 4) {
        mac_address[mac_idx++] = ((buffer[i] - '0') << 4) | (buffer[i + 2] - '0');
    }

    return 0;
}

int cdc_ecm_set_filter(usb_session *session) {
    uint32_t filter = 0x05;  // Enable broadcast + unicast reception

    usb_setup_packet_t setup;
    setup.bmRequestType = 0x21;  // Class, Host-to-Device, Interface
    setup.bRequest = 0x43;
    setup.wValue = 0;
    setup.wIndex = 0;
    setup.wLength = sizeof(filter);

    session->out_data = true;
    int result = usb_handle_transfer(session, &setup, (uint8_t *)&filter, sizeof(filter));
    session->out_data = false;
    return result;
}

int cdc_ecm_init(usb_session *session) {
    return usb_set_configuration(session, 1) | cdc_ecm_get_mac(session) |
           cdc_ecm_set_filter(session);
}

usb_session *network_usb_send_session(usb_session *update) {
    static usb_session send_session;
    if (update != nullptr) send_session = *update;
    return &send_session;
}

usb_session *network_usb_recv_session(usb_session *update) {
    static usb_session recv_session;
    if (update != nullptr) recv_session = *update;
    return &recv_session;
}

void network_loop() {
    usb_session *send_session = network_usb_send_session(nullptr);
    int size;

    dhcp_discover(send_session);

    while (1) {
        size = receive_packet(ethernet_buffer, sizeof(ethernet_buffer));
        if (size) {
            process_packet(send_session, ethernet_buffer, size);
        }
    }
}

void initialize_network_card(usb_session *session, usb_device_descriptor_t *device,
                             usb_device_config_t *config) {
    usb_session network_session;
    memcpy(&network_session, session, sizeof(usb_session));
    if (cdc_ecm_init(&network_session)) {
        printf("[ethernet-driver] failed to initialize cdc ecm\n");
        return;
    }

    usb_device_config_t cdc_config;

    usb_get_device_config_descriptor(&network_session, (uint8_t *)&cdc_config, sizeof(cdc_config));

    uint8_t __attribute__((aligned(4))) full_config[cdc_config.total_length];

    usb_setup_packet_t packet;
    packet.bmRequestType = 0x80;
    packet.bRequest = 0x06;
    packet.wValue = (2 << 8);
    packet.wIndex = 0;
    packet.wLength = cdc_config.total_length;

    usb_handle_transfer(&network_session, &packet, full_config, cdc_config.total_length);

    if (parse_config(full_config, cdc_config.total_length) != TOTAL_CONFIG_ELEMENTS) {
        printf("[ethernet-driver] faulty device configuration.\n");
        return;
    }

    network_session.in_ep_num = bulk_in_ep;
    network_session.out_ep_num = bulk_out_ep;
    network_session.in_toggle = 0x00;
    network_session.out_toggle = 0x00;

    printf("\n[ethernet-driver] network card initialized, attempting dhcp discovery...\n");
    network_session.channel = USB_CHANNEL(6);

    network_session.channel->characteristics.st.enable = false;
    while (network_session.channel->characteristics.st.enable);
    network_session.channel->interrupt.raw = 0x3FF;

    network_session.mps = bulk_in_mps;
    network_usb_recv_session(&network_session);

    network_session.channel = USB_CHANNEL(5);

    network_session.channel->characteristics.st.enable = false;
    while (network_session.channel->characteristics.st.enable);
    network_session.channel->interrupt.raw = 0x3FF;

    network_session.mps = bulk_out_mps;
    network_usb_send_session(&network_session);
}