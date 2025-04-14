#ifndef _IP_H
#define _IP_H

#include "atomic.h"
#include "hash.h"
#include "heap.h"
#include "libk.h"
#include "stdint.h"
#include "tuple.h"

#define IP_ICMP 1  // Internet Control Message Protocol
#define IP_IGMP 2  // Internet Group Management
#define IP_TCP 6
#define IP_UDP 17
#define IP_ARP 18
#define IP_ENCAP 41  // IPv6 encapsulation
#define IP_OSPF 89   // Open Shortest Path First
#define SCTP 132     // Stream Control Tramission

uint16_t i16(uint16_t val);

uint32_t i32(uint32_t val);

struct net_uint32_t {
    uint32_t network_raw;  // must be in big endian

    uint32_t get() {
        return i32(network_raw);
    }
    void set_raw(uint32_t value) {
        network_raw = value;
    }
    net_uint32_t& operator=(uint32_t value) {
        network_raw = i32(value);
        return *this;
    }
};
struct net_uint16_t {
    uint16_t network_raw;  // must be in big endian

    uint16_t get() {
        return i16(network_raw);
    }
    void set_raw(uint16_t value) {
        network_raw = value;
    }
    net_uint16_t& operator=(uint16_t value) {
        network_raw = i16(value);
        return *this;
    }
};

typedef struct {
    net_uint16_t src_port;
    net_uint16_t dst_port;
    net_uint32_t sequence_number;
    net_uint32_t ack_number;

    union flags_u {
        net_uint16_t value;

        uint16_t get_flags() {
            return value.get() & 0x1FF;
        }

        void set_flags(uint16_t flags) {
            uint16_t temp = value.get();
            temp &= ~0x1FF;
            temp |= flags;
            value = temp;
        }

        uint16_t get_data_offset() {
            return (value.get() & 0xF000) >> 12;
        }

        void set_data_offset(uint16_t offset) {
            value = ((value.get() & ~0xFE00) | (offset << 12));
        }

        // uint8_t raw;
    } data_offset_reserved_flags;
    net_uint16_t window;
    net_uint16_t checksum;
    net_uint16_t urgent_pointer;
} __attribute__((packed)) tcp_header;

typedef struct {
    net_uint32_t src_ip;
    net_uint32_t dst_ip;
    uint8_t zero;
    uint8_t protocol;
    net_uint16_t length;
} __attribute__((packed, aligned(4))) pseduo_header;

typedef struct {
    net_uint16_t src_port;
    net_uint16_t dst_port;
    net_uint16_t total_length;
    net_uint16_t checksum;
} __attribute__((packed)) udp_header;

typedef struct {
    net_uint16_t hardware_type;
    net_uint16_t protocol_type;
    uint8_t hardware_size;
    uint8_t protocol_size;
    net_uint16_t opcode;
    uint8_t sender_mac[6];
    net_uint32_t sender_ip;
    uint8_t target_mac[6];
    net_uint32_t target_ip;
} __attribute__((packed)) arp_packet;

typedef struct {
    uint8_t type;
    uint8_t code;
    net_uint16_t checksum;
    union remainder_t {
        struct echo_t {
            net_uint16_t identifier;
            net_uint16_t seq_number;
        } __attribute__((packed)) echo;
    } __attribute__((packed)) remainder;
} __attribute__((packed)) icmp_header;

typedef struct {
    uint8_t ihl : 4;
    uint8_t version : 4;

    uint8_t type_of_service;
    net_uint16_t total_length;

    net_uint16_t identification;

    net_uint16_t _flags_fragments;
    uint8_t get_flags() {
        return _flags_fragments.get() >> 13;
    }
    uint16_t get_fragment_offset() {
        return _flags_fragments.get() & 0x1FFF;
    }
    void set_flags(uint8_t flags) {
        _flags_fragments = (flags << 13) | get_fragment_offset();
    }
    void set_fragment_offset(uint16_t offset) {
        _flags_fragments = ((get_flags() << 13) | offset);
    }

    uint8_t ttl;
    uint8_t protocol;
    net_uint16_t header_check_sum;

    net_uint32_t src_address;
    net_uint32_t dst_address;

} __attribute__((packed)) ipv4_header;

typedef struct {
    uint8_t dst_mac[6];
    uint8_t src_mac[6];
    net_uint16_t ethertype;
} __attribute__((packed)) ethernet_header;

typedef union {
    struct tcp_fields {
        uint32_t ack_number;
        uint32_t seq_number;
    } __attribute__((packed)) tcp;
} __attribute__((packed)) port_status;

typedef struct {
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    net_uint32_t xid;
    net_uint16_t secs;
    net_uint16_t flags;
    net_uint32_t ciaddr;
    net_uint32_t yiaddr;
    net_uint32_t siaddr;
    net_uint32_t giaddr;
    uint8_t chaddr[16];
    char sname[64];
    char file[128];
    uint8_t options[312];
} __attribute__((packed)) dhcp_packet;

template <typename T>
class PacketBuilder {
   public:
    PacketBuilder() : internal(new T()), encapsulated(nullptr) {
        length = sizeof(T);
    };

    ~PacketBuilder() {
        delete internal;
    }

    template <typename J>
    PacketBuilder& encapsulate(const PacketBuilder<J>& other) {
        this->encapsulated =
            const_cast<PacketBuilder<void>*>(reinterpret_cast<const PacketBuilder<void>*>(&other));
        length = sizeof(T) + (this->encapsulated ? this->encapsulated->get_size() : 0);
        return *this;
    }

    virtual void build_at(void*) = 0;
    size_t get_size() {
        return this->length;
    }

    T* build(size_t* len_store = nullptr) {
        // TODO: allow for passing in of a buffer to build the packet to?
        if (len_store) *len_store = this->get_size();
        void* packet = kmalloc(this->get_size());
        build_at(packet);
        return (T*)packet;
    };
    void send();

   protected:
    size_t length;
    T* internal;
    PacketBuilder<void>* encapsulated;
};

class PayloadBuilder : public PacketBuilder<uint8_t> {
   public:
    PayloadBuilder(uint8_t* buffer, size_t length) : buffer(buffer) {
        this->length = length;
    };

    size_t get_size() {
        return length;
    }

    const uint8_t* get_buffer() {
        return const_cast<const uint8_t*>(this->buffer);
    }

   protected:
    void build_at(void* addr) {
        if (buffer != nullptr && length > 0) memcpy(addr, buffer, length);
    }

   private:
    uint8_t* buffer;
};

class TCPBuilder : public PacketBuilder<tcp_header> {
   public:
    TCPBuilder(uint16_t src_port, uint16_t dst_port) {
        this->internal->src_port = src_port;
        this->internal->dst_port = dst_port;
    };

    TCPBuilder& with_seq_number(uint32_t seq_number) {
        this->internal->sequence_number = seq_number;
        return *this;
    }
    TCPBuilder& with_ack_number(uint32_t ack_number) {
        this->internal->ack_number = ack_number;
        return *this;
    }
    TCPBuilder& with_flags(uint8_t flags) {
        this->internal->data_offset_reserved_flags.set_flags(flags);
        return *this;
    }
    TCPBuilder& with_window_size(uint16_t window_size) {
        this->internal->window = window_size;
        return *this;
    }
    TCPBuilder& with_urgent_pointer(uint16_t urgent_pointer) {
        this->internal->urgent_pointer = urgent_pointer;
        return *this;
    }

    TCPBuilder& with_data_offset(uint8_t data_offset) {
        this->internal->data_offset_reserved_flags.set_data_offset(data_offset & 0xf);
        return *this;
    }

    TCPBuilder& with_pseduo_header(uint32_t src, uint32_t dst) {
        pseudo.src_ip = src;
        pseudo.dst_ip = dst;
        return *this;
    }

   protected:
    pseduo_header pseudo;
    void build_at(void*);
};

class ETHFrameBuilder : public PacketBuilder<ethernet_header> {
   public:
    ETHFrameBuilder(uint8_t src_mac[6], uint8_t dst_mac[6], uint16_t ethertype = 0x0800) {
        memcpy(this->internal->src_mac, src_mac, 6);
        memcpy(this->internal->dst_mac, dst_mac, 6);
        this->internal->ethertype = ethertype;
    };

   protected:
    void build_at(void*);
};

class UDPBuilder : public PacketBuilder<udp_header> {
   public:
    UDPBuilder(uint16_t src_port, uint16_t dst_port) {
        this->internal->src_port = src_port;
        this->internal->dst_port = dst_port;
    };

    UDPBuilder& with_pseduo_header(uint32_t src, uint32_t dst) {
        pseudo.src_ip = src;
        pseudo.dst_ip = dst;
        return *this;
    }

   protected:
    pseduo_header pseudo;
    void build_at(void*);
};

class ICMPBuilder : public PacketBuilder<icmp_header> {
   public:
    ICMPBuilder(uint8_t type, uint8_t code) {
        this->internal->type = type;
        this->internal->code = code;
    }

    ICMPBuilder& with_remainder(const icmp_header::remainder_t&& header) {
        this->internal->remainder = std::move(header);
        return *this;
    }

   protected:
    void build_at(void*);
};

class IPv4Builder : public PacketBuilder<ipv4_header> {
   public:
    IPv4Builder() {
        internal->version = 4;
        internal->ihl = 5;
        internal->type_of_service = 0;

        internal->identification = 0x69;
        internal->set_flags(0b000);
        internal->set_fragment_offset(0);

        internal->ttl = 64;
        internal->protocol = IP_TCP;
        internal->header_check_sum = 0;
    }

    IPv4Builder& with_src_address(uint32_t);
    IPv4Builder& with_dst_address(uint32_t);
    IPv4Builder& with_protocol(uint8_t);

   protected:
    void build_at(void*);
};

template <typename T>
class Packet {
   public:
    using HeaderType = T;

    virtual size_t get_next_offset(const uint8_t* data, size_t length) {
        return sizeof(HeaderType);
    }
};

class IPv4Packet : public Packet<ipv4_header> {
   public:
    size_t get_next_offset(const uint8_t* data, size_t length) override {
        return (reinterpret_cast<const ipv4_header*>(data)->ihl) * 4;
    }
};

class TCPPacket : public Packet<tcp_header> {
   public:
    size_t get_next_offset(const uint8_t* data, size_t length) override {
        return (reinterpret_cast<tcp_header*>(const_cast<uint8_t*>(data))
                    ->data_offset_reserved_flags.get_data_offset()) *
               4;
    }
};

typedef Packet<ethernet_header> EthernetFrame;
typedef Packet<arp_packet> ARPPacket;
typedef Packet<udp_header> UDPDatagram;
typedef Packet<icmp_header> ICMPPacket;
typedef Packet<dhcp_packet> DHCPPacket;
typedef Packet<uint8_t> Payload;

class PacketBufferParser {
   private:
    const uint8_t* packet_base;
    size_t length;
    size_t offset;

   public:
    PacketBufferParser(const uint8_t* packet, size_t length)
        : packet_base(packet), length(length), offset(0) {
    }

    template <typename PacketType>
    auto peek() {
        using HeaderType = typename PacketType::HeaderType;
        return const_cast<HeaderType*>(reinterpret_cast<const HeaderType*>(packet_base + offset));
    }

    template <typename PacketType>
    auto pop() {
        PacketType handler;

        using HeaderType = typename PacketType::HeaderType;
        auto ret_val =
            const_cast<HeaderType*>(reinterpret_cast<const HeaderType*>(packet_base + offset));
        offset += handler.get_next_offset(packet_base + offset, length - offset);

        return ret_val;
    }

    uint8_t* get_packet_base() {
        return const_cast<uint8_t*>(packet_base);
    }
    size_t get_length() {
        return length;
    }
};

template <typename... PacketChain>
class PacketParser {
   private:
    const uint8_t* packet_base;
    size_t offsets[sizeof...(PacketChain)];
    size_t length;

    template <size_t index>
    void initialize(size_t offset) {
    }

    template <size_t Index = 0, typename Current, typename... Rest>
    void initialize(size_t offset) {
        if (offset >= length) return;

        Current handler;

        offsets[Index] = offset;
        offset += handler.get_next_offset(packet_base + offset, length - offset);

        initialize<Index + 1, Rest...>(offset);
    }

   public:
    PacketParser(const uint8_t* packet, size_t length) : packet_base(packet), length(length) {
        this->initialize<0, PacketChain...>(0);
    }

    template <typename PacketType>
    auto get() {
        size_t index = tuple::index_of<PacketType, PacketChain...>();
        using HeaderType = typename PacketType::HeaderType;
        return const_cast<HeaderType*>(
            reinterpret_cast<const HeaderType*>(packet_base + offsets[index]));
    }
};

HashMap<uint64_t, void*>& get_ports();

port_status* obtain_port(uint16_t port);
void release_port(uint16_t port);

port_status* get_port_status(uint16_t port);
uint16_t calc_checksum(void*, void*, size_t, size_t);

#endif