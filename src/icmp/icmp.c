#include "../rtl8139/rtl8139.h"
#include "../utility/utility.h"
#include "../arp/arp.h"
#include <stdint.h>

#define ETH_HEADER_SIZE 14
#define IPV4_HEADER_SIZE 20

// Ethernet header
typedef struct {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
} __attribute__((packed)) eth_header_t;

// IPv4 header (minimal, no options)
typedef struct {
    uint8_t  version_ihl;
    uint8_t  tos;
    uint16_t total_length;
    uint16_t id;
    uint16_t flags_fragment_offset;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t header_checksum;
    uint8_t  src_ip[4];
    uint8_t  dst_ip[4];
} __attribute__((packed)) ipv4_header_t;

// Helper: calculate IPv4 checksum (same algorithm as ICMP checksum)
static uint16_t ipv4_checksum(const void *data, int length) {
    uint32_t sum = 0;
    const uint16_t *ptr = (const uint16_t *)data;

    while (length > 1) {
        sum += *ptr++;
        length -= 2;
    }

    if (length == 1) {
        uint16_t last_byte = 0;
        *((uint8_t *)&last_byte) = *(const uint8_t *)ptr;
        sum += last_byte;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)(~sum);
}

uint16_t icmp_checksum(const void *data, int length) {
    uint32_t sum = 0;
    const uint16_t *ptr = (const uint16_t *)data;
    // Sum all 16-bit words
    while (length > 1) {
        sum += *ptr++;
        length -= 2;
    }
    // If there's a leftover byte, pad with zero to make 16 bits
    if (length == 1) {
        uint16_t last_byte = 0;
        *((uint8_t *)&last_byte) = *(const uint8_t *)ptr;
        sum += last_byte;
    }
    // Fold 32-bit sum to 16 bits: add carries
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    // One's complement and return
    return (uint16_t)(~sum);
}

// Send ICMP packet over RTL8139 by building Ethernet + IPv4 + ICMP packet
bool icmp_send_packet_via_rtl8139(const uint8_t *src_ip, const uint8_t *dst_ip, const arp_packet_t *icmp_pkt, int icmp_len) {
    if (!RTL8139 || !RTL8139->initialized) {
        return false;
    }

    uint8_t dest_mac[6];
    if (!arp_resolve(dst_ip, dest_mac)) {
        // ARP resolution failed, cannot send
        return false;
    }

    uint8_t packet[ETH_HEADER_SIZE + IPV4_HEADER_SIZE + 1500]; // max Ethernet MTU
    if (icmp_len > 1500 - IPV4_HEADER_SIZE) {
        // Too large
        return false;
    }

    // Build Ethernet header
    eth_header_t *eth = (eth_header_t *)packet;
    memcpy(eth->dest_mac, dest_mac, 6);
    memcpy(eth->src_mac, RTL8139->mac_address, 6);
    eth->ethertype = htons(0x0800); // IPv4

    // Build IPv4 header
    ipv4_header_t *ip = (ipv4_header_t *)(packet + ETH_HEADER_SIZE);
    ip->version_ihl = (4 << 4) | (IPV4_HEADER_SIZE / 4);
    ip->tos = 0;
    ip->total_length = htons(IPV4_HEADER_SIZE + icmp_len);
    ip->id = htons(0); // can be incremented per packet
    ip->flags_fragment_offset = htons(0);
    ip->ttl = 64;
    ip->protocol = 1; // ICMP
    ip->header_checksum = 0;
    memcpy(ip->src_ip, src_ip, 4);
    memcpy(ip->dst_ip, dst_ip, 4);
    ip->header_checksum = ipv4_checksum(ip, IPV4_HEADER_SIZE);

    // Copy ICMP payload after IPv4 header
    memcpy(packet + ETH_HEADER_SIZE + IPV4_HEADER_SIZE, icmp_pkt, icmp_len);

    // Calculate ICMP checksum (should already be done, but double-check)
    // You can skip if you trust caller did it

    // Send full Ethernet frame
    return rtl8139_send_packet((int8_t *)packet, ETH_HEADER_SIZE + IPV4_HEADER_SIZE + icmp_len);
}
