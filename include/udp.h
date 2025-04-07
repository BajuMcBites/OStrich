#ifndef _UDP_H
#define _UDP_H

#include "net_stack.h"
#include "dwc.h"

void handle_udp(usb_session *session, PacketBufferParser *buffer_parser);

#endif