#include "ip.h"

#include "heap.h"
#include "libk.h"
#include "printf.h"

template <size_t N>
uint16_t calc_checksum(uint16_t (&header)[N]) {
    uint16_t checksum;
    for (size_t i = 0; i < N; i++) {
        uint16_t cur_word = header[i];
        checksum += cur_word;
        checksum = (checksum & 0xFFFF) + (checksum >> 16);
    }
    return (~checksum) & 0xFFFF;
}

IPv4Builder& IPv4Builder::with_src_address(uint32_t src_addr) {
    this->internal->header.src_address = src_addr;
    return *this;
}

IPv4Builder& IPv4Builder::with_dst_address(uint32_t dst_addr) {
    this->internal->header.dst_address = dst_addr;
    return *this;
}

IPv4Builder& IPv4Builder::with_protocol(uint8_t protocol) {
    K::assert(protocol == IP_UDP || protocol == IP_TCP || protocol == IP_ICMP,
              "speficifed protocol type is not supported");
    this->internal->header.protocol = protocol;
    return *this;
}

template <typename lambda>
static void* get(void* value, lambda if_null) {
    if (value != nullptr) return value;
    return reinterpret_cast<void*>(if_null());
}

ethernet_frame* ETHFrameBuilder::build_at(void* addr) {
    this->built_packet = reinterpret_cast<ethernet_frame*>(
        get(addr, [this]() -> void* { return kmalloc(this->get_size()); }));

    size_t offset = 0;
    uint8_t* header_ptr = reinterpret_cast<uint8_t*>(&this->internal->header);

    for (offset = 0; offset < sizeof(this->internal->header); offset++) {
        reinterpret_cast<uint8_t*>(this->built_packet)[offset] = header_ptr[offset];
    }

    uint8_t* encapsulated_ptr = nullptr;
    size_t encapsulated_length = 0;
    if (this->encapsulated) {
        this->encapsulated->build_at(((uint8_t*)this->built_packet) +
                                     sizeof(ethernet_frame_header));
    }

    return this->built_packet;
}

ipv4_packet* IPv4Builder::build_at(void* addr) {
    this->built_packet = reinterpret_cast<ipv4_packet*>(
        get(addr, [this]() -> void* { return kmalloc(this->get_size()); }));

    size_t offset = 0;
    uint8_t* header_ptr = reinterpret_cast<uint8_t*>(&this->internal->header);
    uint8_t* ptr = reinterpret_cast<uint8_t*>(this->built_packet);

    this->internal->header.header_check_sum = 0;
    this->internal->header.header_check_sum = calc_checksum(
        reinterpret_cast<uint16_t(&)[10]>(*reinterpret_cast<uint16_t*>(this->internal)));
    this->internal->header.total_length = get_size();
    printf("");  // don't delete
    for (offset = 0; offset < sizeof(this->internal->header); offset++) {
        ptr[offset] = header_ptr[offset];
    }
    if (this->encapsulated) {
        this->encapsulated->build_at(((uint8_t*)this->built_packet) + sizeof(ipv4_packet_header));
    } else if (this->data_length > 0) {
        for (size_t doff; doff < this->data_length; doff++, offset++) {
            ptr[offset] = reinterpret_cast<uint8_t*>(this->internal->data)[doff];
        }
    }

    return this->built_packet;
}

udp_datagram* UDPBuilder::build_at(void* addr) {
    this->internal->header.checksum = 0;
    this->internal->header.checksum = calc_checksum(
        reinterpret_cast<uint16_t(&)[2]>(*reinterpret_cast<uint16_t*>(this->internal)));

    this->internal->header.length = sizeof(udp_header) + this->data_length;

    uint8_t* header_ptr = reinterpret_cast<uint8_t*>(&this->internal->header);

    this->built_packet = reinterpret_cast<udp_datagram*>(
        get(addr, [this]() -> void* { return kmalloc(this->get_size()); }));

    uint8_t* ptr = reinterpret_cast<uint8_t*>(built_packet);
    size_t offset;

    for (; offset < sizeof(this->internal->header); offset++) {
        ptr[offset] = header_ptr[offset];
    }
    if (this->encapsulated) {
        this->encapsulated->build_at(((uint8_t*)this->built_packet) + sizeof(ipv4_packet_header));
    } else if (this->data_length > 0) {
        for (size_t doff; doff < this->data_length; doff++, offset++) {
            ptr[offset] = reinterpret_cast<uint8_t*>(this->internal->data)[doff];
        }
    }

    return this->built_packet;
}