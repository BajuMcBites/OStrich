#include "ipv4.h"

#include "network_card.h"
#include "tcp.h"
#include "udp.h"
#include "printf.h"

void handle_ipv4_packet(usb_session *session, PacketBufferParser *buffer_parser) {
    auto ipv4_packet = buffer_parser->pop<IPv4Packet>();

    PacketParser<EthernetFrame, IPv4Packet, TCPPacket> parser(buffer_parser->get_packet_base(),
                                                              buffer_parser->get_length());
    uint32_t checked = 0;
    if ((checked = calc_checksum(nullptr, ipv4_packet, 0, 5 * 4)) != 0x00) {
        return;
    }

    switch (ipv4_packet->protocol) {
        case IP_TCP:
            handle_tcp(session, buffer_parser);
            break;
        case IP_UDP:
            handle_udp(session, buffer_parser);
            break;
        default:
            printf("Unknown protocol %d\n", ipv4_packet->protocol);
            break;
    }
}
