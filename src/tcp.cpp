#include "tcp.h"

#include "hash.h"
#include "net_stack.h"
#include "network_card.h"
#include "printf.h"

void cleanup_connection(uint16_t port_id) {
    printf("cleaning up connection on port %d\n\n", port_id);
    release_port(port_id);
}

void handle_receive_status(port_status *status, tcp_header *tcp, uint32_t bytes_received) {
    uint32_t seq = tcp->sequence_number.get();
    uint32_t ack = tcp->ack_number.get();
    uint16_t flags = tcp->data_offset_reserved_flags.get_flags();

    if (status->tcp.ack_number == 0 && flags & TCP_FLAG_SYN) {
        status->tcp.ack_number = seq + 1;
        status->tcp.seq_number = 0x05;
    } else if (flags & TCP_FLAG_ACK) {
        if (ack > status->tcp.seq_number) {
            status->tcp.seq_number = ack;
        }
        if (seq == status->tcp.ack_number) {
            // accept data that matches what we are expecting
            status->tcp.ack_number = status->tcp.ack_number + bytes_received;
        } else if (seq < status->tcp.ack_number) {
            // duplicate packet or retransmitted packet
            // but we have already received & ack'd this packet
            // so ignored
        } else {
            // can improve our tcp implementation by buffering data
            // from packet's w/ higher seq_nums than what we expect
        }
    }
}

void handle_tcp(usb_session *session, PacketBufferParser *buffer_parser) {
    PacketParser<EthernetFrame, IPv4Packet, TCPPacket> parser(buffer_parser->get_packet_base(),
                                                              buffer_parser->get_length());
    auto eth_frame = parser.get<EthernetFrame>();
    auto ip_packet = parser.get<IPv4Packet>();
    auto tcp_packet = buffer_parser->pop<TCPPacket>();

    pseduo_header header;
    header.src_ip.set_raw(ip_packet->src_address.network_raw);
    header.dst_ip.set_raw(ip_packet->dst_address.network_raw);
    header.zero = 0x00;
    header.protocol = IP_TCP;
    header.length =
        ip_packet->total_length.get() - (ip_packet->ihl * 4);  // total length of TCP header

    uint16_t checked = 0x00;

    if ((checked = calc_checksum(&header, tcp_packet, sizeof(header), header.length.get())) !=
        0x00) {
        printf("Received TCP packet with invalid checksum\n");
        printf("Check Checksum result: 0x%x\n", checked);
        return;
    }

    uint16_t flags = tcp_packet->data_offset_reserved_flags.get_flags();
    uint16_t port_id = tcp_packet->dst_port.get();

    uint32_t bytes_received =
        header.length.get() - tcp_packet->data_offset_reserved_flags.get_data_offset() * 4;

    if (flags & TCP_FLAG_RST) {
        cleanup_connection(port_id);
        return;
    }

    uint16_t respond_flags = 0x000;

    if (flags & TCP_FLAG_FIN) {
        respond_flags |= TCP_FLAG_ACK;
    }

    port_status *status = get_port_status(port_id);

    if (flags & TCP_FLAG_SYN) {
        respond_flags |= TCP_FLAG_SYN | TCP_FLAG_ACK;
        if (status == nullptr && (status = obtain_port(port_id)) == nullptr) {
            // port is already in use and doesn't belong to me :/
            return;
        }
    }

    if (flags & TCP_FLAG_ACK) {
        respond_flags |= TCP_FLAG_ACK;
    }

    if (status == nullptr) {
        // port is not in use?
        // there should be a port_status associated with this
        // tcp packet if it is valid
        return;
    }

    handle_receive_status(status, tcp_packet, bytes_received);

    PayloadBuilder *payload = nullptr;
    char *response_payload = nullptr;
    size_t response_length = 0;

    if (bytes_received > 0) {
        auto receive_buffer = buffer_parser->pop<Payload>();

        char print_buffer[bytes_received + 1];
        memcpy(print_buffer, receive_buffer, bytes_received);
        print_buffer[bytes_received] = '\0';

        printf("received payload (%d): %s\n", bytes_received, print_buffer);
        response_payload = "Hello from server!";
        response_length = 19;
    }

    size_t packet_length;

    ethernet_header *response;
    response =
        ETHFrameBuilder{eth_frame->dst_mac, eth_frame->src_mac, 0x0800}
            .encapsulate(
                IPv4Builder{}
                    .with_src_address(ip_packet->dst_address.get())
                    .with_dst_address(ip_packet->src_address.get())
                    .with_protocol(IP_TCP)
                    .encapsulate(
                        TCPBuilder{tcp_packet->dst_port.get(), tcp_packet->src_port.get()}
                            .with_seq_number(status->tcp.seq_number)
                            .with_ack_number(status->tcp.ack_number)
                            .with_flags(respond_flags)
                            .with_data_offset(5)
                            .with_window_size(0xFFFF)
                            .with_urgent_pointer(0)
                            .with_pseduo_header(ip_packet->src_address.get(),
                                                ip_packet->dst_address.get())
                            .encapsulate(PayloadBuilder{(uint8_t*)response_payload, response_length})))
            .build(&packet_length);

    send_packet(session, (uint8_t *)response, packet_length);

    delete response;

    if (flags & TCP_FLAG_FIN) {
        cleanup_connection(port_id);
    }
}
