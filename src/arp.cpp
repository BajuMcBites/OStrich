#include "arp.h"

#include "libk.h"
#include "network_card.h"
#include "printf.h"

void _debug_print_arp_packet(arp_packet *packet) {
    printf("arp_packet->opcode = 0x%x\n", packet->opcode);
    printf("ARP Packet:\n");
    printf("  Hardware Type: 0x%x\n", packet->hardware_type.get());
    printf("  Protocol Type: 0x%x\n", packet->protocol_type.get());
    printf("  Hardware Size: 0x%x\n", packet->hardware_size);
    printf("  Protocol Size: 0x%x\n", packet->protocol_size);
    printf("  Opcode: 0x%x\n", packet->opcode.get());

    printf("  Sender MAC: ");
    for (int i = 0; i < 6; i++) {
        printf("%02X ", packet->sender_mac[i]);
    }
    printf("\n");
    printf("  Sender IP: %d.%d.%d.%d\n", packet->sender_ip[0], packet->sender_ip[1],
           packet->sender_ip[2], packet->sender_ip[3]);
    printf("  Target MAC: ");
    for (int i = 0; i < 6; i++) {
        printf("%02X ", packet->target_mac[i]);
    }
    printf("\n");
    printf("  Target IP: %d.%d.%d.%d\n", packet->target_ip[0], packet->target_ip[1],
           packet->target_ip[2], packet->target_ip[3]);
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
    ethernet_header * frame =
        ETHFrameBuilder{mac_ptr, ((arp_packet *)request)->sender_mac, 0x0806}
            .encapsulate(PacketBuffer((uint8_t *)&packet, sizeof(packet)).ptr())
            .build(&len);
                
    send_packet(session, (uint8_t *)frame, len);
}

int handle_arp_packet(usb_session *session, PacketBufferParser *parser) {
    auto arp_packet = parser->pop<ARPPacket>();

    if (arp_packet->opcode.get() == 0x01) {
        send_arp_response(session, arp_packet);
    }

    return 0;
}