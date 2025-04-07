#ifndef _ARP_H
#define _ARP_H
#include <stdint.h>

#include "dwc.h"
#include "hash.h"
#include "net_stack.h"

typedef struct {
    net_uint16_t id;
    net_uint16_t flags;
    net_uint16_t qdCount;
    net_uint16_t anCount;
    net_uint16_t nsCount;
    net_uint16_t arCount;
} __attribute__((packed)) dns_query_header;

extern HashMap<uint8_t[6]> arp_cache;

int handle_arp_packet(usb_session *session, PacketBufferParser *parser);

#endif