#include "mouse.h"

#include "dwc.h"
#include "printf.h"

mouse usb_mouse;

int get_mouse_input() {
    if (!usb_mouse.device_state.discovered || !usb_mouse.device_state.connected) return 1;

    usb_session session;
    init_usb_session(&session, &usb_mouse.device_state, usb_mouse.device_state.mps);

    if (usb_interrupt_in_transfer(&session, (uint8_t *)&usb_mouse.mouse_state, 4)) return 1;
    return 0;
}