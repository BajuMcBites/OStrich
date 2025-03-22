#include "peripherals/dwc.h"

#include "hid.h"
#include "libk.h"
#include "peripherals/timer.h"
#include "printf.h"

void usb_reset_controller() {
    *USB_GRSTCTL = (1 << 0);

    while (*USB_GRSTCTL & (1 << 0)) {
    }

    *USB_GUSBCFG = USB_FORCE_HOST_MODE;
    *USB_HCFG |= (1 << 23);
}

void usleep(unsigned int microseconds) {
    unsigned int cycles = (SYSTEM_CLOCK_FREQ / 1000000) * microseconds;
    volatile unsigned int i;
    for (i = 0; i < cycles; i++) {
    }
}

void usb_reset_port(volatile uint32_t *port) {
    *port |= USB_HPRT_PRT_RST;
    usleep(50);
    *port &= ~USB_HPRT_PRT_RST;
    usleep(50);
}

int handle_transaction(usb_session *session, uint8_t stage) {
    struct host_channel::interrupts_u::interrupts *interrupts = &session->channel->interrupt.st;

    session->channel->interrupt_mask |= 0x3ff;
    uint32_t timeout = 100000;

    while (!(interrupts->transfer_complete) && timeout-- > 0 && stage != USB_STATUS_STAGE) {
    }

    int errors = timeout == 0 ? USB_ERR_NONE : USB_ERR_TIMEOUT;
    if (timeout == 0) {
        printf("USB Error: Timeout\n");
    }

    int int_mask = 0x200;

    if (session->channel->interrupt.raw == 0x3) {
        *((uint32_t *)&session->channel->interrupt) |= 0x1;
        return 0;
    }
    if (session->channel->interrupt.raw == 0) {
        printf("USB Error: Unknown error (interrupt.raw = 0)\n");
        return USB_ERR_UNKNOWN;
    }

    if (interrupts->stall) {
        printf("USB Error: STALL received\n");
        errors |= USB_ERR_STALL;
        int_mask |= 0x8;
    }
    if (interrupts->babble_err) {
        printf("USB Error: Babble error\n");
        errors |= USB_ERR_BUB;
        int_mask |= 0x100;
    }
    if (interrupts->negative_ack) {
        printf("USB Error: NAK received\n");
        errors |= USB_ERR_NAK;
        int_mask |= 0x10;
    }
    if (interrupts->transaction_err) {
        printf("USB Error: Transaction error\n");
        errors |= USB_ERR_TRANS;
        int_mask |= 0x80;
    }
    if (interrupts->abh_error) {
        printf("USB Error: AHB error\n");
        errors |= USB_ERR_ABH;
        int_mask |= 0x10;
    }
    if (interrupts->data_tgl_err) {
        printf("USB Error: Data toggle error\n");
        errors |= USB_ERR_DATA_TOGGLE;
        int_mask |= 0x400;
    }

    // Optional: print these non-error conditions too
    if (interrupts->not_yet) {
        printf("USB Status: Not yet\n");
        int_mask |= 0x40;
    }
    if (interrupts->ack) {
        printf("USB Status: ACK received\n");
        int_mask |= 0x20;
    }

    if (errors == 0) {
        printf("USB Transaction completed successfully\n");
    }

    session->channel->interrupt.raw |= int_mask;
    return errors;
}

// if you are curious, read about the control register bits here:
// channel characteristics:
// https://www.intel.com/content/www/us/en/programmable/hps/stratix-10/hps.html#agi1505406227399.html
// channel transfer_size:
// https://www.intel.com/content/www/us/en/programmable/hps/stratix-10/hps.html#qcc1505406231527.html
// etc
int usb_send_setup_packet(usb_session *session, usb_setup_packet_t *setup, int len) {
    struct host_channel::characteristics_u::characteristics *chars =
        &session->channel->characteristics.st;
    struct host_channel::transfer_size_u::transfer_size *transfer =
        &session->channel->transfer_size.st;

    chars->device_address = session->device_address;
    chars->ep_num = session->ep_num;
    chars->ep_dir = (setup->bmRequestType & USB_DEVICE_TO_HOST ? DeviceToHost : HostToDevice);
    chars->ep_type = EndpointType::Control;
    chars->mps = 8;
    chars->low_speed = session->low_speed;

    transfer->xfer_size = len;
    transfer->pid = USB_MDATA_CONTROL;
    transfer->pck_cnt = 1;
    transfer->do_ping = true;

    session->channel->dma_address = (uint64_t)setup;

    chars->enable = true;

    return handle_transaction(session, USB_SETUP_STAGE);
}

int usb_receive_data(usb_session *session, uint8_t *buffer, uint16_t length,
                     uint16_t max_packet_size) {
    struct host_channel::characteristics_u::characteristics *chars =
        &session->channel->characteristics.st;
    struct host_channel::transfer_size_u::transfer_size *transfer =
        &session->channel->transfer_size.st;

    chars->device_address = session->device_address;
    chars->ep_num = session->ep_num;
    chars->ep_dir = DeviceToHost;
    chars->ep_type = EndpointType::Control;
    chars->mps = max_packet_size;
    chars->low_speed = session->low_speed;

    uint8_t pckt = ((length + 7) / 8);

    transfer->xfer_size = length;
    transfer->pid = USB_DATA1;
    transfer->pck_cnt = pckt;
    transfer->do_ping = true;  // for OUT transfers.

    session->channel->dma_address = (uint64_t)buffer;

    chars->enable = true;

    return handle_transaction(session, USB_DATA_STAGE);
}

int usb_status_stage(usb_session *session, uint8_t direction) {
    struct host_channel::characteristics_u::characteristics *chars =
        &session->channel->characteristics.st;
    struct host_channel::transfer_size_u::transfer_size *transfer =
        &session->channel->transfer_size.st;
    chars->device_address = session->device_address;
    chars->ep_num = session->ep_num;
    chars->ep_dir = (direction ? HostToDevice : DeviceToHost);
    chars->ep_type = EndpointType::Control;
    chars->mps = 8;
    chars->low_speed = session->low_speed;

    transfer->xfer_size = 0;
    transfer->pid = USB_DATA1;
    transfer->pck_cnt = 1;
    transfer->do_ping = true;

    chars->enable = true;

    return handle_transaction(session, USB_STATUS_STAGE);
}

int usb_handle_transfer(usb_session *session, usb_setup_packet_t *setup_packet, uint8_t *buffer,
                        int buf_length, uint16_t max_packet_size) {
    // Setup packet is 8 bytes.
    int setup_status = usb_send_setup_packet(session, setup_packet, 8);

    // Print the setup packet.
    printf("Setup Packet Request Type: %x\n", setup_packet->bmRequestType);
    printf("Setup Packet Request: %x\n", setup_packet->bRequest);
    printf("Setup Packet Value: %x\n", setup_packet->wValue);
    printf("Setup Packet Index: %x\n", setup_packet->wIndex);
    printf("Setup Packet Length: %x\n", setup_packet->wLength);

    if (setup_status) return setup_status;

    // Data packet is buf_length bytes.
    int data_status = buffer != nullptr && buf_length > 0
                          ? usb_receive_data(session, buffer, buf_length, max_packet_size)
                          : 0;
    printf("data_status: %d\n", data_status);
    if (data_status) return data_status;
    usb_status_stage(session,
                     (setup_packet->bmRequestType & USB_DEVICE_TO_HOST) == USB_DEVICE_TO_HOST);
    printf("usb_handle_transfer ret: %d\n", setup_status & data_status);

    return setup_status & data_status;
}

int usb_send_token_packet(usb_session *session, uint8_t pid) {
    // Send token packet.
    struct host_channel::characteristics_u::characteristics *chars =
        &session->channel->characteristics.st;
    struct host_channel::transfer_size_u::transfer_size *transfer =
        &session->channel->transfer_size.st;

    chars->device_address = session->device_address;
    chars->ep_num = session->ep_num;
    chars->ep_dir = DeviceToHost;
    chars->ep_type = EndpointType::Control;
    chars->mps = 8;
    chars->low_speed = session->low_speed;

    transfer->xfer_size = 0;
    transfer->pid = pid;
    transfer->pck_cnt = 1;
    transfer->do_ping = true;

    session->channel->dma_address = (uint64_t)buffer;

    chars->enable = true;

    return handle_transaction(session, USB_DATA_STAGE);
}

int usb_get_device_descriptor(usb_session *session, uint8_t *buffer) {
    usb_setup_packet_t setup_packet;
    setup_packet.bmRequestType = USB_DEVICE_TO_HOST;
    setup_packet.bRequest = USB_GET_DESCRIPTOR;
    setup_packet.wValue = 0x0100;
    setup_packet.wIndex = 0x0000;
    setup_packet.wLength = 18;

    return usb_handle_transfer(session, &setup_packet, buffer, 18, 8);
}
int usb_get_device_config_descriptor(usb_session *session, uint8_t *buffer, uint16_t length) {
    usb_setup_packet_t setup_packet;
    setup_packet.bmRequestType = USB_DEVICE_TO_HOST;
    setup_packet.bRequest = 0x06;
    setup_packet.wValue = 0x0200;
    setup_packet.wIndex = 0x00;
    setup_packet.wLength = length;

    return usb_handle_transfer(session, &setup_packet, buffer, length, 8);
}

int usb_assign_address(usb_session *session) {
    static int address = 1;

    // Tell device what address we are assigning to it.
    usb_setup_packet_t setup_packet;
    setup_packet.bmRequestType = 0x00;
    setup_packet.bRequest = 0x05;  // 0x05 is Set Address Request.
    setup_packet.wValue = address;
    setup_packet.wIndex = 0;
    setup_packet.wLength = 0x0000;

    int error_status = usb_handle_transfer(session, &setup_packet, nullptr, 0, 8);
    if (error_status == 0) {
        session->channel->characteristics.st.device_address = address;
        session->device_address = address;
        session->port = 0;
        address += 1;
    } else {
        printf("USB Error: Failed to assign address %d\n", address);
    }
    return error_status;
}

int usb_set_configuration(usb_session *session, uint8_t config_value) {
    usb_setup_packet_t setup_packet;
    setup_packet.bmRequestType = 0x00;
    setup_packet.bRequest = 0x09;
    setup_packet.wValue = config_value;
    setup_packet.wIndex = 0x00;
    setup_packet.wLength = 0x00;

    return usb_handle_transfer(session, &setup_packet, nullptr, 0, 8);
}

int usb_set_interface(usb_session *session, uint8_t interface_num, uint8_t alternate_setting) {
    usb_setup_packet_t setup_packet;
    setup_packet.bmRequestType = 0x00;
    setup_packet.bRequest = 0x0B;
    setup_packet.wValue = alternate_setting;
    setup_packet.wIndex = interface_num;
    setup_packet.wLength = 0x00;

    return usb_handle_transfer(session, &setup_packet, nullptr, 0, 8);
}

int usb_get_hub_descriptor(usb_session *session, uint8_t *buffer) {
    usb_setup_packet_t setup_packet;
    setup_packet.bmRequestType = 0xa0;
    setup_packet.bRequest = 0x06;
    setup_packet.wValue = 0x0100;
    setup_packet.wIndex = 0x00;
    setup_packet.wLength = 0x10;

    return usb_handle_transfer(session, &setup_packet, buffer, 0x10, 8);
}

int usb_hub_get_port_status(usb_session *session, uint8_t *buffer) {
    usb_setup_packet_t setup_packet;
    setup_packet.bmRequestType = 0xa3;
    setup_packet.bRequest = 0x00;
    setup_packet.wValue = 0x00;
    setup_packet.wIndex = 1 + session->port;
    setup_packet.wLength = 0x4;

    return usb_handle_transfer(session, &setup_packet, buffer, 0x4, 8);
}

int usb_hub_clear_feature(usb_session *session, uint8_t feature) {
    usb_setup_packet_t setup_packet;
    setup_packet.bmRequestType = 0x23;
    setup_packet.bRequest = 0x01;
    setup_packet.wValue = feature;
    setup_packet.wIndex = 1 + session->port;
    setup_packet.wLength = 0;

    return usb_handle_transfer(session, &setup_packet, nullptr, 0x10, 8);
}

int usb_get_interfaces(usb_session *session) {
    usb_setup_packet_t setup_packet;
    setup_packet.bmRequestType = USB_DEVICE_TO_HOST;
    setup_packet.bRequest = 0x06;
    setup_packet.wValue = 0x0400;
    setup_packet.wIndex = 0;
    setup_packet.wLength = 0x09;

    return usb_handle_transfer(session, &setup_packet, nullptr, 0x0, 8);
}

int usb_hub_reset_port(usb_session *session) {
    usb_setup_packet_t setup_packet;
    setup_packet.bmRequestType = 0x23;
    setup_packet.bRequest = 0x03;
    setup_packet.wValue = 0x04;
    setup_packet.wIndex = 1 + session->port;
    setup_packet.wLength = 0;

    return usb_handle_transfer(session, &setup_packet, nullptr, 0x0, 8);
}

int usb_get_hub_status(usb_session *session, uint8_t *buffer) {
    usb_setup_packet_t setup_packet;
    setup_packet.bmRequestType = 0xa0;
    setup_packet.bRequest = 0x00;
    setup_packet.wValue = 0x00;
    setup_packet.wIndex = 0x00;
    setup_packet.wLength = 0x4;

    return usb_handle_transfer(session, &setup_packet, buffer, 0x10, 8);
}

int usb_hub_enable_port(usb_session *session) {
    usb_setup_packet_t setup_packet;
    setup_packet.bmRequestType = 0x23;
    setup_packet.bRequest = 0x03;
    setup_packet.wValue = 0x08;
    setup_packet.wIndex = 1 + session->port;
    setup_packet.wLength = 0;

    return usb_handle_transfer(session, &setup_packet, nullptr, 0x0, 8);
}

void _debug_usb_print_dev_desc(usb_device_descriptor_t *device_descriptor) {
    printf("Device Descriptior: \n");
    printf(
        "\tlength = 0x%x\n\tdescriptor_type = 0x%x\n\tusb_version = 0x%x\n\tdevice_class = "
        "0x%x\n\tdevice_subclass = 0x%x\n\tdevice_protocol = 0x%x\n\tmax_packet_size = "
        "0x%x\n\t"
        "vendor_id = 0x%x\n\tproduct_id = 0x%x\n\tdevice_version = 0x%x\n\t"
        "manufacturer_index "
        "= 0x%x\n\tproduct_index = 0x%x\n\tserial_number_index = 0x%x\n\tnum_configurations "
        "= "
        "0x%x\n",
        device_descriptor->length, device_descriptor->descriptor_type,
        device_descriptor->usb_version, device_descriptor->device_class,
        device_descriptor->device_subclass, device_descriptor->device_protocol,
        device_descriptor->max_packet_size, device_descriptor->vendor_id,
        device_descriptor->product_id, device_descriptor->device_version,
        device_descriptor->manufacturer_index, device_descriptor->product_index,
        device_descriptor->serial_number_index, device_descriptor->num_configurations);
}

void _debug_usb_print_dev_config_desc(usb_device_config_t *device_descriptor) {
    printf("Device Configuration Descriptior: \n");
    printf(
        "\tlength = 0x%x\n\tdescriptor_type = 0x%x\n\ttotal_length = 0x%x\n\t"
        "num_configurations = 0x%x\n\tconfiguration_value = 0x%x\n\tconfiguration_index = "
        "0x%x\n\tbm_attributes = 0x%x\n\tmax_power = 0x%x\n",
        device_descriptor->length, device_descriptor->descriptor_type,
        device_descriptor->total_length, device_descriptor->num_configurations,
        device_descriptor->configuration_value, device_descriptor->configuration_index,
        device_descriptor->bm_attributes, device_descriptor->max_power);
}

void _debug_usb_print_hub_desc(usb_hub_descriptor_t *hub_descriptor) {
    printf("Hub Descriptior: \n");
    printf(
        "\tbLength = 0x%x\n\tbDescriptorType = 0x%x\n\tbNbrPorts = 0x%x\n\twHubCharacteristics = "
        "0x%x\n\tbPwrOn2pwrGood = 0x%x\n\tbHubContrCurrent = 0x%x\n",
        hub_descriptor->bLength, hub_descriptor->bDescriptorType, hub_descriptor->bNbrPorts,
        hub_descriptor->wHubCharacteristics, hub_descriptor->bPwrOn2pwrGood,
        hub_descriptor->bHubContrCurrent);
}

void _debug_usb_print_port_status(usb_port_status_t *port_status) {
    printf(
        "\tconn = %d, enb = %d, sus = %d, over_cur = %d, reset = %d, power = %d, low = %d, "
        "high = %d\n",
        (port_status->status & (1 << 0)), (port_status->status & (1 << 1)),
        (port_status->status & (1 << 2)), (port_status->status & (1 << 3)),
        (port_status->status & (1 << 4)), (port_status->status & (1 << 8)),
        (port_status->status & (1 << 9)), (port_status->status & (1 << 10)));
    printf("\t: c(lear)_conn = %d, c_enable = %d, c_sus = %d, c_overcur = %d, c_reset = %d\n",
           port_status->change & PORT_STAT_C_CONNECTION, port_status->change & PORT_STAT_C_ENABLE,
           port_status->change & PORT_STAT_C_SUSPEND, port_status->change & PORT_STAT_C_OVERCURRENT,
           port_status->change & PORT_STAT_C_RESET);
}

void usb_device_enumeration(usb_session *);

void usb_handle_hub_enumeration(usb_session *session) {
    K::assert(session->device_address != 0, "usb hub cannot be assigned have address=0");
    if (session->device_address > 1) return;
    usb_hub_descriptor_t usb_hub_desc;

    if (usb_get_hub_descriptor(session, reinterpret_cast<uint8_t *>(&usb_hub_desc))) {
        return;
    }

    _debug_usb_print_hub_desc(&usb_hub_desc);

    for (int i = 0; i < usb_hub_desc.bNbrPorts; i++) {
        session->port = i;
        if (usb_hub_enable_port(session)) {
            return;
        }
        usb_port_status_t p;
        usb_hub_get_port_status(session, reinterpret_cast<uint8_t *>(&p));

        if (p.status & (1 << 0) && p.change & (1 << 0)) {
            usb_hub_clear_feature(session, USB_HUB_PORT_CONN_CLEAR_FEATURE);

            // reset the device to set it's address to 0,
            // which we can then enumerate and change it's address
            usb_hub_reset_port(session);

            int saved_addr = session->device_address;
            session->device_address = 0;
            session->port = 0;
            usb_device_enumeration(session);
            session->device_address = saved_addr;
        }
    }

    session->port = 0;
    hubs.connected[hubs.hub_count].num_ports = usb_hub_desc.bNbrPorts;
    hubs.connected[hubs.hub_count].device_address = session->device_address;
    hubs.connected[hubs.hub_count].low_speed = session->low_speed;
    hubs.hub_count++;
}

void usb_finished_enumeration(usb_session *session, usb_device_descriptor_t *d,
                              usb_device_config_t *config) {
    // TODO: create some structure to store devices
    printf("Detected & assigned new device to address = %d.\n", session->device_address);
    if (d->device_class == 0x2) {
        // CDC-based Network card initialized here.
    } else if (d->device_class == 0x0 || d->device_class == USB_HID_INTERFACE_CLASS) {
        // HID devices often have device_class 0 (with interfaces specifying the class)
        // or directly specify HID class at device level
        printf("\n\n ––––––––––– Detected HID device –––––––––––\n");
        mouse_detect(session, d, config);
    }
}

void usb_device_enumeration(usb_session *session) {
    K::assert(session->device_address == 0, "device must be at address = 0 to enumerate");
    usb_device_descriptor_t device_descriptor;
    usb_hub_descriptor_t usb_hub_desc;

    if (usb_assign_address(session)) {
        return;
    }

    if (usb_get_device_descriptor(session, reinterpret_cast<uint8_t *>(&device_descriptor))) {
        return;
    }
    printf("\n");
    _debug_usb_print_dev_desc(&device_descriptor);

    usb_device_config_t device_config;
    if (usb_get_device_config_descriptor(session, reinterpret_cast<uint8_t *>(&device_config))) {
        return;
    }
    _debug_usb_print_dev_config_desc(&device_config);

    if (device_descriptor.usb_version < 0x200) {
        session->low_speed = 1;
    }

    if (device_descriptor.device_class == 0x9) {
        usb_handle_hub_enumeration(session);
    }

    usb_finished_enumeration(session, &device_descriptor, &device_config);
}

void usb_disable_device_port(host_channel *channel, volatile uint32_t *port) {
    *port &= ~(1 << 2);
    usb_reset_controller();

    channel->characteristics.st.device_address = 0x0;
}

void usb_handle_hub_scan_port(usb_session *session, usb_hub *hub) {
    session->device_address = hub->device_address;

    usb_port_status_t port_status;

    if (usb_hub_get_port_status(session, reinterpret_cast<uint8_t *>(&port_status))) {
        return;
    }

    if (port_status.status & 0x1 && port_status.change & 0x1) {
        usb_hub_reset_port(session);

        session->device_address = 0;
        session->port = 0;
        usb_device_enumeration(session);
    }
}
void usb_handle_host_scan_port(usb_session *session, usb_hub *hub) {
    if (*USB_HPRT & USB_HPRT_PRT_CONNDET && *USB_HPRT & USB_HPRT_PRT_CONN) {
        usb_reset_port(USB_HPRT);

        usb_session enum_session;
        enum_session.channel = session->channel;
        enum_session.ep_num = session->ep_num;
        enum_session.device_address = 0;
        enum_session.port = 0;
        usb_device_enumeration(&enum_session);

        usb_disable_device_port(enum_session.channel, USB_HPRT);
    }
}

usb_connected_hubs hubs = {.hub_count = 0};

void usb_scan_ports() {
    int hubs_enumerated = 0;
    while (hubs_enumerated < hubs.hub_count) {
        for (int i = 0; i < hubs.hub_count; i++) {
            host_channel *channel = USB_CHANNEL(0);
            usb_hub hub = hubs.connected[i];

            usb_session session;
            session.channel = channel;
            session.ep_num = 0;
            session.low_speed = hub.low_speed;

            for (int port = 0; port < hub.num_ports; port++) {
                session.port = port;
                if (i != 0) {
                    printf("Scanning hub port %d\n", port);
                    usb_handle_hub_scan_port(&session, &hub);
                } else {
                    printf("IDX 0: Scanning host port %d\n", port);
                    usb_handle_host_scan_port(&session, &hub);
                }
            }

            hubs_enumerated++;
        }
    }
}

void usb_initialize() {
    usb_reset_controller();

    hubs.connected[0].num_ports = 1;
    hubs.connected[0].device_address = 0;
    hubs.hub_count = 1;

    usb_scan_ports();
    return;
}

int get_interval_ms_for_interface(usb_session *session, uint8_t interval) {
    bool is_low_speed = session->low_speed;
    int period_microseconds = is_low_speed ? 125 : 1000;
    return period_microseconds * (1 << interval);
}