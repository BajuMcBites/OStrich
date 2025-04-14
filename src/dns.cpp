#include "dns.h"

#include "arp.h"
#include "dhcp.h"
#include "libk.h"
#include "listener.h"
#include "network_card.h"

HashMap<const char*, server_group*>& get_dns_cache() {
    static HashMap<const char*, server_group*> dns_cache(string_hash, string_equals, 30);
    return dns_cache;
}

uint8_t* encode_dns_name(const char* name, uint8_t* buffer) {
    uint8_t* ptr = buffer;
    const char* label = name;
    const char* scan = name;

    while (true) {
        int len = 0;
        while (*scan != '.' && *scan != '\0') {
            ++len;
            ++scan;
        }

        *ptr++ = (uint8_t)len;

        for (int i = 0; i < len; ++i) {
            *ptr++ = *label++;
        }

        if (*scan == '.') {
            ++scan;
            label = scan;
        } else {
            break;
        }
    }

    *ptr++ = 0;
    return ptr;
}

uint8_t* parse_name(uint8_t* packet, uint8_t* ptr, char* out) {
    int out_len = 0;
    bool jumped = false;
    uint8_t* original_ptr = ptr;

    while (true) {
        uint8_t len = *ptr;

        if ((len & 0xC0) == 0xC0) {
            uint16_t offset = ((len & 0x3F) << 8) | ptr[1];
            ptr = packet + offset;
            if (!jumped) original_ptr += 2;
            jumped = true;
            continue;
        }

        if (len == 0) {
            if (!jumped) original_ptr = ptr + 1;
            break;
        }

        ptr++;
        if (out_len > 0) out[out_len++] = '.';

        for (int i = 0; i < len; i++) {
            out[out_len++] = ptr[i];
        }

        ptr += len;
    }

    out[out_len] = '\0';
    return original_ptr;
}

void dns_query(usb_session* session, const char* domain, Function<void(server_group*)>&& consumer) {
    if (get_dns_cache().get(domain) != nullptr) {
        return;
    }

    K::assert(dhcp_state.dns_servers.count > 0, "DHCP found no available DNS servers");

    dhcp_state.dns_servers.for_each([&session, &domain, &consumer](uint32_t dns_server_ip) {
        if (!arp_has_resolved(dns_server_ip)) {
            uint8_t* ip = (uint8_t*)&dns_server_ip;
            return;
        }

        uint8_t packet[512];

        dns_query_header* header = (dns_query_header*)packet;
        header->id = 0x6969;
        header->flags = (1 << 9);
        header->qdCount = 0x01;
        header->anCount = 0;
        header->nsCount = 0;
        header->arCount = 0;

        uint8_t* end = encode_dns_name(domain, packet + sizeof(dns_query_header));
        *(uint16_t*)end = i16(1);
        end += 2;
        *(uint16_t*)end = i16(1);
        end += 2;

        size_t len;
        ethernet_header* frame;
        frame = ETHFrameBuilder{get_mac_address(), get_arp_cache().get(dns_server_ip), 0x0800}
                    .encapsulate(IPv4Builder{}
                                     .with_src_address(dhcp_state.my_ip)
                                     .with_dst_address(dns_server_ip)
                                     .with_protocol(IP_UDP)
                                     .encapsulate(UDPBuilder{22, 53}.encapsulate(
                                         PayloadBuilder{packet, end - packet})))
                    .build(nullptr, &len);

        send_packet(session, (uint8_t*)frame, len);

        delete frame;

        get_event_handler().register_listener(
            DNS_RESOLVED_EVENT | (string_hash(domain) & 0xFFFFFFFF),
            new Listener<server_group*>(std::move(consumer)));
    });
}

// https://datatracker.ietf.org/doc/html/rfc1035#section-4.1.1
void handle_dns_response(usb_session* session,
                         PacketParser<EthernetFrame, IPv4Packet, UDPDatagram,
                                      Packet<dns_query_header>, Payload>* parser) {
    ipv4_header* ipv4_header = parser->get<IPv4Packet>();
    dns_query_header* header = parser->get<Packet<dns_query_header>>();

    auto payload = parser->get<Payload>();

    uint16_t flags = header->flags.get();
    uint8_t* ptr = payload;

    char domain_name[256];
    for (int i = 0; i < header->qdCount.get(); i++) {
        ptr = parse_name((uint8_t*)header, ptr, domain_name);

        uint16_t qtype = i16(*(uint16_t*)ptr);
        ptr += 2;
        uint16_t qclass = i16(*(uint16_t*)ptr);
        ptr += 2;
    }

    if (header->anCount.get() > 0) {
        server_group* group = (server_group*)kcalloc(1, sizeof(server_group));
        for (int i = 0; i < header->anCount.get() && group->count < 6; i++) {
            char name[256];
            ptr = parse_name((uint8_t*)header, ptr, name);
            uint16_t type = i16(*(uint16_t*)ptr);
            ptr += 2;
            uint16_t clazz = i16(*(uint16_t*)ptr);
            ptr += 2;
            uint32_t ttl = i32(*(uint32_t*)ptr);
            ptr += 4;
            uint16_t rdlen = i16(*(uint16_t*)ptr);
            ptr += 2;

            if (rdlen == 4 && string_equals(domain_name, name)) {
                uint32_t resolved_ip = i32(*((uint32_t*)ptr));
                group->ips[group->count++] = resolved_ip;
            }

            ptr += rdlen;
        }

        uint32_t len = K::strlen(domain_name);
        char* malloced_key = (char*)kmalloc(len);
        memcpy(malloced_key, domain_name, len);
        const char* key = (const char*)malloced_key;

        get_dns_cache().put(key, group);

        uint64_t event_id = DNS_RESOLVED_EVENT | (string_hash(domain_name) & 0xFFFFFFFF);
        get_event_handler().handle_event(event_id, group);
    }
}
