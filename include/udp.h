#ifndef _UDP_H
#define _UDP_H

#include "dwc.h"
#include "net_stack.h"

void handle_udp(usb_session *session, PacketBufferParser *buffer_parser);

#endif