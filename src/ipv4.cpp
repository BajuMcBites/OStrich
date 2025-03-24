#include "ipv4.h"

#include "network_card.h"
#include "tcp.h"

void _ipv4_print(PacketParser<EthernetFrame, IPv4Packet, TCPPacket> *parser) {
    auto ip_packet = parser->get<IPv4Packet>();
    auto tcp_packet = parser->get<TCPPacket>();

    printf("frame->ethertype() = 0x%x\n", parser->get<EthernetFrame>()->ethertype.get());

    // print out ip_packet
    printf("ip_packet->version.get() = 0x%x\n", ip_packet->version);
    printf("ip_packet->ihl.get() = 0x%x\n", ip_packet->ihl);
    printf("ip_packet->src_address.get() = 0x%x\n", ip_packet->src_address.get());
    printf("ip_packet->dst_address.get() = 0x%x\n", ip_packet->dst_address.get());
    printf("ip_packet->protocol = 0x%x\n", ip_packet->protocol);
    printf("ip_packet->header_check_sum.get() = 0x%x\n", ip_packet->header_check_sum.get());
    printf("ip_packet->total_length.get() = 0x%x\n", ip_packet->total_length.get());
    printf("ip_packet->ttl = 0x%x\n", ip_packet->ttl);
    printf("ip_packet->version_ihl = 0x%x\n", ip_packet->ihl);
    printf("ip_packet->flags.raw = 0x%x\n", ip_packet->get_flags());
    printf("ip_packet->fragment_offset.get() = 0x%x\n", ip_packet->get_fragment_offset());
    printf("ip_packet->identification.get() = 0x%x\n", ip_packet->identification.get());
    printf("ip_packet->type_of_service = 0x%x\n", ip_packet->type_of_service);

    printf("tcp_packet->src_port.get() = 0x%x\n", tcp_packet->src_port.get());
    printf("tcp_packet->dst_port.get() = 0x%x\n", tcp_packet->dst_port.get());
    printf("tcp_packet->sequence_number.get() = 0x%x\n", tcp_packet->sequence_number.get());
    printf("tcp_packet->ack_number.get() = 0x%x\n", tcp_packet->ack_number.get());
    printf("tcp_packet->data_offset_reserved_flags.get() = 0x%x\n",
           tcp_packet->data_offset_reserved_flags.value.get());

    printf("tcp_packet->flags.get() = 0x%x\n", tcp_packet->data_offset_reserved_flags.get_flags());
    printf("tcp_packet->window.get() = 0x%x\n", tcp_packet->window.get());

    printf("tcp_packet->checksum.get() = %d\n", tcp_packet->checksum.get());
    printf("tcp_packet->urgent_pointer.get() = %d\n", tcp_packet->urgent_pointer.get());
}

// uint16_t _temp_calc_checksum(void *header, size_t length) {
//     /* Compute Internet Checksum for "N" bytes
//      * beginning at location "header".
//      */
//     uint32_t sum = 0;

//     size_t n = length;
//     uint16_t *ptr = (uint16_t *)header;
//     while (n > 1) {
//         sum += *ptr++;
//         n -= 2;
//     }
//     /*  Add left-over byte, if any */
//     if (n) sum += ((uint8_t *)header)[length - 1];

//     /*  Fold 32-bit sum to 16 bits */
//     while (sum >> 16) sum = (sum & 0xffff) + (sum >> 16);

//     return ~sum;
// }

void handle_ipv4_packet(usb_session *session, PacketBufferParser *buffer_parser) {
    auto ipv4_packet = buffer_parser->pop<IPv4Packet>();

    PacketParser<EthernetFrame, IPv4Packet, TCPPacket> parser(buffer_parser->get_packet_base(),
                                                              buffer_parser->get_length());
    _ipv4_print(&parser);

    // uint16_t checked = 0x00;
    // if ((checked = _temp_calc_checksum(ipv4_packet, 5 * 4)) != 0x00) {
    //     printf("Received ipv4 packet with invalid checksum\n");
    //     printf("Checksum: 0x%x\n", checked);
    //     return;
    // }

    switch (ipv4_packet->protocol) {
        case IP_TCP:
            handle_tcp(session, buffer_parser);
            break;
        default:
            printf("Unknown protocol %d\n", ipv4_packet->protocol);
            break;
    }
}
