#include "ksocket.h"

#include "../user_programs/system_calls.h"
#include "arp.h"
#include "ipv4.h"
#include "tcp.h"
#include "timer.h"

#define TCP_STATUS_PRIOR_HANSHAKE 0
#define TCP_STATUS_SYN_SENT 1
#define TCP_STATUS_SYN_RECV 2
#define TCP_STATUS_ESTABLISHED 3

#define TCP_STATUS_RECV_ACK (1 << 0)
#define TCP_STATUS_RECV_DATA_ACK (1 << 1)
#define TCP_STATUS_OLD_PACKET (1 << 2)
#define TCP_STATUS_RE_ACK (1 << 3)
#define TCP_STATUS_HANDSHAKE_CMPLT (1 << 4)
#define TCP_STATUS_RETRANS (1 << 5)
#define TCP_STATUS_IGNORE (1 << 6)

int _light_parse(PacketParser<EthernetFrame, IPv4Packet, TCPPacket>&& parser, uint32_t rcv,
                 uint32_t seq, uint8_t stage) {
    auto ip_packet = parser.get<IPv4Packet>();
    auto tcp_packet = parser.get<TCPPacket>();
    uint16_t bytes = ip_packet->total_length.get() - (ip_packet->ihl * 4) -
                     (tcp_packet->data_offset_reserved_flags.get_data_offset() * 4);

    if ((tcp_packet->sequence_number.get() < rcv || tcp_packet->ack_number.get() < seq) &&
        stage == TCP_STATUS_ESTABLISHED)
        return 0x2;
    if ((tcp_packet->data_offset_reserved_flags.get_flags() & (TCP_FLAG_ACK | TCP_FLAG_FIN)) ==
            0x0 &&
        bytes == 0) {
        return 0x1;
    }
    return 0;
}

void ISocket::enqueue(uint8_t* buffer, size_t length) {
    if (protocol == IP_TCP) {
        if (auto err =
                _light_parse(PacketParser<EthernetFrame, IPv4Packet, TCPPacket>(buffer, length),
                             status.tcp.rcv_nxt, status.tcp.snd_una, status.tcp.flags.stage)) {
            if (err == 0x1) this->handle_tcp(buffer, nullptr);
            return;
        }
    }

    num_empty.down();

    uint8_t mine = enqueue_idx;
    enqueue_idx = (enqueue_idx + 1) % QUEUE_MAX_SIZE;
    memcpy(recv_buffer + (mine * 1520), buffer, length);
    size_t size;
    // printf("[ISocket::enqueue] queue_size = %d\n", num_queue.value + 1);
    num_queue.up();

    // printf("QUEUED PACKET!\n");
    // printf("num_queue::");
}

size_t ISocket::dequeue(uint8_t* buffer) {
    num_queue.down();

    uint8_t* slot = recv_buffer + (dequeue_idx * 1520);
    dequeue_idx = (dequeue_idx + 1) % QUEUE_MAX_SIZE;

    size_t payload_length = 0;
    switch (protocol) {
        case IP_TCP:
            payload_length = handle_tcp(slot, buffer);
            break;
        case IP_UDP:
            payload_length = handle_udp(slot, buffer);
            break;
        default:
            printf("unknown protocol\n");
            break;
    }
    num_empty.up();

    // printf("num_empty::");

    printf("Payload length: %d\n", payload_length);

    return payload_length;
}

size_t ISocket::recv(uint8_t* buffer) {
    size_t length;
    while ((status.tcp.flags.alive || protocol == IP_UDP) && (length = dequeue(buffer)) == 0) {
        printf("recv: waiting for packet\n");
        // wait_msec(1);
    }
    printf("recv: got packet\n");
    return length;
}

size_t ISocket::handle_tcp(uint8_t* slot, uint8_t* buffer) {
    PacketParser<EthernetFrame, IPv4Packet, TCPPacket, Payload> other(slot, 1514);

    auto ip_packet = other.get<IPv4Packet>();
    auto tcp_packet = other.get<TCPPacket>();

    uint16_t flags = tcp_packet->data_offset_reserved_flags.get_flags();

    if (is_configured() && status.tcp.flags.stage != TCP_STATUS_PRIOR_HANSHAKE &&
        (this->pseudo.dst_ip.get() != ip_packet->src_address.get())) {
        printf("rejected packet.\n");
        return 0;
    }

    uint32_t bytes_received = ip_packet->total_length.get() - (ip_packet->ihl * 4) -
                              (tcp_packet->data_offset_reserved_flags.get_data_offset() * 4);

    if (!is_configured() && (flags & TCP_FLAG_SYN)) {
        configure(&other);
        status.tcp.rcv_nxt = tcp_packet->sequence_number.get() + 1;
        status.tcp.snd_nxt = 1;
        status.tcp.snd_una = 1;
        status.tcp.flags.stage = TCP_STATUS_SYN_RECV;
        send(nullptr, 0, TCP_FLAG_SYN | TCP_FLAG_ACK);
        return 0;
    }

    uint16_t src_port_id = tcp_packet->src_port.get();
    uint16_t port_id = tcp_packet->dst_port.get();

    if (this->status.tcp.flags.stage && (flags & TCP_FLAG_RST)) {
        this->close();
        return 0;
    }

    if (flags & TCP_FLAG_FIN) {
        send(nullptr, 0, TCP_FLAG_ACK | TCP_FLAG_FIN);
        close();
        return 0;
    }

    if (status.tcp.flags.stage == TCP_STATUS_SYN_RECV && (flags & TCP_FLAG_ACK)) {
        status.tcp.flags.stage = TCP_STATUS_HANDSHAKE_CMPLT;
        status.tcp.rcv_nxt = tcp_packet->sequence_number.get();
        status.tcp.snd_una = tcp_packet->ack_number.get();
        status.tcp.snd_nxt = tcp_packet->ack_number.get();
        return 0;
    }

    uint32_t pckt_seq_num = tcp_packet->sequence_number.get();
    uint32_t pckt_ack_num = tcp_packet->ack_number.get();

    if (flags & TCP_FLAG_ACK) {
        if (pckt_ack_num > status.tcp.snd_una && pckt_ack_num <= status.tcp.snd_nxt) {
            status.tcp.snd_una = pckt_ack_num;
        }

        if (pckt_seq_num == status.tcp.rcv_nxt) {
            status.tcp.rcv_nxt = status.tcp.rcv_nxt + bytes_received;

            memcpy(buffer, other.get<Payload>(), bytes_received);
            return bytes_received;
        }
    }
    return 0;
}

size_t ISocket::handle_udp(uint8_t* slot, uint8_t* buffer) {
    PacketParser<EthernetFrame, IPv4Packet, UDPDatagram, Payload> other(slot, 1514);

    auto ip_packet = other.get<IPv4Packet>();
    auto udp_packet = other.get<UDPDatagram>();
    auto payload = other.get<Payload>();

    int payload_length = 512;  // hard coding rn bc im a chad.
    memcpy(buffer, payload, payload_length);
    return payload_length;
}

void ISocket::send(uint8_t* buffer, size_t length, uint16_t response_flags) {
    K::assert(length < 1515, "buffer too long");

    PacketParser<EthernetFrame, IPv4Packet, TCPPacket, Payload> parser(send_buffer, 1514);
    auto ipv4 = parser.get<IPv4Packet>();
    auto tcp = parser.get<TCPPacket>();
    auto payload = parser.get<Payload>();

    if (length > 0) {
        memcpy(payload, buffer, length);
    }

    tcp->data_offset_reserved_flags.set_flags(response_flags);

    tcp->ack_number = status.tcp.rcv_nxt;
    tcp->sequence_number = status.tcp.snd_una;

    uint16_t header_length = tcp->data_offset_reserved_flags.get_data_offset() * 4;

    pseudo.length = header_length + length;

    tcp->checksum = 0;
    tcp->checksum = calc_checksum(&pseudo, tcp, sizeof(pseudo), pseudo.length.get());

    ipv4->total_length = header_length + length + (ipv4->ihl * 4);

    ipv4->header_check_sum = 0;
    ipv4->header_check_sum = calc_checksum(nullptr, ipv4, 0, ipv4->ihl * 4);

    send_packet(send_buffer, ipv4->total_length.get() + sizeof(ethernet_header));

    status.tcp.snd_nxt += length;
    if (response_flags & TCP_FLAG_SYN) status.tcp.snd_nxt += 1;
    if (response_flags & TCP_FLAG_FIN) status.tcp.snd_nxt += 1;
}

void UDPSocket::send_udp(const uint8_t* buffer, size_t length) {
    PacketParser<EthernetFrame, IPv4Packet, UDPDatagram, Payload> parser(send_buffer, 1514);
    auto ipv4 = parser.get<IPv4Packet>();
    auto udp = parser.get<UDPDatagram>();
    auto payload = parser.get<Payload>();

    udp->total_length = length;
    udp->src_port = src_port;
    udp->dst_port = dst_port;
    memcpy(payload, buffer, length);

    send_packet(send_buffer, ipv4->total_length.get() + sizeof(ethernet_header));
}

void UDPSocket::_initialize() {
    K::memset(send_buffer, 0, sizeof(send_buffer));
    size_t len;
    ETHFrameBuilder{get_mac_address(), get_mac_address(), 0x0800}
        .encapsulate(IPv4Builder{}
                         .with_src_address(dhcp_state.my_ip)
                         .with_dst_address(dhcp_state.my_ip)
                         .with_protocol(IP_UDP)
                         .encapsulate(UDPBuilder{dst_port, dst_port}.with_pseduo_header(
                             dhcp_state.my_ip, dhcp_state.my_ip)))
        .build(send_buffer, &len);
}

void ServerSocket::_initialize() {
    K::memset(send_buffer, 0, sizeof(send_buffer));
    size_t len;
    ETHFrameBuilder{get_mac_address(), get_mac_address(), 0x0800}
        .encapsulate(IPv4Builder{}
                         .with_src_address(dhcp_state.my_ip)
                         .with_dst_address(dhcp_state.my_ip)
                         .with_protocol(IP_TCP)
                         .encapsulate(TCPBuilder{port, port}
                                          .with_pseduo_header(dhcp_state.my_ip, dhcp_state.my_ip)
                                          .with_data_offset(5)
                                          .with_urgent_pointer(0)
                                          .with_window_size(0xFFFF)))
        .build(send_buffer, &len);
}

void ISocket::configure(PacketParser<EthernetFrame, IPv4Packet, TCPPacket, Payload>* other) {
    if (!configured) {
        configured = true;
        PacketBufferParser buffer_parser(send_buffer, 1514);

        auto frame = other->get<EthernetFrame>();
        auto ipv4 = other->get<IPv4Packet>();
        auto tcp = other->get<TCPPacket>();

        auto our_frame = buffer_parser.pop<EthernetFrame>();
        auto our_ipv4 = buffer_parser.pop<IPv4Packet>();
        auto our_tcp = buffer_parser.pop<TCPPacket>();

        memcpy(our_frame->src_mac, frame->dst_mac, 6);
        memcpy(our_frame->dst_mac, frame->src_mac, 6);
        our_ipv4->dst_address = ipv4->src_address.get();
        our_ipv4->src_address = ipv4->dst_address.get();

        our_tcp->dst_port = tcp->src_port.get();
        our_tcp->src_port = tcp->dst_port.get();

        pseudo.protocol = IP_TCP;
        pseudo.zero = 0x00;
        pseudo.src_ip = ipv4->dst_address.get();
        pseudo.dst_ip = ipv4->src_address.get();
    }
}

HashMap<uint16_t, ISocket*>* get_open_sockets() {
    static HashMap<uint16_t, ISocket*>* sockets = nullptr;
    if (sockets == nullptr)
        sockets = new HashMap<uint16_t, ISocket*>(uint16_t_hash, uint16_t_equals, 20);
    return sockets;
}