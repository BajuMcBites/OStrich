#ifndef _ARP_H
#define _ARP_H
#include <stdint.h>

#include "function.h"
#include "dwc.h"
#include "hash.h"
#include "net_stack.h"

#define ARP_ETHERNET_TYPE 0x01
#define ARP_REQUEST_OPCODE 0x01
#define ARP_REPLY_OPCODE 0x02

HashMap<uint64_t, uint8_t*> &get_arp_cache();

bool arp_has_resolved(uint32_t dst_ip);

void arp_resolve_mac(usb_session *session, uint32_t dst_ip, Function<void(uint8_t *)>&& consumer);
int handle_arp_packet(usb_session *session, PacketBufferParser *parser);

#endif