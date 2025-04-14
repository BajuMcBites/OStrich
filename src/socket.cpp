#include "socket.h"

#include "arp.h"


void Socket::_initialize() {
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
    parser = new PacketParser<EthernetFrame, IPv4Packet, TCPPacket, Payload>(send_buffer, 1514);
}

void Socket::send(uint8_t* buffer, size_t length) {
    K::assert(length < 1515, "buffer too long");

    auto ipv4 = parser->get<IPv4Packet>();
    auto tcp = parser->get<TCPPacket>();
    auto payload = parser->get<Payload>();

    memcpy(payload, buffer, length);

    tcp->data_offset_reserved_flags.set_flags(0x010);
    tcp->ack_number = status->tcp.ack_number;
    tcp->sequence_number = status->tcp.seq_number;

    uint16_t header_length = tcp->data_offset_reserved_flags.get_data_offset() * 4;

    pseduo_header pseudo;
    pseudo.length = header_length + length;
    pseudo.protocol = 0x06;
    pseudo.zero = 0x00;
    pseudo.src_ip = ipv4->src_address.get();
    pseudo.dst_ip = ipv4->dst_address.get();

    tcp->checksum = 0;
    tcp->checksum = calc_checksum(&pseudo, tcp, sizeof(pseudo), pseudo.length.get());

    ipv4->total_length = header_length + length + (ipv4->ihl * 4);
    ipv4->header_check_sum = 0;

    ipv4->header_check_sum = calc_checksum(nullptr, ipv4, 0, ipv4->ihl * 4);

    send_packet(network_usb_session(nullptr), send_buffer,
                ipv4->total_length.get() + sizeof(ethernet_header));
}

void Socket::recv(ethernet_header* frame, ipv4_header* ipv4, tcp_header* tcp, size_t length) {
    if (!configured) {
        configured = true;

        auto our_frame = parser->get<EthernetFrame>();
        auto our_ipv4 = parser->get<IPv4Packet>();
        auto our_tcp = parser->get<TCPPacket>();

        memcpy(our_frame->src_mac, frame->dst_mac, 6);
        memcpy(our_frame->dst_mac, frame->src_mac, 6);
        our_ipv4->dst_address = ipv4->src_address.get();
        our_ipv4->src_address = ipv4->dst_address.get();
        this->destination = ipv4->dst_address.get();

        our_tcp->dst_port = tcp->src_port.get();
        our_tcp->src_port = tcp->dst_port.get();
    }
    this->recv_callback(length);
}

HashMap<uint16_t, Socket*>& get_open_sockets() {
    static HashMap<uint16_t, Socket*> sockets =
        new HashMap<uint16_t, Socket*>(uint16_t_hash, uint16_t_equals, 20);
    return sockets;
}