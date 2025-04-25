#ifndef _DNS_H
#define _DNS_H

#include "dhcp.h"
#include "dwc.h"
#include "hash.h"
#include "net_stack.h"

#define DNS_IDENTIFICATION 0x0510

typedef struct {
    net_uint16_t id;
    net_uint16_t flags;
    net_uint16_t qdCount;
    net_uint16_t anCount;
    net_uint16_t nsCount;
    net_uint16_t arCount;
} __attribute__((packed)) dns_query_header;

HashMap<const char*, server_group*>& get_dns_cache();

void handle_dns_response(usb_session* session,
                         PacketParser<EthernetFrame, IPv4Packet, UDPDatagram,
                                      Packet<dns_query_header>, Payload>* parser);

void dns_query(usb_session* session, const char* domain, Function<void(server_group*)>&& consumer);

#endif