#include "arp.h"

#include "libk.h"
#include "network_card.h"
#include "printf.h"

void dns_query(usb_session *session, char *domain_name) {
}

void broadcast(usb_session *session, uint8_t unknown_ip[6]) {
}

void send_arp_response(usb_session *session, arp_packet *request) {
    arp_packet packet;
    packet.hardware_type = 0x0001;
    packet.protocol_type = 0x0800;
    packet.hardware_size = 6;
    packet.protocol_size = 4;
    packet.opcode = 0x0002;
    uint8_t *mac_ptr = get_mac_address();

    memcpy(packet.sender_mac, get_mac_address(), 6);
    memcpy(packet.sender_ip, ((arp_packet *)request)->target_ip, 4);
    memcpy(packet.target_mac, ((arp_packet *)request)->sender_mac, 6);
    memcpy(packet.target_ip, ((arp_packet *)request)->sender_ip, 4);

    size_t len;
    ethernet_header *frame =
        ETHFrameBuilder{mac_ptr, ((arp_packet *)request)->sender_mac, 0x0806}
            .encapsulate(PacketBufferBuilder((uint8_t *)&packet, sizeof(packet)).ptr())
            .build(&len);

    send_packet(session, (uint8_t *)frame, len);

    delete frame;
}

int handle_arp_packet(usb_session *session, PacketBufferParser *parser) {
    auto arp_packet = parser->pop<ARPPacket>();

    if (arp_packet->opcode.get() == 0x01) {
        send_arp_response(session, arp_packet);
    }

    return 0;
}