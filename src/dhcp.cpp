#include "dhcp.h"

#include "libk.h"
#include "network_card.h"
#include "printf.h"

static const uint32_t DHCP_XID = 0x55444455;
static const uint32_t DHCP_MAGIC_COOKIE = 0x63825363;
static const uint8_t DHCP_SERVER_ADDRESS_TYPE = 54;

dhcp_state_t dhcp_state;

uint32_t our_ip;

void print_ip(uint32_t ip) {
    printf("%d.%d.%d.%d\n", (ip >> 24), (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
}

bool dhci_is_valid_server_ip(uint32_t ip) {
    // TODO: make this better lol
    uint8_t a = (ip >> 24) & 0xFF;
    uint8_t b = (ip >> 16) & 0xFF;
    uint8_t c = (ip >> 8) & 0xFF;
    uint8_t d = ip & 0xFF;

    if (a == 0 || a >= 224) return false;
    if (d == 0 || d == 255) return false;
    if (a == 127) return false;
    if (a == 169 && b == 254) return false;
    if (a == 192 && b == 168) return false;
    if (a == 10) return false;
    if (a == 172 && (b >= 16 && b <= 31)) return false;

    return true;
}

template <size_t T>
void print_server_group(const char* name, server_group<T>* group) {
    printf("\t%s\n", name);
    for (uint8_t i = 0; i < group->count; i++) {
        printf("\t\t%d: ", i + 1);
        print_ip(group->ips[i]);
    }
    printf("\n");
}

void dhcp_print_state() {
    printf("\nDHCP state: \n");

    printf("\tSubnet Mask: ");
    print_ip(dhcp_state.subnet_mask);

    printf("\t(my) device ip: ");
    print_ip(dhcp_state.my_ip);

    printf("\tDHCP server ip: ");
    print_ip(dhcp_state.dhcp_server_ip);
    printf("\n");

    print_server_group("DNS Servers:", &dhcp_state.dns_servers);
    print_server_group("Time Servers:", &dhcp_state.time_servers);
}

void dhcp_handle_offer(PacketParser<EthernetFrame, IPv4Packet, UDPDatagram, DHCPPacket>* parser) {
    auto udp_packet = parser->get<UDPDatagram>();
    auto ipv4 = parser->get<IPv4Packet>();

    auto dhcp_pckt = parser->get<DHCPPacket>();

    if (dhcp_pckt->op != 2 || dhcp_pckt->htype != 1 || dhcp_pckt->hlen != 6 ||
        dhcp_pckt->xid.get() != DHCP_XID ||
        i32(*((uint32_t*)dhcp_pckt->options)) != DHCP_MAGIC_COOKIE) {
        printf("received invalid DHCP offer\n");
        return;
    }

    K::memset(&dhcp_state, 0, sizeof(dhcp_state));

    dhcp_state.dhcp_server_ip = dhcp_pckt->siaddr.get();
    dhcp_state.my_ip = dhcp_pckt->yiaddr.get();

    for (int i = 4; i < sizeof(dhcp_pckt->options); i++) {
        uint8_t option = dhcp_pckt->options[i];
        if (option == 0xFF) {
            // reached "EOF"
            break;
        }
        switch (option) {
            case DHCP_OPTION_SUBNET_MASK: {
                dhcp_state.subnet_mask = i32(*((uint32_t*)(dhcp_pckt->options + i + 2)));
                break;
            }

            case DHCP_OPTION_TIME_SERVER: {
                uint8_t count = dhcp_pckt->options[i + 1] >> 2;
                dhcp_state.time_servers.count = 0;
                for (int j = 0; j < count && dhcp_state.time_servers.count < DHCP_MAX_TIME_SERVERS;
                     j++) {
                    uint32_t dns_ip = i32(*((uint32_t*)(dhcp_pckt->options + i + 2 + (j * 4))));
                    if (!dhci_is_valid_server_ip(dns_ip)) {
                        continue;
                    }
                    dhcp_state.time_servers.ips[dhcp_state.time_servers.count] = dns_ip;
                    dhcp_state.time_servers.count += 1;
                }
                break;
            }
            case DHCP_OPTION_DNS_SERVER: {
                uint8_t count = dhcp_pckt->options[i + 1] >> 2;
                dhcp_state.dns_servers.count = 0;
                for (int j = 0; j < count && dhcp_state.dns_servers.count < DHCP_MAX_DNS_SERVERS;
                     j++) {
                    uint32_t dns_ip = i32(*((uint32_t*)(dhcp_pckt->options + i + 2 + (j * 4))));
                    if (!dhci_is_valid_server_ip(dns_ip)) {
                        continue;
                    }
                    dhcp_state.dns_servers.ips[dhcp_state.dns_servers.count] = dns_ip;
                    dhcp_state.dns_servers.count += 1;
                }
                break;
            }
            case DHCP_OPTION_MESSAGE_TYPE: { 
                uint8_t message_type = dhcp_pckt->options[i + 1];
                printf("DHCP Message Type: %d\n", message_type);
                break;
            }
            case DHCP_OPTION_SERVER_ID: {
                uint32_t server_id = i32(*((uint32_t*)(dhcp_pckt->options + i + 2)));
                print_ip(server_id);
                break;
            }
            default: {
                break;
            }
        }
    }

    dhcp_print_state();
}

void dhcp_discover(usb_session* session) {
    dhcp_packet dhcp_packet;
    K::memset(&dhcp_packet, 0, sizeof(dhcp_packet));
    dhcp_packet.op = 1;
    dhcp_packet.htype = 1;
    dhcp_packet.hlen = 6;
    dhcp_packet.xid = DHCP_XID;  // just needs to be random
    K::memcpy(dhcp_packet.chaddr, get_mac_address(), 6);

    memcpy(dhcp_packet.options, &DHCP_MAGIC_COOKIE, 4);
    dhcp_packet.options[4] = DHCP_OPTION_MESSAGE_TYPE;
    dhcp_packet.options[5] = 1;
    dhcp_packet.options[6] = 55;
    dhcp_packet.options[7] = 4;
    dhcp_packet.options[8] = 1;
    dhcp_packet.options[9] = 3;
    dhcp_packet.options[10] = 6;
    dhcp_packet.options[11] = 12;
    dhcp_packet.options[12] = 255;

    size_t packet_len = sizeof(dhcp_packet) - sizeof(dhcp_packet.options) + 13;

    size_t len;
    ethernet_header* frame;
    frame =
        ETHFrameBuilder{get_mac_address(), (uint8_t[6]){0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, 0x0800}
            .encapsulate(
                &IPv4Builder()
                     .with_src_address(0x00000000)
                     .with_dst_address(0xFFFFFFFF)
                     .with_protocol(IP_UDP)
                     .encapsulate(
                         &UDPBuilder{68, 67}
                              .with_pseduo_header(0x00000000, 0xFFFFFFFF)
                              .encapsulate(
                                  PacketBufferBuilder{(uint8_t*)&dhcp_packet, packet_len}.ptr())))
            .build(&len);

    send_packet(session, (uint8_t*)frame, len);

    delete frame;
}