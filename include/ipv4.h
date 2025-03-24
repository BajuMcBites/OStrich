#ifndef _IPV4_H
#define _IPV4_H

#include "dwc.h"
#include "net_stack.h"

void handle_ipv4_packet(usb_session *session, PacketBufferParser* parser);

#endif