#include "tcp.h"

#include "arp.h"
#include "event.h"
#include "ksocket.h"
#include "network_card.h"
#include "printf.h"

void ip_send(uint8_t *buffer, size_t length) {
    PacketParser<EthernetFrame, IPv4Packet> parser(buffer, length);
    usb_session *session = network_usb_send_session(nullptr);

    auto ipv4 = parser.get<IPv4Packet>();
    if (arp_has_resolved(ipv4->dst_address.get())) {
        send_packet(buffer, length);
        return;
    }

    arp_resolve_mac(session, ipv4->dst_address.get(), [&](uint8_t *dst_mac) {
        uint32_t dst_ip = ipv4->dst_address.get();
        uint8_t *ip = ((uint8_t *)&dst_ip);
        auto ethernet = parser.get<EthernetFrame>();

        memcpy(ethernet->dst_mac, dst_mac, 6);

        send_packet(buffer, length);
    });
}

void handle_tcp(usb_session *session, PacketBufferParser *buffer_parser) {
    PacketParser<EthernetFrame, IPv4Packet, TCPPacket, Payload> parser(
        buffer_parser->get_packet_base(), buffer_parser->get_length());
    auto eth_frame = parser.get<EthernetFrame>();
    auto ip_packet = parser.get<IPv4Packet>();
    auto tcp_packet = parser.get<TCPPacket>();

    pseduo_header header;
    header.src_ip.set_raw(ip_packet->src_address.network_raw);
    header.dst_ip.set_raw(ip_packet->dst_address.network_raw);
    header.zero = 0x00;
    header.protocol = IP_TCP;
    header.length = ip_packet->total_length.get() - (ip_packet->ihl * 4);

    uint16_t checked = 0x00;
    if ((checked = calc_checksum(&header, tcp_packet, sizeof(header), header.length.get())) !=
        0x00) {
        printf("Received TCP packet with invalid checksum\n");
        printf("Check Checksum result: 0x%x\n", checked);
        return;
    }

    uint16_t port_id = tcp_packet->dst_port.get();

    if (auto socket = get_open_sockets().get(port_id)) {
        socket->enqueue(buffer_parser->get_packet_base(), ip_packet->total_length.get() + sizeof(ethernet_header));
    }
}
