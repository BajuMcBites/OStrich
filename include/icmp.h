#ifndef _ICMP_H
#define _ICMP_H

#include "dwc.h"
#include "net_stack.h"

#define ICMP_ECHO_REQUEST_TYPE 8

void handle_icmp(usb_session *session, PacketBufferParser *buffer_parser);

#endif