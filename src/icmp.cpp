#include "icmp.h"

#include "arp.h"
#include "dhcp.h"
#include "listener.h"
#include "net_stack.h"
#include "network_card.h"
#include "timer.h"

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
                                          .seq_number =
                                              icmp_packet->remainder.echo.seq_number.get() +
                                              (icmp_packet->type == ICMP_ECHO_REPLY_TYPE ? 1 : 0)}})
                            .encapsulate(PayloadBuilder{(uint8_t *)payload, payload_length})))
            .build(nullptr, &len);

    send_packet((uint8_t *)frame, len);

    delete frame;

    PacketParser<ICMPPacket, Payload> event_parser((const uint8_t *)icmp_packet,
                                                   payload_length + sizeof(icmp_header));

        get_event_handler().handle_event(ICMP_PING_EVENT | ipv4_packet->src_address.get(),
                                         &event_parser);
}

void icmp_ping(usb_session *session, uint32_t ip,
               Function<void(PacketParser<ICMPPacket, Payload> *)> &&callback) {
    size_t len;
    ethernet_header *frame;

    frame =
        ETHFrameBuilder{get_mac_address(), get_arp_cache().get(dhcp_state.dhcp_server_ip), 0x0800}
            .encapsulate(IPv4Builder{}
                             .with_src_address(dhcp_state.my_ip)
                             .with_dst_address(ip)
                             .with_protocol(IP_ICMP)
                             .encapsulate(ICMPBuilder{ICMP_ECHO_REQUEST_TYPE, 0}.with_remainder(
                                 {.echo = {.identifier = 22, .seq_number = 1}})))
            .build(nullptr, &len);

    send_packet((uint8_t *)frame, len);

    delete frame;

    get_event_handler().register_listener(ICMP_PING_EVENT | ip, new Listener(std::move(callback)));
    uint8_t *ptr = get_arp_cache().get(dhcp_state.dhcp_server_ip);
}

void handle_icmp(usb_session *session, PacketBufferParser *buffer_parser) {
    auto icmp_packet = buffer_parser->pop<ICMPPacket>();
    PacketParser<EthernetFrame, IPv4Packet, ICMPPacket, Payload> parser(
        buffer_parser->get_packet_base(), buffer_parser->get_length());

    switch (icmp_packet->type) {
        case ICMP_ECHO_REPLY_TYPE:
        case ICMP_ECHO_REQUEST_TYPE: {
            handle_icmp_echo(session, &parser);
            break;
        }
    }
}