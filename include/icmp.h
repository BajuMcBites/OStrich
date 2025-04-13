#ifndef _ICMP_H
#define _ICMP_H

#include "dwc.h"
#include "net_stack.h"

#define ICMP_ECHO_REPLY_TYPE 0
#define ICMP_ECHO_REQUEST_TYPE 8

void handle_icmp(usb_session *session, PacketBufferParser *buffer_parser);
void icmp_ping(usb_session *session, uint32_t ip,
               Function<void(PacketParser<ICMPPacket, Payload>*)> &&callback);
#endif