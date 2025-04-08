#include "icmp.h"

#include "net_stack.h"
#include "network_card.h"
#include "printf.h"

void handle_icmp_echo(usb_session *session,
                      PacketParser<EthernetFrame, IPv4Packet, ICMPPacket, Payload> *parser) {
    auto eth_frame = parser->get<EthernetFrame>();
    auto ipv4_packet = parser->get<IPv4Packet>();
    auto icmp_packet = parser->get<ICMPPacket>();
    auto payload = parser->get<Payload>();

    size_t payload_length =
        ipv4_packet->total_length.get() - ipv4_packet->ihl * 4 - sizeof(icmp_header);

    size_t len;
    ethernet_header *frame;
    frame =
        ETHFrameBuilder{eth_frame->dst_mac, eth_frame->src_mac, eth_frame->ethertype.get()}
            .encapsulate(
                IPv4Builder{}
                    .with_src_address(ipv4_packet->dst_address.get())
                    .with_dst_address(ipv4_packet->src_address.get())
                    .with_protocol(IP_ICMP)
                    .encapsulate(
                        ICMPBuilder{ICMP_ECHO_REQUEST_TYPE, 0}
                            .with_remainder(
                                {.echo = {.identifier = icmp_packet->remainder.echo.identifier,
                                          .seq_number = icmp_packet->remainder.echo.seq_number}})
                            .encapsulate(PayloadBuilder{(uint8_t *)payload, payload_length})))
            .build(&len);

    send_packet(session, (uint8_t *)frame, len);

    delete frame;
}

void handle_icmp(usb_session *session, PacketBufferParser *buffer_parser) {
    auto icmp_packet = buffer_parser->pop<ICMPPacket>();
    PacketParser<EthernetFrame, IPv4Packet, ICMPPacket, Payload> parser(
        buffer_parser->get_packet_base(), buffer_parser->get_length());

    switch (icmp_packet->type) {
        case ICMP_ECHO_REQUEST_TYPE:
            handle_icmp_echo(session, &parser);
            break;
    }
}