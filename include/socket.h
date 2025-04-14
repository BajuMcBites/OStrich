#ifndef _SOCKET_H
#define _SOCKET_H

#include <stdint.h>

#include "dhcp.h"
#include "function.h"
#include "hash.h"
#include "net_stack.h"
#include "network_card.h"
#include "printf.h"

class Socket;

HashMap<uint16_t, Socket*>& get_open_sockets();

class Socket {
   private:
    uint32_t destination;
    uint16_t port;

    size_t send_size;
    uint8_t send_buffer[1514];
    uint8_t recv_buffer[1514];

    Function<void(size_t)> recv_callback;
    bool configured = false;
    port_status* status;

    PacketParser<EthernetFrame, IPv4Packet, TCPPacket, Payload>* parser;

    void _initialize();

   public:
    Socket(uint16_t port) : port(port) {
        K::assert((status = obtain_port(port)) != nullptr, "port already in use");
        get_open_sockets().put(port, this);

        _initialize();
    }

    ~Socket() {
        if (is_alive()) close();
        delete parser;
    }

    uint8_t* get_recv_buffer() {
        return recv_buffer;
    }

    void on_recv(Function<void(size_t)>&& callback) {
        this->recv_callback = std::move(callback);
    }

    bool is_alive() {
        return status->tcp.alive;
    }

    void close() {
        status->tcp.alive = false;
        get_open_sockets().remove(port);
        release_port(port);
    }
    
    void send(uint8_t* buffer, size_t length);

    void recv(ethernet_header* frame, ipv4_header* ipv4, tcp_header* tcp, size_t length);
};

#endif