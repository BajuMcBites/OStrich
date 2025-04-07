#include "net_stack.h"

#include "heap.h"
#include "libk.h"
#include "printf.h"

extern "C" void* __dso_handle;

extern "C" int __cxa_atexit(void (*func)(void*), void* arg, void* dso) {
    // Do nothing (or store for later if you want custom handling)
    return 0;
}

// Define the dummy handle
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
 * From: https://www.packetmania.net/en/2021/12/26/IPv4-IPv6-checksum/
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
    /*  Add left-over byte, if any */
    if (n > 0) sum += *((uint8_t*)ptr);

    /*  Fold 32-bit sum to 16 bits */
    while (sum >> 16) sum = (sum & 0xffff) + (sum >> 16);

    return ~sum;
}

template <typename lambda>
static void* get(void* value, lambda if_null) {
    if (value != nullptr) return value;
    return reinterpret_cast<void*>(if_null());
}

void TCPBuilder::build_at(void* addr) {
    this->pseudo.length = this->internal->data_offset_reserved_flags.get_data_offset() * 4;
    this->pseudo.protocol = 0x06;
    this->pseudo.zero = 0x00;

    this->internal->checksum = 0;
    this->internal->checksum = calc_checksum(&this->pseudo, this->internal, sizeof(this->pseudo),
                                             this->pseudo.length.get());

    // this->internal->checksum = 0;
    uint32_t checked = calc_checksum(&this->pseudo, (void*)this->internal, sizeof(this->pseudo),
                                     this->pseudo.length.get());
    printf("tcp checked = 0x%x\n", checked);

    memcpy(addr, this->internal, sizeof(*this->internal));

    uint8_t* encapsulated_ptr = nullptr;
    size_t encapsulated_length = 0;
    if (this->encapsulated) {
        this->encapsulated->build_at(addr + sizeof(tcp_header));
    }
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
    size_t offset = 0;
    uint8_t* header_ptr = reinterpret_cast<uint8_t*>(this->internal);

    memcpy(addr, this->internal, sizeof(*this->internal));

    uint8_t* encapsulated_ptr = nullptr;
    if (this->encapsulated) {
        this->encapsulated->build_at(((uint8_t*)addr) + sizeof(ethernet_header));
    }
}

void IPv4Builder::build_at(void* addr) {
    size_t offset = 0;

    this->internal->header_check_sum = 0;
    this->internal->total_length = get_size();
    this->internal->header_check_sum =
        calc_checksum(nullptr, this->internal, 0, sizeof(ipv4_header));

    uint32_t checked = calc_checksum(nullptr, this->internal, 0, sizeof(ipv4_header));

    printf("");  // don't delete

    memcpy(addr, this->internal, sizeof(*this->internal));

    if (this->encapsulated) {
        this->encapsulated->build_at(((uint8_t*)addr) + sizeof(ipv4_header));
    }
}

void UDPBuilder::build_at(void* addr) {
    this->internal->checksum = 0;
    this->internal->checksum = calc_checksum(nullptr, this->internal, 0, sizeof(udp_header));

    this->internal->total_length = this->get_size();

    memcpy(addr, this->internal, sizeof(*this->internal));

    if (this->encapsulated) {
        this->encapsulated->build_at((addr) + sizeof(udp_header));
    }
}