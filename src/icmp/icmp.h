#ifndef ICMP_H
#define ICMP_H

#include <stdint.h>
#include <stdbool.h>

#define ICMP_TYPE_ECHO_REPLY 0
#define ICMP_TYPE_ECHO_REQUEST 8
#define ICMP_TYPE_DEST_UNREACHABLE 3

#pragma pack(push, 1)
typedef struct {
    uint16_t id;
    uint16_t sequence;
} icmp_echo_t;

typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    union {
        icmp_echo_t echo;
        struct {
            uint16_t unused;
            uint16_t next_mtu;
            uint8_t original_ip_header[20];
            uint8_t original_payload[8];
        } dest_unreachable;
        // Removed flexible array member here
    };
    // Data payload follows immediately after this struct in memory
} icmp_packet_t;
#pragma pack(pop)

// Calculate ICMP checksum over data of given length
uint16_t icmp_checksum(const void *data, int length);

// Handle incoming ICMP packet
// src_ip and dst_ip are IPv4 addresses of sender and receiver (4 bytes each)
void icmp_handle_packet(const uint8_t *packet, int length, const uint8_t *src_ip, const uint8_t *dst_ip);

// Create and send ICMP Echo Request
// id and sequence are identifiers for matching replies
// data and data_len are optional payload
void icmp_send_echo_request(const uint8_t *src_ip, const uint8_t *dst_ip,
                            uint16_t id, uint16_t sequence,
                            const uint8_t *data, int data_len);

// Create and send ICMP Echo Reply in response to a request
void icmp_send_echo_reply(const uint8_t *src_ip, const uint8_t *dst_ip,
                         uint16_t id, uint16_t sequence,
                         const uint8_t *data, int data_len);

// Build and send full Ethernet + IPv4 + ICMP packet via RTL8139
// Returns true on success, false on failure (e.g., ARP resolution failure)
bool icmp_send_packet_via_rtl8139(const uint8_t *src_ip, const uint8_t *dst_ip,
                                  const icmp_packet_t *icmp_pkt, int icmp_len);

#endif // ICMP_H
