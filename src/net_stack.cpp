#include "net_stack.h"

#include "heap.h"
#include "libk.h"
#include "printf.h"

extern "C" void* __dso_handle;

extern "C" int __cxa_atexit(void (*func)(void*), void* arg, void* dso) {
    return 0;
}

void* __dso_handle;

HashMap<void*> ports(20);
uint8_t port_bitmap[(1 << 16) >> 3];
SpinLock bitmap_lock;

port_status* get_port_status(uint16_t port) {
    return (port_status*)ports.get(port);
}

port_status* obtain_port(uint16_t port) {
    bitmap_lock.lock();
    if (port_bitmap[port >> 3] & (1 << (port & 0x7))) {
        bitmap_lock.unlock();
        return nullptr;
    }
    port_bitmap[port >> 3] |= (1 << (port & 0x7));
    bitmap_lock.unlock();

    port_status* status = (port_status*)kcalloc(0, sizeof(port_status));
    ports.put(port, status);
    return status;
}

// lot of trust in this :)
void release_port(uint16_t port) {
    port_status* status = (port_status*)ports.get(port);

    if (ports.remove(port)) {
        delete status;
        port_bitmap[port >> 3] &= ~(1 << (port & 0x7));
    }
}

uint16_t i16(uint16_t val) {
    return ((val & 0x00FF) << 8) | ((val & 0xFF00) >> 8);
}

uint32_t i32(uint32_t val) {
    return ((val & 0x000000FF) << 24) | ((val & 0x0000FF00) << 8) | ((val & 0x00FF0000) >> 8) |
           ((val & 0xFF000000) >> 24);
}

/*
 * Adapted from: https://www.packetmania.net/en/2021/12/26/IPv4-IPv6-checksum/
 */
uint16_t calc_checksum(void* pseudo, void* header, size_t pseudo_length, size_t length) {
    /* Compute Internet Checksum for "N" bytes
     * beginning at location "header".
     */
    uint32_t sum = 0;

    size_t n;
    uint16_t* ptr;

    if (pseudo != nullptr && pseudo_length > 0) {
        n = pseudo_length;
        ptr = (uint16_t*)pseudo;
        while (n > 1) {
            sum += i16(*ptr++);
            n -= 2;
        }
    }

    n = length;
    ptr = (uint16_t*)header;
    while (n > 1) {
        sum += i16(*ptr++);
        n -= 2;
    }
    if (n > 0) sum += i16(*((uint8_t*)ptr));

    while (sum >> 16) sum = (sum & 0xffff) + (sum >> 16);

    return ~sum;
}

template <typename lambda>
static void* get(void* value, lambda if_null) {
    if (value != nullptr) return value;
    return reinterpret_cast<void*>(if_null());
}

IPv4Builder& IPv4Builder::with_src_address(uint32_t src_addr) {
    this->internal->src_address = src_addr;
    return *this;
}

IPv4Builder& IPv4Builder::with_dst_address(uint32_t dst_addr) {
    this->internal->dst_address = dst_addr;
    return *this;
}

IPv4Builder& IPv4Builder::with_protocol(uint8_t protocol) {
    K::assert(protocol == IP_UDP || protocol == IP_TCP || protocol == IP_ICMP,
              "speficifed protocol type is not supported");
    this->internal->protocol = protocol;
    return *this;
}

void ETHFrameBuilder::build_at(void* addr) {
    memcpy(addr, this->internal, sizeof(*this->internal));

    if (this->encapsulated) {
        this->encapsulated->build_at(((uint8_t*)addr) + sizeof(ethernet_header));
    }
}

void IPv4Builder::build_at(void* addr) {
    this->internal->header_check_sum = 0;
    this->internal->total_length = get_size();
    this->internal->header_check_sum =
        calc_checksum(nullptr, this->internal, 0, sizeof(ipv4_header));

    memcpy(addr, this->internal, sizeof(*this->internal));

    if (this->encapsulated) {
        this->encapsulated->build_at(((uint8_t*)addr) + sizeof(ipv4_header));
    }
}

void UDPBuilder::build_at(void* addr) {
    size_t total_length = this->get_size();
    this->internal->total_length = total_length;

    this->pseudo.length = total_length;
    this->pseudo.protocol = 0x11;
    this->pseudo.zero = 0x00;

    if (this->encapsulated) {
        this->encapsulated->build_at((addr) + sizeof(udp_header));
    }
    memcpy(addr, this->internal, sizeof(*this->internal));

    this->internal->checksum = 0;
    this->internal->checksum =
        calc_checksum(&this->pseudo, addr, sizeof(this->pseudo), total_length);
}

void TCPBuilder::build_at(void* addr) {
    uint16_t header_length = this->internal->data_offset_reserved_flags.get_data_offset() * 4;

    if (this->encapsulated) {
        this->encapsulated->build_at(addr + 1 + header_length);
    }

    this->pseudo.length =
        header_length + (this->encapsulated != nullptr ? this->encapsulated->get_size() : 0);
    this->pseudo.protocol = 0x06;
    this->pseudo.zero = 0x00;

    this->internal->checksum = 0;
    memcpy(addr, this->internal, sizeof(*this->internal));

    ((tcp_header*)addr)->checksum =
        calc_checksum(&this->pseudo, addr, sizeof(this->pseudo), this->pseudo.length.get());
}