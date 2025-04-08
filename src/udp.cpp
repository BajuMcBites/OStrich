#include "udp.h"

#include "dhcp.h"
#include "net_stack.h"
#include "printf.h"

void handle_udp(usb_session *session, PacketBufferParser *buffer_parser) {
    PacketParser<EthernetFrame, IPv4Packet, TCPPacket> parser(buffer_parser->get_packet_base(),
                                                              buffer_parser->get_length());

    auto ip_packet = parser.get<IPv4Packet>();
    auto udp_datagram = buffer_parser->pop<UDPDatagram>();

    pseduo_header header;
    header.src_ip.set_raw(ip_packet->src_address.network_raw);
    header.dst_ip.set_raw(ip_packet->dst_address.network_raw);
    header.zero = 0x00;
    header.protocol = IP_UDP;
    header.length = udp_datagram->total_length.get();

    uint16_t checked = 0x00;

    if ((checked = calc_checksum(&header, udp_datagram, sizeof(header), header.length.get())) !=
        0x00) {
        printf("Received udp packet with invalid checksum\n");
        printf("Checksum: 0x%x\n", checked);
        return;
    }

    uint16_t port = udp_datagram->dst_port.get();

    switch (port) {
        case 68: {  // DHCP
            PacketParser<EthernetFrame, IPv4Packet, UDPDatagram, DHCPPacket> parser(
                buffer_parser->get_packet_base(), buffer_parser->get_length());
            dhcp_handle_offer(&parser);
            break;
        }
        default:
            printf("Unable to handle UDP Datagram sent to port: %d\n", port);
            break;
    }
}