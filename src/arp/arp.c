#include "arp.h"
#include <stdbool.h>
#include "../io/io.h"       // For inb/outb if needed
#include "../utility/utility.h" // For memcpy, memset, etc.
#include "../rtl8139/rtl8139.h"
#include "../timers/timer.h"
// Ethernet frame header size
#define ETH_HEADER_SIZE 14

// Ethernet type for ARP
#define ETH_TYPE_ARP 0x0806

// Broadcast MAC address
static const uint8_t broadcast_mac[ARP_HLEN_ETHERNET] = {0xff,0xff,0xff,0xff,0xff,0xff};

// ARP cache
static arp_cache_entry_t arp_cache[ARP_CACHE_SIZE];

// Our MAC and IP addresses (set at init)
static uint8_t my_mac[ARP_HLEN_ETHERNET];
static uint8_t my_ip[ARP_PLEN_IPV4];

// Helper: convert 16-bit to big endian
static uint16_t to_be16(uint16_t val) {
    return (val >> 8) | (val << 8);
}

// Helper: convert 16-bit from big endian
static uint16_t from_be16(uint16_t val) {
    return (val >> 8) | (val << 8);
}

// Initialize ARP module and cache
void arp_init(const uint8_t *mac, const uint8_t *ip) {
    memcpy(my_mac, mac, ARP_HLEN_ETHERNET);
    memcpy(my_ip, ip, ARP_PLEN_IPV4);
    memset(arp_cache, 0, sizeof(arp_cache));
}

// Update ARP cache entry or add new
void arp_cache_update(const uint8_t *ip, const uint8_t *mac) {
    // Check if entry exists, update if found
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && memcmp(arp_cache[i].ip, ip, ARP_PLEN_IPV4) == 0) {
            memcpy(arp_cache[i].mac, mac, ARP_HLEN_ETHERNET);
            return;
        }
    }
    // Add new entry in first invalid slot
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (!arp_cache[i].valid) {
            memcpy(arp_cache[i].ip, ip, ARP_PLEN_IPV4);
            memcpy(arp_cache[i].mac, mac, ARP_HLEN_ETHERNET);
            arp_cache[i].valid = true;
            return;
        }
    }
    // Cache full, overwrite oldest (simple strategy: overwrite index 0)
    memcpy(arp_cache[0].ip, ip, ARP_PLEN_IPV4);
    memcpy(arp_cache[0].mac, mac, ARP_HLEN_ETHERNET);
    arp_cache[0].valid = true;
}

// Lookup MAC by IP in ARP cache
bool arp_cache_lookup(const uint8_t *ip, uint8_t *mac_out) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && memcmp(arp_cache[i].ip, ip, ARP_PLEN_IPV4) == 0) {
            memcpy(mac_out, arp_cache[i].mac, ARP_HLEN_ETHERNET);
            return true;
        }
    }
    return false;
}

// Compose and send ARP packet (request or reply)
static void arp_send_packet(const arp_packet_t *arp_pkt) {
    // Ethernet frame buffer: Ethernet header + ARP packet
    uint8_t frame[ETH_HEADER_SIZE + sizeof(arp_packet_t)];

    // Ethernet header
    memcpy(frame, arp_pkt->dsthw, ARP_HLEN_ETHERNET);          // Destination MAC
    memcpy(frame + 6, arp_pkt->srchw, ARP_HLEN_ETHERNET);      // Source MAC
    frame[12] = (ETH_TYPE_ARP >> 8) & 0xFF;                    // Ethertype high byte
    frame[13] = ETH_TYPE_ARP & 0xFF;                           // Ethertype low byte

    // ARP packet
    memcpy(frame + ETH_HEADER_SIZE, arp_pkt, sizeof(arp_packet_t));

    // Send via RTL8139 driver
    rtl8139_send_packet((const int8_t*)frame, sizeof(frame));
}

// Send ARP request
void arp_send_request(const uint8_t *src_mac, const uint8_t *src_ip, const uint8_t *target_ip) {
    arp_packet_t packet;

    packet.htype = to_be16(ARP_HTYPE_ETHERNET);
    packet.ptype = to_be16(ARP_PTYPE_IPv4);
    packet.hlen = ARP_HLEN_ETHERNET;
    packet.plen = ARP_PLEN_IPV4;
    packet.opcode = to_be16(ARP_OP_REQUEST);

    memcpy(packet.srchw, src_mac, ARP_HLEN_ETHERNET);
    memcpy(packet.srcpr, src_ip, ARP_PLEN_IPV4);

    memset(packet.dsthw, 0, ARP_HLEN_ETHERNET);
    memcpy(packet.dstpr, target_ip, ARP_PLEN_IPV4);

    arp_send_packet(&packet);
}

// Send ARP reply
void arp_send_reply(const uint8_t *src_mac, const uint8_t *src_ip,
                    const uint8_t *target_mac, const uint8_t *target_ip) {
    arp_packet_t packet;

    packet.htype = to_be16(ARP_HTYPE_ETHERNET);
    packet.ptype = to_be16(ARP_PTYPE_IPv4);
    packet.hlen = ARP_HLEN_ETHERNET;
    packet.plen = ARP_PLEN_IPV4;
    packet.opcode = to_be16(ARP_OP_REPLY);

    memcpy(packet.srchw, src_mac, ARP_HLEN_ETHERNET);
    memcpy(packet.srcpr, src_ip, ARP_PLEN_IPV4);

    memcpy(packet.dsthw, target_mac, ARP_HLEN_ETHERNET);
    memcpy(packet.dstpr, target_ip, ARP_PLEN_IPV4);

    arp_send_packet(&packet);
}

// Handle incoming ARP packet
void arp_handle_packet(const uint8_t *packet_data, uint16_t length) {
    if (length < sizeof(arp_packet_t)) {
        return; // Too short
    }

    const arp_packet_t *packet = (const arp_packet_t *)packet_data;

    // Validate hardware and protocol types and lengths
    if (packet->htype != to_be16(ARP_HTYPE_ETHERNET) || packet->ptype != to_be16(ARP_PTYPE_IPv4)) {
        return;
    }
    if (packet->hlen != ARP_HLEN_ETHERNET || packet->plen != ARP_PLEN_IPV4) {
        return;
    }

    uint16_t opcode = from_be16(packet->opcode);

    // Update ARP cache with sender info
    arp_cache_update(packet->srcpr, packet->srchw);

    if (opcode == ARP_OP_REQUEST) {
        // If request is for us, send reply
        if (memcmp(packet->dstpr, my_ip, ARP_PLEN_IPV4) == 0) {
            arp_send_reply(my_mac, my_ip, packet->srchw, packet->srcpr);
        }
    } else if (opcode == ARP_OP_REPLY) {
        // ARP reply received, cache updated above
        // You can notify upper layers here if needed
    }
}

bool arp_resolve(const uint8_t *ip, uint8_t *mac_out) {
    if (!ip || !mac_out) {
        return false;
    }
    // Check if IP is already in cache
    if (arp_cache_lookup(ip, mac_out)) {
        return true;
    }
    // Not in cache, send ARP request
    arp_send_request(my_mac, my_ip, ip);
    // Wait and poll cache for reply
    for (int i = 0; i < ARP_RESOLVE_MAX_RETRIES; i++) {
        delay_ms(ARP_RESOLVE_RETRY_MS);
        if (arp_cache_lookup(ip, mac_out)) {
            return true;
        }
    }
    // Failed to resolve
    return false;
}