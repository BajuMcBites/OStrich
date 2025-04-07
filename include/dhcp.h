#ifndef _DHCP_H
#define _DHCP_H

#include "dwc.h"
#include "net_stack.h"

/*
 * Important Resources:
 * - https://www.iana.org/assignments/bootp-dhcp-parameters/bootp-dhcp-parameters.xhtml
 * - https://www.rfc-editor.org/rfc/rfc2132.html
 */

#define DHCP_OPTION_SUBNET_MASK 1
#define DHCP_OPTION_TIME_OFFSET 2
#define DHCP_OPTION_ROUTE_SERVER 3
#define DHCP_OPTION_TIME_SERVER 4
#define DHCP_OPTION_DNS_SERVER 5
#define DHCP_OPTION_IMPRESS_SERVER 10
#define DHCP_OPTION_MESSAGE_TYPE 53
#define DHCP_OPTION_SERVER_ID 54 

#define DHCP_MAX_TIME_SERVERS 4
#define DHCP_MAX_DNS_SERVERS 10

template <size_t Max>
struct server_group {
    uint8_t count;
    uint32_t ips[Max];
} __attribute__((packed));

typedef struct {
    uint32_t subnet_mask;

    uint32_t my_ip;
    uint32_t dhcp_server_ip;

    server_group<DHCP_MAX_DNS_SERVERS> dns_servers;
    server_group<DHCP_MAX_TIME_SERVERS> time_servers;

} __attribute__((packed)) dhcp_state_t;

void dhcp_handle_offer(PacketParser<EthernetFrame, IPv4Packet, UDPDatagram, DHCPPacket>* parser);
void dhcp_discover(usb_session* session);

extern dhcp_state_t dhcp_state;

#endif