#ifndef NETSTACK_H
#define NETSTACK_H

#include <stdint.h>
#include <stdbool.h>

// Byte order conversion macros

// Network configuration
typedef struct {
    uint32_t ip_address;
    uint32_t subnet_mask;
    uint32_t gateway;
    uint8_t mac_address[6];
} network_config_t;

extern network_config_t net_config;

// ARP Cache
#define ARP_CACHE_SIZE 16

typedef struct {
    uint32_t ip;
    uint8_t mac[6];
    uint32_t timestamp;
    bool valid;
} arp_cache_entry_t;

extern arp_cache_entry_t arp_cache[ARP_CACHE_SIZE];

// Ethernet Frame
#define ETHERTYPE_IPV4 0x0800
#define ETHERTYPE_ARP  0x0806

typedef struct __attribute__((packed)) {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
    uint8_t payload[1500];
} ethernet_frame_t;

// ARP Packet
#define ARP_REQUEST 1
#define ARP_REPLY   2

typedef struct __attribute__((packed)) {
    uint16_t hw_type;
    uint16_t protocol_type;
    uint8_t hw_addr_len;
    uint8_t protocol_addr_len;
    uint16_t operation;
    uint8_t sender_mac[6];
    uint32_t sender_ip;
    uint8_t target_mac[6];
    uint32_t target_ip;
} arp_packet_t;

// IPv4 Header
#define IP_PROTOCOL_ICMP 1
#define IP_PROTOCOL_TCP  6
#define IP_PROTOCOL_UDP  17

typedef struct __attribute__((packed)) {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t total_length;
    uint16_t id;
    uint16_t flags_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
} ipv4_header_t;

// ICMP Header
#define ICMP_ECHO_REPLY   0
#define ICMP_ECHO_REQUEST 8

typedef struct __attribute__((packed)) {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t sequence;
} icmp_header_t;

// UDP Header
typedef struct __attribute__((packed)) {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} udp_header_t;

// Network stack functions
void netstack_init(uint32_t ip, uint32_t subnet, uint32_t gateway);
void netstack_set_mac(uint8_t* mac);
void netstack_process_packet(uint8_t* data, uint16_t length);
void netstack_process_pending(void);
void netstack_queue_packet(uint32_t dest_ip, uint8_t protocol, uint8_t* data, uint16_t length);

// Ethernet layer
void ethernet_send(uint8_t* dest_mac, uint16_t ethertype, uint8_t* payload, uint16_t length);
void ethernet_process(uint8_t* frame, uint16_t length);

// ARP layer
void arp_send_request(uint32_t target_ip);
void arp_send_reply(uint32_t target_ip, uint8_t* target_mac);
void arp_process(arp_packet_t* arp);
bool arp_lookup(uint32_t ip, uint8_t* mac_out);
void arp_cache_add(uint32_t ip, uint8_t* mac);
void arp_cache_dump(void);

// IPv4 layer
void ipv4_send(uint32_t dest_ip, uint8_t protocol, uint8_t* payload, uint16_t length);
void ipv4_process(ipv4_header_t* ip_header, uint16_t length);
uint16_t ipv4_checksum(uint16_t* data, int length);
uint32_t ntohl(uint32_t n);
// ICMP layer
void icmp_send_echo_request(uint32_t dest_ip, uint16_t id, uint16_t seq);
void icmp_send_echo_reply(uint32_t dest_ip, uint16_t id, uint16_t seq, uint8_t* data, uint16_t data_len);
void icmp_process(ipv4_header_t* ip_header, icmp_header_t* icmp, uint16_t length);

// UDP layer
void udp_send(uint32_t dest_ip, uint16_t src_port, uint16_t dest_port, uint8_t* data, uint16_t length);
void udp_process(ipv4_header_t* ip_header, udp_header_t* udp, uint16_t length);

// Utility functions
uint32_t ip_parse(const char* ip_str);
void ip_to_string(uint32_t ip, char* buffer);
void mac_to_string(uint8_t* mac, char* buffer);

#endif // NETSTACK_H