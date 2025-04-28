#ifndef _SOCKET_H
#define _SOCKET_H

#include <stdint.h>

#include "dhcp.h"
#include "function.h"
#include "hash.h"
#include "net_stack.h"
#include "network_card.h"
#include "printf.h"
#include "tcp.h"

class ISocket;

HashMap<uint16_t, ISocket*>* get_open_sockets();

struct CountingSemaphore {
    SpinLock lock;
    uint32_t value;

    CountingSemaphore() : value(0) {
    }
    CountingSemaphore(uint32_t value) : value(value) {
    }

    void down() {
    loop:
        lock.lock();
        if (value > 0) {
            value -= 1;
            lock.unlock();
        } else {
            lock.unlock();
            goto loop;
        }
    }

    void up() {
        LockGuard<SpinLock> guard(lock);
        value++;
    }
};

class ISocket {
   protected:
    static const uint8_t QUEUE_MAX_SIZE = 4;

    CountingSemaphore num_empty;
    CountingSemaphore num_queue;

    uint8_t enqueue_idx;
    uint8_t dequeue_idx;

    uint8_t __attribute__((aligned(8))) recv_buffer[1516 * QUEUE_MAX_SIZE];
    uint8_t __attribute__((aligned(8))) send_buffer[1516];

    pseduo_header pseudo;

    Function<void(size_t)> recv_callback;
    bool configured = false;

    size_t lock_if_present(uint8_t* buffer);
    size_t handle_tcp(uint8_t* slot, uint8_t* buffer);
    size_t handle_udp(uint8_t* slot, uint8_t* buffer);

    void configure(PacketParser<EthernetFrame, IPv4Packet, TCPPacket, Payload>* other);
    bool is_configured() {
        return this->configured;
    }

   public:
    uint8_t protocol;
    port_status status;

    ISocket() {
        K::memset(&status, 0, sizeof(status));
        num_queue = CountingSemaphore(0);
        num_empty = CountingSemaphore(QUEUE_MAX_SIZE);
        status.tcp.flags.alive = true;
        enqueue_idx = 0;
        dequeue_idx = 0;

        printf("recv_buffer = 0x%x, send_buffer = 0x%x\n", recv_buffer, send_buffer);
    }

    virtual void close() = 0;

    void enqueue(uint8_t* buffer, size_t length);
    size_t dequeue(uint8_t* buffer);

    void send(uint8_t* buffer, size_t length, uint16_t response_flags = TCP_FLAG_ACK);
    size_t recv(uint8_t* buffer);
};

class ClientSocket {
   private:
    uint32_t dst_ip;
    uint16_t dst_port;

   public:
    ClientSocket(uint32_t dst_ip, uint16_t dst_port) : dst_ip(dst_ip), dst_port(dst_port) {
    }
};

class UDPSocket : public ISocket {
   private:
    uint32_t dst_ip;
    uint16_t dst_port;
    uint16_t src_port;

    void _initialize();

   public:
    UDPSocket(uint32_t dst_ip, uint16_t dst_port) : dst_ip(dst_ip), dst_port(dst_port) {
        protocol = IP_UDP;
        K::assert(obtain_port(100), "port already in use");
        src_port = 100;
        get_open_sockets()->put(src_port, this);
        _initialize();
        printf("UDPSocket created on port %d\n", src_port);
    }

    // ISocket recv works, but wanted to do a simple one for udp.
    void send_udp(const uint8_t* buffer, size_t length);

    void close() {
        printf("Closing UDP socket\n");
    }
};

class ServerSocket : public ISocket {
   private:
    uint16_t port;

    size_t send_size;

    void _initialize();

   public:
    ServerSocket(uint16_t port) : port(port) {
        K::assert(obtain_port(port), "port already in use");
        get_open_sockets()->put(port, this);
        _initialize();
        protocol = IP_TCP;
    }

    ~ServerSocket() {
    }

    uint8_t* get_recv_buffer() {
        return recv_buffer;
    }

    void on_recv(Function<void(size_t)>&& callback) {
        this->recv_callback = std::move(callback);
    }

    bool is_alive() {
        return status.tcp.flags.alive;
    }

    void close() {
        status.tcp.flags.alive = false;
        get_open_sockets()->remove(port);
        release_port(port);
        printf("closed called!\n");
    }

    void bind(uint16_t port) {
        this->port = port;
        get_open_sockets()->put(port, this);
    }

    // Send step 2 of 3.
    void listen() {
        // todo.
    }

    // Receive step 3 of 3.
    // ISocket* accept() {
    //     int assigned_port = assign_port();
    //     if (assigned_port == -1) {
    //         return nullptr;
    //     }
    //     finish later.
    // }
};

#endif