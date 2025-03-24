#ifndef _ARP_H
#define _ARP_H
#include <stdint.h>

#include "dwc.h"
#include "hash.h"
#include "net_stack.h"

extern HashMap<uint8_t[6]> arp_cache;

int handle_arp_packet(usb_session *session, PacketBufferParser *parser);

#endif