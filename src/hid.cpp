#include "hid.h"

#include "dwc.h"
#include "libk.h"
#include "peripherals/timer.h"
#include "printf.h"

// Global mouse device instance
mouse_device_t USB_MOUSE = {0};

// USB HID Report Descriptor for a typical mouse (optional for boot protocol)
const uint8_t mouse_report_descriptor[] = {
    0x05, 0x01,  // Usage Page (Generic Desktop)
    0x09, 0x02,  // Usage (Mouse)
    0xA1, 0x01,  // Collection (Application)
    0x09, 0x01,  //   Usage (Pointer)
    0xA1, 0x00,  //   Collection (Physical)
    0x05, 0x09,  //     Usage Page (Button)
    0x19, 0x01,  //     Usage Minimum (Button 1)
    0x29, 0x03,  //     Usage Maximum (Button 3)
    0x15, 0x00,  //     Logical Minimum (0)
    0x25, 0x01,  //     Logical Maximum (1)
    0x95, 0x03,  //     Report Count (3)
    0x75, 0x01,  //     Report Size (1)
    0x81, 0x02,  //     Input (Data, Variable, Absolute)
    0x95, 0x01,  //     Report Count (1)
    0x75, 0x05,  //     Report Size (5)
    0x81, 0x03,  //     Input (Constant)
    0x05, 0x01,  //     Usage Page (Generic Desktop)
    0x09, 0x30,  //     Usage (X)
    0x09, 0x31,  //     Usage (Y)
    0x09, 0x38,  //     Usage (Wheel)
    0x15, 0x81,  //     Logical Minimum (-127)
    0x25, 0x7F,  //     Logical Maximum (127)
    0x75, 0x08,  //     Report Size (8)
    0x95, 0x03,  //     Report Count (3)
    0x81, 0x06,  //     Input (Data, Variable, Relative)
    0xC0,        //   End Collection
    0xC0         // End Collection
};

/**
 * Detect USB HID mouse devices during enumeration
 *
 * @param session Pointer to the USB session
 * @param dev_desc Pointer to the device descriptor
 * @param config Pointer to the configuration descriptor
 * @return 0 if a mouse is found and initialized, error code otherwise
 */
int mouse_detect(usb_session *session, usb_device_descriptor_t *dev_desc,
                 usb_device_config_t *config) {
    // Check if this is a HID device
    if (dev_desc->device_class != 0 && dev_desc->device_class != USB_HID_INTERFACE_CLASS) {
        return -1;
    }

    // Check if we already have a mouse connected
    if (USB_MOUSE.connected) {
        printf("Mouse already connected, ignoring additional mouse\n");
        return -1;
    }

    // We need to get the full configuration descriptor to find interfaces
    uint8_t config_buffer[config->total_length];

    // Get configuration, interface, endpoint, and HID descriptors.
    usb_get_device_config_descriptor(session, config_buffer, config->total_length);

    // Parse the configuration descriptor to find the HID interface
    printf("Config descriptor: total_length=%d, num_interfaces=%d\n", config->total_length,
           config_buffer[4]);

    // Start parsing from the end of the configuration descriptor header
    uint16_t offset = 0;

    // Loop through all interfaces
    while (offset < config->total_length) {
        uint8_t descriptor_length = config_buffer[offset];
        uint8_t descriptor_type = config_buffer[offset + 1];

        // Check if this is an interface descriptor
        if (descriptor_type == USB_INTERFACE_DESCRIPTOR_TYPE) {
            usb_device_interface_config_t *interface =
                (usb_device_interface_config_t *)&config_buffer[offset];

            uint8_t interface_class = config_buffer[offset + 5];
            uint8_t interface_subclass = config_buffer[offset + 6];
            uint8_t interface_protocol = config_buffer[offset + 7];

            printf("Interface found: class=%02x, subclass=%02x, protocol=%02x\n",
                   interface->bInterfaceClass, interface->bInterfaceSubClass,
                   interface->bInterfaceProtocol);

            // Check if this is a HID mouse interface (Boot Interface Subclass with Mouse Protocol)
            if (interface->bInterfaceClass == USB_HID_INTERFACE_CLASS &&
                interface->bInterfaceSubClass == USB_HID_INTERFACE_SUBCLASS_BOOT &&
                interface->bInterfaceProtocol == USB_HID_INTERFACE_PROTOCOL_MOUSE) {
                printf("Found HID mouse interface: %d\n", interface->bInterfaceNumber);
                printf("Num endpoints: %d\n", interface->bNumEndpoints);
                printf("descriptor_length: %d\n", interface->bLength);

                // Save the interface number
                USB_MOUSE.interface_num = interface->bInterfaceNumber;

                // Now look for the HID descriptor and endpoint descriptor
                uint16_t next_offset = offset + interface->bLength;
                bool found_hid_descriptor = false;

                // Scan through descriptors following the interface descriptor
                while (next_offset < config->total_length) {
                    uint8_t next_length = config_buffer[next_offset];
                    uint8_t next_type = config_buffer[next_offset + 1];

                    printf("next_offset = %d, next_length = %d, next_type = %02x\n", next_offset,
                           next_length, next_type);

                    // Make sure the descriptor length is valid
                    if (next_length == 0) break;

                    // Check if this is a HID descriptor
                    if (next_type == USB_HID_DESCRIPTOR_TYPE) {
                        // Found HID descriptor
                        printf("Found HID descriptor at offset %d, length %d\n", next_offset,
                               next_length);

                        // Extract HID descriptor information if needed
                        uint16_t bcdHID =
                            config_buffer[next_offset + 2] | (config_buffer[next_offset + 3] << 8);
                        uint8_t country_code = config_buffer[next_offset + 4];
                        uint8_t num_descriptors = config_buffer[next_offset + 5];

                        printf("  HID version: %d.%d, Country code: %d, Num descriptors: %d\n",
                               bcdHID >> 8, bcdHID & 0xFF, country_code, num_descriptors);

                        found_hid_descriptor = true;

                        // Read report descriptor
                        // if (config_buffer[next_offset + 6] == 0x22) {
                        //     printf("Report descriptor size: %d bytes\n",
                        //     config_buffer[next_offset + 7]);
                        // }
                    }

                    // Check if this is an endpoint descriptor
                    if (next_type == USB_ENDPOINT_DESCRIPTOR_TYPE) {
                        uint8_t endpoint_addr = config_buffer[next_offset + 2];
                        uint8_t attributes = config_buffer[next_offset + 3];
                        uint16_t max_packet_size =
                            config_buffer[next_offset + 4] | (config_buffer[next_offset + 5] << 8);
                        uint8_t interval = config_buffer[next_offset + 6];

                        // Check if this is an IN endpoint (bit 7 set) and interrupt type (bits 0-1
                        // = 11b)
                        if ((endpoint_addr & USB_HID_IN_ENDPOINT) &&
                            (attributes & USB_HID_ENDPOINT_TYPE_INTERRUPT)) {
                            printf(
                                "Found interrupt IN endpoint: addr=%02x, interval=%d, "
                                "max_packet_size=%d\n",
                                endpoint_addr, interval);

                            // Initialize the mouse device
                            USB_MOUSE.session = session;
                            USB_MOUSE.endpoint_address =
                                endpoint_addr &
                                0x0F;  // Remove direction bit, address is only 4 bits.
                            USB_MOUSE.interval_ms =
                                get_interval_ms_for_interface(session, interval);
                            USB_MOUSE.connected = true;

                            // Set the protocol to boot protocol (0)
                            int result = mouse_set_protocol(&USB_MOUSE, 0);
                            if (result != 0) {
                                printf("Failed to set boot protocol: %d\n", result);
                                return -1;
                            }

                            // Set the idle rate
                            result = mouse_set_idle(&USB_MOUSE, 0, 0);
                            if (result != 0) {
                                printf("Failed to set idle rate: %d\n", result);
                                return -1;
                            }

                            printf("Mouse initialized successfully\n");
                            return 0;
                        }
                    }

                    // Move to next descriptor
                    next_offset += next_length;
                    if (next_length == 0) {
                        break;
                    }
                }
                offset = next_offset;
            }
        } else {
            offset += descriptor_length;
        }

        if (descriptor_length == 0) {
            break;
        }
    }

    printf("No mouse interface found\n");
    return -1;  // No mouse interface found
}

/**
 * Set the HID protocol (boot or report)
 *
 * @param mouse Pointer to the mouse device
 * @param protocol 0 for boot protocol, 1 for report protocol
 * @return 0 if successful, error code otherwise
 */
int mouse_set_protocol(mouse_device_t *mouse, uint8_t protocol) {
    usb_setup_packet_t setup_packet;
    setup_packet.bmRequestType = HID_HOST_TO_DEVICE;
    setup_packet.bRequest = USB_HID_SET_PROTOCOL;
    setup_packet.wValue = protocol;
    setup_packet.wIndex = mouse->interface_num;
    setup_packet.wLength = 0;

    return usb_handle_transfer(mouse->session, &setup_packet, nullptr, 0, mouse->max_packet_size);
}

/**
 * Set the idle rate for the mouse
 *
 * @param mouse Pointer to the mouse device
 * @param duration Duration in 4ms increments (0 = indefinite)
 * @param report_id Report ID (0 = all reports)
 * @return 0 if successful, error code otherwise
 */
int mouse_set_idle(mouse_device_t *mouse, uint8_t duration, uint8_t report_id) {
    usb_setup_packet_t setup_packet;
    setup_packet.bmRequestType = HID_HOST_TO_DEVICE;
    setup_packet.bRequest = USB_HID_SET_IDLE;
    setup_packet.wValue = (duration << 8) | report_id;
    setup_packet.wIndex = mouse->interface_num;
    setup_packet.wLength = 0;

    return usb_handle_transfer(mouse->session, &setup_packet, nullptr, 0, mouse->max_packet_size);
}

/**
 * Get a report from the mouse
 *
 * @param mouse Pointer to the mouse device
 * @return 0 if successful, error code otherwise
 */
int mouse_get_report(mouse_device_t *mouse) {
    printf("\n\n\nmouse_get_report\n");
    if (!mouse->connected) {
        printf("Mouse not connected\n");
        return -1;
    }

    usb_setup_packet_t setup_packet;
    setup_packet.bmRequestType = HID_DEVICE_TO_HOST;
    setup_packet.bRequest = USB_HID_GET_REPORT;
    setup_packet.wValue = 0x0100;  // Report type = Input (1), Report ID = 0
    setup_packet.wIndex = mouse->interface_num;
    setup_packet.wLength = sizeof(mouse_report_t);

    int ret = usb_handle_transfer(mouse->session, &setup_packet, (uint8_t *)&mouse->report,
                                  sizeof(mouse_report_t), mouse->max_packet_size);
    printf("mouse_get_report ret: %d\n", ret);
    return ret;
}

/**
 * Process mouse input report
 *
 * @param mouse Pointer to the mouse device
 */
void mouse_process_input(mouse_device_t *mouse) {
    if (!mouse->connected) {
        return;
    }

    // Get a report from the mouse
    int result = mouse_get_report(mouse);
    if (result != 0) {
        printf("Failed to get mouse report: %d\n", result);
        return;
    }

    // Process the mouse movement
    mouse_handle_movement(mouse->report.x, mouse->report.y, mouse->report.wheel,
                          mouse->report.buttons);
}

/**
 * Handle mouse movement and button events
 *
 * @param x X movement
 * @param y Y movement
 * @param wheel Mouse wheel movement
 * @param buttons Button state
 */
void mouse_handle_movement(int8_t x, int8_t y, int8_t wheel, uint8_t buttons) {
    static int cursor_x = 0;
    static int cursor_y = 0;
    static uint8_t prev_buttons = 0;

    // Update cursor position
    cursor_x += x;
    cursor_y += y;

    // Constrain cursor to screen boundaries (example values)
    if (cursor_x < 0) cursor_x = 0;
    if (cursor_y < 0) cursor_y = 0;
    if (cursor_x > 800) cursor_x = 800;  // Example screen width
    if (cursor_y > 600) cursor_y = 600;  // Example screen height

    // Check for button changes
    uint8_t button_changes = buttons ^ prev_buttons;
    prev_buttons = buttons;

    // Print debug info
    printf("Mouse: X=%d, Y=%d, Wheel=%d, Buttons=%02x\n", cursor_x, cursor_y, wheel, buttons);

    if (button_changes & MOUSE_BUTTON_LEFT) {
        printf("Left button %s\n", (buttons & MOUSE_BUTTON_LEFT) ? "pressed" : "released");
    }

    if (button_changes & MOUSE_BUTTON_RIGHT) {
        printf("Right button %s\n", (buttons & MOUSE_BUTTON_RIGHT) ? "pressed" : "released");
    }

    if (button_changes & MOUSE_BUTTON_MIDDLE) {
        printf("Middle button %s\n", (buttons & MOUSE_BUTTON_MIDDLE) ? "pressed" : "released");
    }
}
