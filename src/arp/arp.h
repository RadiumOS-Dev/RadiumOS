#ifndef ARP_H
#define ARP_H

#include <stdint.h>
#include <stdbool.h>

#define ARP_HTYPE_ETHERNET 0x0001
#define ARP_PTYPE_IPv4 0x0800
#define ARP_HLEN_ETHERNET 6
#define ARP_PLEN_IPV4 4

#define ARP_OP_REQUEST 0x0001
#define ARP_OP_REPLY 0x0002

#pragma pack(push, 1)
typedef struct arp_packet {
    uint16_t htype;               // Hardware type
    uint16_t ptype;               // Protocol type
    uint8_t hlen;                 // Hardware address length
    uint8_t plen;                 // Protocol address length
    uint16_t opcode;              // Operation code
    uint8_t srchw[ARP_HLEN_ETHERNET];  // Source hardware address (MAC)
    uint8_t srcpr[ARP_PLEN_IPV4];      // Source protocol address (IPv4)
    uint8_t dsthw[ARP_HLEN_ETHERNET];  // Destination hardware address (MAC)
    uint8_t dstpr[ARP_PLEN_IPV4];      // Destination protocol address (IPv4)
} arp_packet_t;
#pragma pack(pop)
#define ARP_RESOLVE_TIMEOUT_MS 1000
#define ARP_RESOLVE_RETRY_MS 100
#define ARP_RESOLVE_MAX_RETRIES (ARP_RESOLVE_TIMEOUT_MS / ARP_RESOLVE_RETRY_MS)
// ARP cache entry
typedef struct arp_cache_entry {
    uint8_t ip[ARP_PLEN_IPV4];
    uint8_t mac[ARP_HLEN_ETHERNET];
    bool valid;
} arp_cache_entry_t;

#define ARP_CACHE_SIZE 16

// Initialize ARP module and cache
void arp_init(const uint8_t *my_mac, const uint8_t *my_ip);

// Send ARP request
void arp_send_request(const uint8_t *src_mac, const uint8_t *src_ip, const uint8_t *target_ip);

// Send ARP reply
void arp_send_reply(const uint8_t *src_mac, const uint8_t *src_ip,
                    const uint8_t *target_mac, const uint8_t *target_ip);

// Handle incoming ARP packet
void arp_handle_packet(const uint8_t *packet_data, uint16_t length);

// Lookup MAC address in ARP cache by IP, returns true if found
bool arp_cache_lookup(const uint8_t *ip, uint8_t *mac_out);

// Update ARP cache with IP-MAC mapping
void arp_cache_update(const uint8_t *ip, const uint8_t *mac);
bool arp_resolve(const uint8_t *ip, uint8_t *mac_out);
#endif // ARP_H