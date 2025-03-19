#ifndef _IP_H
#define _IP_H

#include "heap.h"
#include "printf.h"
#include "stdint.h"

#define IP_ICMP 1  // Internet Control Message Protocol
#define IP_IGMP 2  // Internet Group Management
#define IP_TCP 6
#define IP_UDP 17
#define IP_ENCAP 41  // IPv6 encapsulation
#define IP_OSPF 89   // Open Shortest Path First
#define SCTP 132     // Stream Control Tramission

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) udp_header;

typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint32_t remainder;
} __attribute__((packed)) icpm_header;

typedef struct {
#ifdef LITTLE_ENDIAN
    uint8_t ihl : 4;  // because we are in a little endian machine, this matters
    uint8_t version : 4;
#else
    uint8_t version : 4;
    uint8_t ihl : 4;
#endif

    uint8_t type_of_service;
    uint16_t total_length;

    uint16_t identification;

#ifdef LITTLE_ENDIAN
    uint16_t fragment_offset : 13;
    uint8_t flags : 3;
#else
    uint8_t flags : 3;
    uint16_t fragment_offset : 13;
#endif
    uint8_t ttl;
    uint8_t protocol;
    uint16_t header_check_sum;

    uint32_t src_address;
    uint32_t dst_address;

} __attribute__((packed)) ipv4_packet_header;

typedef struct {
    uint8_t dst_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;  // 0x0800 for IPv4
} __attribute__((packed)) ethernet_frame_header;

typedef struct {
    ethernet_frame_header header;
    void* data;
} __attribute__((packed)) ethernet_frame;

typedef struct {
    ipv4_packet_header header;
    void* data;
} __attribute__((packed)) ipv4_packet;
typedef struct {
    udp_header header;
    void* data;
} __attribute__((packed)) udp_datagram;

template <typename T>
class PacketBuilder {
   public:
    PacketBuilder() : internal(new T()), encapsulated(nullptr), built_packet(nullptr) {};
    ~PacketBuilder() {
        if (built_packet) delete built_packet;
    }

    template <typename J>
    PacketBuilder& encapsulate(PacketBuilder<J>* other) {
        this->encapsulated = reinterpret_cast<PacketBuilder<void>*>(other);
        return *this;
    }
    virtual T* build_at(void*) = 0;
    virtual size_t get_size() = 0;
    T* build() {
        if (built_packet != nullptr) {
            return built_packet;
        }
        build_at(0x0);
        return this->built_packet;
    };
    void send();

   protected:
    T* internal;
    T* built_packet;
    PacketBuilder<void>* encapsulated;
};

template <typename T>
class PacketBuilderWithData : public PacketBuilder<T> {
   public:
    PacketBuilderWithData& with_data(void* data, size_t length) {
        this->internal->data = data;
        this->data_length = length;
        return *this;
    }

   protected:
    size_t data_length = 0;
};

class ETHFrameBuilder : public PacketBuilderWithData<ethernet_frame> {
   public:
    ETHFrameBuilder(uint8_t src_mac[6], uint8_t dst_mac[6]) {
        for (int i = 0; i < 6; i++) {
            this->internal->header.src_mac[i] = src_mac[i];
            this->internal->header.dst_mac[i] = dst_mac[i];
        }
    };

    size_t get_size() {
        return sizeof(ethernet_frame_header) +
               (this->encapsulated != nullptr ? this->encapsulated->get_size() : 0);
    }

   protected:
    ethernet_frame* build_at(void*);
};

class UDPBuilder : public PacketBuilderWithData<udp_datagram> {
   public:
    UDPBuilder(uint16_t src_port, uint16_t dst_port) {
        this->internal->header.src_port = src_port;
        this->internal->header.dst_port = dst_port;
    };

    size_t get_size() {
        return sizeof(udp_header) + (this->data_length) +
               (this->encapsulated != nullptr ? this->encapsulated->get_size() : 0);
    }

   protected:
    udp_datagram* build_at(void*);
};

class IPv4Builder : public PacketBuilderWithData<ipv4_packet> {
   public:
    IPv4Builder() {
        internal->header.version = 4;
        internal->header.ihl = 5;
        internal->header.type_of_service = 0;

        internal->header.identification = 0xcaee;
        internal->header.flags = 0b000;
        internal->header.fragment_offset = 0;

        internal->header.ttl = 1;
        internal->header.protocol = IP_UDP;
        internal->header.header_check_sum = 0;
    }

    size_t get_size() {
        return sizeof(udp_header) + (this->data_length) +
               (this->encapsulated != nullptr ? this->encapsulated->get_size() : 0);
    }

    IPv4Builder& with_src_address(uint32_t);
    IPv4Builder& with_dst_address(uint32_t);
    IPv4Builder& with_protocol(uint8_t);

   protected:
    ipv4_packet* build_at(void*);
};

#endif