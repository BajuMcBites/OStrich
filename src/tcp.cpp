#include "tcp.h"

#include "hash.h"
#include "net_stack.h"
#include "network_card.h"

void _tcp_print(PacketParser<EthernetFrame, IPv4Packet, TCPPacket> *parser) {
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

void cleanup_connection(uint16_t port_id) {
    printf("cleaning up connection on port %d\n", port_id);
    release_port(port_id);
}

void handle_receive_status(port_status *status, tcp_header *tcp, uint32_t bytes_received) {
    if (status->tcp.ack_number == 0) {
        status->tcp.ack_number = tcp->sequence_number.get() + 1;
        status->tcp.seq_number = 0x6969;
        printf("updated ack & seq number!\n");
    } else if (tcp->data_offset_reserved_flags.get_flags() & TCP_FLAG_ACK) {
        if (tcp->sequence_number.get() == status->tcp.seq_number) {
            // accept data
            status->tcp.ack_number = status->tcp.ack_number + bytes_received;
        }
    }
}

void handle_tcp(usb_session *session, PacketBufferParser *buffer_parser) {
    PacketParser<EthernetFrame, IPv4Packet, TCPPacket> parser(buffer_parser->get_packet_base(),
                                                              buffer_parser->get_length());
    auto eth_frame = parser.get<EthernetFrame>();
    auto ip_packet = parser.get<IPv4Packet>();
    auto tcp_packet = buffer_parser->pop<TCPPacket>();

    uint16_t checked = 0x00;

    pseduo_header header;
    header.src_ip.set_raw(ip_packet->src_address.network_raw);
    header.dst_ip.set_raw(ip_packet->dst_address.network_raw);
    header.zero = 0x00;
    header.protocol = IP_TCP;
    header.length = tcp_packet->data_offset_reserved_flags.get_data_offset() * 4;

    uint32_t packet_checksum = tcp_packet->checksum.get();

    if ((checked = calc_checksum(&header, tcp_packet, sizeof(header), header.length.get())) !=
        0x00) {
        printf("Received TCP packet with invalid checksum\n");
        printf("Check Checksum result: 0x%x\n", checked);
        return;
    }

    printf("\n\nTCP response\n\n");

    uint16_t flags = tcp_packet->data_offset_reserved_flags.get_flags();
    uint16_t port_id = tcp_packet->dst_port.get();

    uint32_t bytes_received =
        buffer_parser->get_length() -
        ((((uintptr_t)tcp_packet) + tcp_packet->data_offset_reserved_flags.get_data_offset() * 4) -
         (uintptr_t)(buffer_parser->get_packet_base()));

    if (flags & TCP_FLAG_RST) {
        cleanup_connection(port_id);
        return;
    }

    uint16_t respond_flags = 0x000;

    if (flags & TCP_FLAG_FIN) {
        respond_flags |= TCP_FLAG_FIN;
        respond_flags |= TCP_FLAG_ACK;
    }

    port_status *status = get_port_status(port_id);

    if (flags & TCP_FLAG_SYN) {
        respond_flags |= TCP_FLAG_SYN | TCP_FLAG_ACK;
        if (status == nullptr && (status = obtain_port(port_id)) == nullptr) {
            // port is already in use
            return;
        }
    }

    if (status == nullptr) {
        // port is not in use?
        // there should be a port_status associated with this
        // tcp packet if it is valid
        return;
    }

    handle_receive_status(status, tcp_packet, bytes_received);

    size_t packet_length;

    ethernet_header *response =
        ETHFrameBuilder{eth_frame->dst_mac, eth_frame->src_mac, 0x0800}
            .encapsulate(&IPv4Builder{}
                              .with_src_address(ip_packet->dst_address.get())
                              .with_dst_address(ip_packet->src_address.get())
                              .with_protocol(IP_TCP)
                              .encapsulate(&TCPBuilder{tcp_packet->dst_port.get(),
                                                       tcp_packet->src_port.get()}
                                                .with_seq_number(status->tcp.seq_number)
                                                .with_ack_number(status->tcp.ack_number)
                                                .with_flags(respond_flags)
                                                .with_data_offset(5)
                                                .with_window_size(0xFFFF)
                                                .with_urgent_pointer(0)
                                                .with_pseduo_header(ip_packet->src_address.get(),
                                                                    ip_packet->dst_address.get())))
            .build(&packet_length);

    PacketParser<EthernetFrame, IPv4Packet, TCPPacket> response_parser((uint8_t *)response,
                                                                       packet_length);
    _tcp_print(&response_parser);

    send_packet(session, (uint8_t *)response, packet_length);

    // delete response;

    if (flags & TCP_FLAG_FIN) {
        cleanup_connection(port_id);
    }
}
