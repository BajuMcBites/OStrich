#ifndef _TCP_H
#define _TCP_H

#include "dwc.h"
#include "net_stack.h"

#define TCP_FLAG_FIN 0x001
#define TCP_FLAG_SYN 0x002
#define TCP_FLAG_RST 0x004
#define TCP_FLAG_PUSH 0x008
#define TCP_FLAG_ACK 0x010
#define TCP_FLAG_URG 0x020
#define TCP_FLAG_ECE 0x040
#define TCP_FLAG_CWR 0x080
#define TCP_FLAG_NS 0x100

void handle_tcp(usb_session *session, PacketBufferParser *buffer_parser);

#endif