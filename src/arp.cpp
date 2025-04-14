#include "arp.h"

#include "dhcp.h"
#include "dns.h"
#include "libk.h"
#include "listener.h"
#include "network_card.h"
#include "udp.h"

extern "C" void __cxa_atexit(void (*func)(void *), void *arg, void *dso_handle) {
}

void *__dso_handle = nullptr;

extern "C" int __cxa_guard_acquire(int *guard) {
    return !*guard;
}

extern "C" void __cxa_guard_release(int *guard) {
    *guard = 1;
}

extern "C" void __cxa_guard_abort(int *) {
}

HashMap<uint64_t, uint8_t *> &get_arp_cache() {
    static HashMap<uint64_t, uint8_t *> arp_cache(uint64_t_hash, uint64_t_equals, 30);
    return arp_cache;
}

bool arp_has_resolved(uint32_t dst_ip) {
    return get_arp_cache().get(dst_ip) != nullptr;
}

void arp_resolve_mac(usb_session *session, uint32_t dst_ip, Function<void(uint8_t *)> &&consumer) {
    if (get_arp_cache().get(dst_ip) != nullptr) {
        consumer(get_arp_cache().get(dst_ip));
        return;
    }

    arp_packet packet;
    packet.hardware_type = ARP_ETHERNET_TYPE;
    packet.protocol_type = 0x0800;
    packet.hardware_size = 0x06;
    packet.protocol_size = 0x04;
    packet.opcode = ARP_REQUEST_OPCODE;
    memcpy(packet.sender_mac, get_mac_address(), 6);
    packet.sender_ip = dhcp_state.my_ip;
    memcpy(packet.target_mac, (uint8_t[6]){0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, 6);
    packet.target_ip = dst_ip;

    size_t length;
    ethernet_header *frame;
    frame = ETHFrameBuilder{packet.sender_mac, packet.target_mac, 0x0806}
                .encapsulate(PayloadBuilder{(uint8_t *)&packet, sizeof(packet)})
                .build(nullptr, &length);

    send_packet(session, (uint8_t *)frame, length);

    delete frame;
    get_event_handler().register_listener(ARP_RESOLVED_EVENT | dst_ip,
                                          new Listener<uint8_t *>(std::move(consumer)));
}

void send_arp_response(usb_session *session, arp_packet *request) {
    arp_packet packet;
    packet.hardware_type = ARP_ETHERNET_TYPE;
    packet.protocol_type = 0x0800;
    packet.hardware_size = 6;
    packet.protocol_size = 4;
    packet.opcode = ARP_REPLY_OPCODE;

    memcpy(packet.sender_mac, get_mac_address(), 6);
    memcpy(&packet.sender_ip, &((arp_packet *)request)->target_ip, 4);
    memcpy(packet.target_mac, ((arp_packet *)request)->sender_mac, 6);
    memcpy(&packet.target_ip, &((arp_packet *)request)->sender_ip, 4);

    size_t len;
    ethernet_header *frame =
        ETHFrameBuilder{get_mac_address(), ((arp_packet *)request)->sender_mac, 0x0806}
            .encapsulate(PayloadBuilder{(uint8_t *)&packet, sizeof(packet)})
            .build(nullptr, &len);

    send_packet(session, (uint8_t *)frame, len);

    delete frame;
}

int handle_arp_packet(usb_session *session, PacketBufferParser *parser) {
    auto arp_packet = parser->pop<ARPPacket>();

    if (arp_packet->opcode.get() == ARP_REQUEST_OPCODE) {
        send_arp_response(session, arp_packet);
    } else if (arp_packet->opcode.get() == ARP_REPLY_OPCODE) {
        uint32_t sender_ip = arp_packet->sender_ip.get();
        uint8_t *sender_mac = arp_packet->sender_mac;

        uint8_t *mac_malloced = (uint8_t *)kmalloc(6);
        memcpy(mac_malloced, sender_mac, 6);

        get_arp_cache().put(sender_ip, mac_malloced);

        get_event_handler().handle_event(ARP_RESOLVED_EVENT | sender_ip, mac_malloced);
    }

    return 0;
}