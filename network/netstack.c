// netstack.c - Network Stack Implementation (Fixed)
#include "netstack.h"
#include "../rtl8139/rtl8139.h"
#include "../terminal/terminal.h"
#include "../utility/utility.h"
#include "../timers/timer.h"

// Debug flag - set to 0 to disable verbose output
#define NET_DEBUG 1

// Global network configuration
network_config_t net_config = {0};
arp_cache_entry_t arp_cache[ARP_CACHE_SIZE] = {0};

// Packet queue for waiting on ARP
typedef struct {
    uint32_t dest_ip;
    uint8_t protocol;
    uint8_t data[1500];
    uint16_t length;
    bool valid;
    uint32_t timestamp;
} pending_packet_t;

#define MAX_PENDING_PACKETS 8
pending_packet_t pending_packets[MAX_PENDING_PACKETS] = {0};

uint32_t ntohl(uint32_t n) {
    return htonl(n);
}

// Initialize network stack
void netstack_init(uint32_t ip, uint32_t subnet, uint32_t gateway) {
    net_config.ip_address = ip;
    net_config.subnet_mask = subnet;
    net_config.gateway = gateway;
    
    // Clear ARP cache
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        arp_cache[i].valid = false;
    }
    
    // Clear pending packets
    for (int i = 0; i < MAX_PENDING_PACKETS; i++) {
        pending_packets[i].valid = false;
    }
    
    print("Network Stack Initialized\n");
    print("IP: ");
    char buffer[32];
    ip_to_string(ip, buffer);
    print(buffer);
    print(" | Subnet: ");
    ip_to_string(subnet, buffer);
    print(buffer);
    print(" | Gateway: ");
    ip_to_string(gateway, buffer);
    print(buffer);
    print("\n");
}

// Set MAC address from RTL8139
void netstack_set_mac(uint8_t* mac) {
    for (int i = 0; i < 6; i++) {
        net_config.mac_address[i] = mac[i];
    }
    
    print("MAC Address: ");
    char buffer[32];
    mac_to_string(mac, buffer);
    print(buffer);
    print("\n");
}

// Process incoming packet
void netstack_process_packet(uint8_t* data, uint16_t length) {
    if (length < 14) {
        if (NET_DEBUG) print("ERROR: Packet too small\n");
        return;
    }
    
    ethernet_process(data, length);
    
    // Process any pending packets that might now have ARP entries
    netstack_process_pending();
}

// Process pending packets waiting for ARP
void netstack_process_pending(void) {
    uint32_t current_time = get_time_ms();
    
    for (int i = 0; i < MAX_PENDING_PACKETS; i++) {
        if (!pending_packets[i].valid) continue;
        
        // Timeout after 5 seconds
        if (current_time - pending_packets[i].timestamp > 5000) {
            if (NET_DEBUG) {
                print("Pending packet timeout\n");
            }
            pending_packets[i].valid = false;
            continue;
        }
        
        // Try to send again
        uint8_t dest_mac[6];
        if (arp_lookup(pending_packets[i].dest_ip, dest_mac)) {
            // Got MAC, send packet
            ipv4_header_t ip_header;
            ip_header.version_ihl = 0x45;
            ip_header.tos = 0;
            ip_header.total_length = htons(20 + pending_packets[i].length);
            ip_header.id = htons(1234);
            ip_header.flags_fragment = 0;
            ip_header.ttl = 64;
            ip_header.protocol = pending_packets[i].protocol;
            ip_header.checksum = 0;
            ip_header.src_ip = htonl(net_config.ip_address);
            ip_header.dest_ip = htonl(pending_packets[i].dest_ip);
            ip_header.checksum = ipv4_checksum((uint16_t*)&ip_header, 20);
            
            uint8_t packet[1520];
            memcpy(packet, &ip_header, 20);
            memcpy(packet + 20, pending_packets[i].data, pending_packets[i].length);
            
            ethernet_send(dest_mac, ETHERTYPE_IPV4, packet, 20 + pending_packets[i].length);
            pending_packets[i].valid = false;
            
            if (NET_DEBUG) print("Sent pending packet\n");
        }
    }
}

// Queue packet while waiting for ARP
void netstack_queue_packet(uint32_t dest_ip, uint8_t protocol, uint8_t* data, uint16_t length) {
    for (int i = 0; i < MAX_PENDING_PACKETS; i++) {
        if (!pending_packets[i].valid) {
            pending_packets[i].dest_ip = dest_ip;
            pending_packets[i].protocol = protocol;
            memcpy(pending_packets[i].data, data, length);
            pending_packets[i].length = length;
            pending_packets[i].valid = true;
            pending_packets[i].timestamp = get_time_ms();
            
            if (NET_DEBUG) print("Packet queued for ARP\n");
            return;
        }
    }
    
    if (NET_DEBUG) print("Pending packet queue full\n");
}

// ===== ETHERNET LAYER =====

void ethernet_send(uint8_t* dest_mac, uint16_t ethertype, uint8_t* payload, uint16_t length) {
    if (NET_DEBUG) {
        char buffer[32];
        print("TX Ethernet: type=0x");
        itoa(ethertype, buffer, 16);
        print(buffer);
        print(" len=");
        itoa(length, buffer, 10);
        print(buffer);
        print("\n");
    }
    
    ethernet_frame_t frame;
    
    // Set destination MAC
    for (int i = 0; i < 6; i++) {
        frame.dest_mac[i] = dest_mac[i];
    }
    
    // Set source MAC
    for (int i = 0; i < 6; i++) {
        frame.src_mac[i] = net_config.mac_address[i];
    }
    
    // Set EtherType (network byte order)
    frame.ethertype = htons(ethertype);
    
    // Copy payload (truncate if necessary)
    if (length > 1500) length = 1500;
    memcpy(frame.payload, payload, length);
    
    // Send via RTL8139
    rtl8139_send_packet((int8*)&frame, 14 + length);
}

void ethernet_process(uint8_t* frame, uint16_t length) {
    ethernet_frame_t* eth = (ethernet_frame_t*)frame;
    uint16_t ethertype = ntohs(eth->ethertype);
    
    if (NET_DEBUG) {
        char buffer[16];
        print("RX Ethernet: type=0x");
        itoa(ethertype, buffer, 16);
        print(buffer);
        print("\n");
    }
    
    switch (ethertype) {
        case ETHERTYPE_ARP:
            arp_process((arp_packet_t*)eth->payload);
            break;
            
        case ETHERTYPE_IPV4:
            ipv4_process((ipv4_header_t*)eth->payload, length - 14);
            break;
            
        default:
            if (NET_DEBUG) {
                print("Unknown EtherType: 0x");
                char buffer[16];
                itoa(ethertype, buffer, 16);
                print(buffer);
                print("\n");
            }
            break;
    }
}

// ===== ARP LAYER =====

void arp_send_request(uint32_t target_ip) {
    if (NET_DEBUG) {
        char buffer[32];
        print("ARP Request for ");
        ip_to_string(target_ip, buffer);
        print(buffer);
        print("\n");
    }
    
    arp_packet_t arp;
    
    arp.hw_type = htons(1); // Ethernet
    arp.protocol_type = htons(0x0800); // IPv4
    arp.hw_addr_len = 6;
    arp.protocol_addr_len = 4;
    arp.operation = htons(ARP_REQUEST);
    
    // Sender info
    for (int i = 0; i < 6; i++) {
        arp.sender_mac[i] = net_config.mac_address[i];
    }
    arp.sender_ip = htonl(net_config.ip_address);
    
    // Target info (MAC unknown)
    for (int i = 0; i < 6; i++) {
        arp.target_mac[i] = 0x00;
    }
    arp.target_ip = htonl(target_ip);
    
    // Send as broadcast
    uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    ethernet_send(broadcast_mac, ETHERTYPE_ARP, (uint8_t*)&arp, sizeof(arp_packet_t));
}

void arp_send_reply(uint32_t target_ip, uint8_t* target_mac) {
    if (NET_DEBUG) {
        char buffer[32];
        print("ARP Reply to ");
        ip_to_string(target_ip, buffer);
        print(buffer);
        print("\n");
    }
    
    arp_packet_t arp;
    
    arp.hw_type = htons(1);
    arp.protocol_type = htons(0x0800);
    arp.hw_addr_len = 6;
    arp.protocol_addr_len = 4;
    arp.operation = htons(ARP_REPLY);
    
    for (int i = 0; i < 6; i++) {
        arp.sender_mac[i] = net_config.mac_address[i];
    }
    arp.sender_ip = htonl(net_config.ip_address);
    
    for (int i = 0; i < 6; i++) {
        arp.target_mac[i] = target_mac[i];
    }
    arp.target_ip = htonl(target_ip);
    
    ethernet_send(target_mac, ETHERTYPE_ARP, (uint8_t*)&arp, sizeof(arp_packet_t));
}

void arp_process(arp_packet_t* arp) {
    uint16_t operation = ntohs(arp->operation);
    uint32_t sender_ip = ntohl(arp->sender_ip);
    uint32_t target_ip = ntohl(arp->target_ip);
    
    // Add sender to ARP cache
    arp_cache_add(sender_ip, arp->sender_mac);
    
    if (operation == ARP_REQUEST && target_ip == net_config.ip_address) {
        char buffer[32];
        print("ARP: Who has ");
        ip_to_string(target_ip, buffer);
        print(buffer);
        print("? Tell ");
        ip_to_string(sender_ip, buffer);
        print(buffer);
        print(" - Replying\n");
        
        arp_send_reply(sender_ip, arp->sender_mac);
    } else if (operation == ARP_REPLY) {
        char buffer[64];
        print("ARP: ");
        ip_to_string(sender_ip, buffer);
        print(buffer);
        print(" is at ");
        mac_to_string(arp->sender_mac, buffer);
        print(buffer);
        print("\n");
    }
}

bool arp_lookup(uint32_t ip, uint8_t* mac_out) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip == ip) {
            for (int j = 0; j < 6; j++) {
                mac_out[j] = arp_cache[i].mac[j];
            }
            return true;
        }
    }
    return false;
}

void arp_cache_add(uint32_t ip, uint8_t* mac) {
    // Check if entry already exists
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip == ip) {
            // Update existing entry
            for (int j = 0; j < 6; j++) {
                arp_cache[i].mac[j] = mac[j];
            }
            arp_cache[i].timestamp = get_time_ms();
            return;
        }
    }
    
    // Find empty slot
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (!arp_cache[i].valid) {
            arp_cache[i].ip = ip;
            for (int j = 0; j < 6; j++) {
                arp_cache[i].mac[j] = mac[j];
            }
            arp_cache[i].timestamp = get_time_ms();
            arp_cache[i].valid = true;
            return;
        }
    }
    
    // Cache full, overwrite oldest
    uint32_t oldest_time = arp_cache[0].timestamp;
    int oldest_idx = 0;
    
    for (int i = 1; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].timestamp < oldest_time) {
            oldest_time = arp_cache[i].timestamp;
            oldest_idx = i;
        }
    }
    
    arp_cache[oldest_idx].ip = ip;
    for (int i = 0; i < 6; i++) {
        arp_cache[oldest_idx].mac[i] = mac[i];
    }
    arp_cache[oldest_idx].timestamp = get_time_ms();
    arp_cache[oldest_idx].valid = true;
}

void arp_cache_dump(void) {
    char buffer[64];
    print("\n=== ARP Cache ===\n");
    
    bool found = false;
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid) {
            found = true;
            ip_to_string(arp_cache[i].ip, buffer);
            print(buffer);
            print(" -> ");
            mac_to_string(arp_cache[i].mac, buffer);
            print(buffer);
            print("\n");
        }
    }
    
    if (!found) {
        print("(empty)\n");
    }
    print("=================\n\n");
}

// ===== IPv4 LAYER =====

uint16_t ipv4_checksum(uint16_t* data, int length) {
    uint32_t sum = 0;
    
    while (length > 1) {
        sum += *data++;
        length -= 2;
    }
    
    if (length > 0) {
        sum += *(uint8_t*)data;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

void ipv4_send(uint32_t dest_ip, uint8_t protocol, uint8_t* payload, uint16_t length) {
    if (NET_DEBUG) {
        char buffer[32];
        print("IPv4 TX to ");
        ip_to_string(dest_ip, buffer);
        print(buffer);
        printr(" proto=%d len=%d\n", protocol, length);
    }
    
    ipv4_header_t ip_header;
    
    ip_header.version_ihl = 0x45; // IPv4, 20-byte header
    ip_header.tos = 0;
    ip_header.total_length = htons(20 + length);
    ip_header.id = htons(1234);
    ip_header.flags_fragment = 0;
    ip_header.ttl = 64;
    ip_header.protocol = protocol;
    ip_header.checksum = 0;
    ip_header.src_ip = htonl(net_config.ip_address);
    ip_header.dest_ip = htonl(dest_ip);
    
    // Calculate checksum
    ip_header.checksum = ipv4_checksum((uint16_t*)&ip_header, 20);
    
    // Combine header and payload
    uint8_t packet[1500];
    memcpy(packet, &ip_header, 20);
    memcpy(packet + 20, payload, length);
    
    // Look up MAC address
    uint8_t dest_mac[6];
    if (arp_lookup(dest_ip, dest_mac)) {
        ethernet_send(dest_mac, ETHERTYPE_IPV4, packet, 20 + length);
    } else {
        // Queue packet and send ARP request
        netstack_queue_packet(dest_ip, protocol, payload, length);
        arp_send_request(dest_ip);
    }
}

void ipv4_process(ipv4_header_t* ip_header, uint16_t length) {
    uint32_t dest_ip = ntohl(ip_header->dest_ip);
    uint32_t src_ip = ntohl(ip_header->src_ip);
    
    // Check if packet is for us (unicast or broadcast)
    if (dest_ip != net_config.ip_address && dest_ip != 0xFFFFFFFF) {
        if (NET_DEBUG) print("IPv4: Not for us\n");
        return;
    }
    
    uint8_t protocol = ip_header->protocol;
    uint8_t header_len = (ip_header->version_ihl & 0x0F) * 4;
    uint8_t* payload = ((uint8_t*)ip_header) + header_len;
    uint16_t payload_len = ntohs(ip_header->total_length) - header_len;
    
    if (NET_DEBUG) {
        char buffer[32];
        print("IPv4 RX from ");
        ip_to_string(src_ip, buffer);
        print(buffer);
        printr(" proto=%d len=%d\n", protocol, payload_len);
    }
    
    switch (protocol) {
        case IP_PROTOCOL_ICMP:
            icmp_process(ip_header, (icmp_header_t*)payload, payload_len);
            break;
            
        case IP_PROTOCOL_UDP:
            udp_process(ip_header, (udp_header_t*)payload, payload_len);
            break;
            
        case IP_PROTOCOL_TCP:
            if (NET_DEBUG) print("TCP packet (not implemented)\n");
            break;
            
        default:
            if (NET_DEBUG) printr("Unknown IP protocol: %d\n", protocol);
            break;
    }
}

// ===== ICMP LAYER =====

void icmp_send_echo_request(uint32_t dest_ip, uint16_t id, uint16_t seq) {
    char buffer[32];
    print("PING ");
    ip_to_string(dest_ip, buffer);
    print(buffer);
    printr(": seq=%d\n", seq);
    
    icmp_header_t icmp;
    icmp.type = ICMP_ECHO_REQUEST;
    icmp.code = 0;
    icmp.checksum = 0;
    icmp.id = htons(id);
    icmp.sequence = htons(seq);
    
    // Add payload data
    uint8_t payload[56];
    for (int i = 0; i < 56; i++) {
        payload[i] = 0x10 + (i % 32);
    }
    
    uint8_t packet[64];
    memcpy(packet, &icmp, 8);
    memcpy(packet + 8, payload, 56);
    
    // Calculate checksum
    icmp_header_t* icmp_ptr = (icmp_header_t*)packet;
    icmp_ptr->checksum = ipv4_checksum((uint16_t*)packet, 64);
    
    ipv4_send(dest_ip, IP_PROTOCOL_ICMP, packet, 64);
}

void icmp_send_echo_reply(uint32_t dest_ip, uint16_t id, uint16_t seq, uint8_t* data, uint16_t data_len) {
    icmp_header_t icmp;
    icmp.type = ICMP_ECHO_REPLY;
    icmp.code = 0;
    icmp.checksum = 0;
    icmp.id = id;
    icmp.sequence = seq;
    
    uint8_t packet[1480];
    if (data_len > 1472) data_len = 1472;
    
    memcpy(packet, &icmp, 8);
    memcpy(packet + 8, data, data_len);
    
    icmp_header_t* icmp_ptr = (icmp_header_t*)packet;
    icmp_ptr->checksum = ipv4_checksum((uint16_t*)packet, 8 + data_len);
    
    ipv4_send(dest_ip, IP_PROTOCOL_ICMP, packet, 8 + data_len);
}

void icmp_process(ipv4_header_t* ip_header, icmp_header_t* icmp, uint16_t length) {
    uint8_t type = icmp->type;
    uint32_t src_ip = ntohl(ip_header->src_ip);
    
    if (type == ICMP_ECHO_REQUEST) {
        char buffer[32];
        print("ICMP Echo Request from ");
        ip_to_string(src_ip, buffer);
        print(buffer);
        print(" - replying\n");
        
        // Send reply with same data
        uint8_t* data = ((uint8_t*)icmp) + 8;
        icmp_send_echo_reply(src_ip, icmp->id, icmp->sequence, data, length - 8);
    } else if (type == ICMP_ECHO_REPLY) {
        char buffer[32];
        print("PONG from ");
        ip_to_string(src_ip, buffer);
        print(buffer);
        printr(": seq=%d\n", ntohs(icmp->sequence));
    } else {
        if (NET_DEBUG) printr("ICMP type %d code %d\n", type, icmp->code);
    }
}

// ===== UDP LAYER =====

void udp_send(uint32_t dest_ip, uint16_t src_port, uint16_t dest_port, uint8_t* data, uint16_t length) {
    if (NET_DEBUG) {
        char buffer[32];
        print("UDP TX to ");
        ip_to_string(dest_ip, buffer);
        print(buffer);
        printr(":%d from port %d\n", dest_port, src_port);
    }
    
    udp_header_t udp;
    udp.src_port = htons(src_port);
    udp.dest_port = htons(dest_port);
    udp.length = htons(8 + length);
    udp.checksum = 0; // Optional for IPv4
    
    uint8_t packet[1480];
    if (length > 1472) length = 1472;
    
    memcpy(packet, &udp, 8);
    memcpy(packet + 8, data, length);
    
    ipv4_send(dest_ip, IP_PROTOCOL_UDP, packet, 8 + length);
}

void udp_process(ipv4_header_t* ip_header, udp_header_t* udp, uint16_t length) {
    uint16_t src_port = ntohs(udp->src_port);
    uint16_t dest_port = ntohs(udp->dest_port);
    uint32_t src_ip = ntohl(ip_header->src_ip);
    uint16_t data_len = ntohs(udp->length) - 8;
    
    char buffer[32];
    print("UDP from ");
    ip_to_string(src_ip, buffer);
    print(buffer);
    printr(":%d -> port %d (%d bytes)\n", src_port, dest_port, data_len);
    
    // Application can register UDP handlers here
}

// ===== UTILITY FUNCTIONS =====

uint32_t ip_parse(const char* ip_str) {
    uint32_t ip = 0;
    int octet = 0;
    
    for (int i = 0; ip_str[i]; i++) {
        if (ip_str[i] >= '0' && ip_str[i] <= '9') {
            octet = octet * 10 + (ip_str[i] - '0');
        } else if (ip_str[i] == '.') {
            ip = (ip << 8) | (octet & 0xFF);
            octet = 0;
        }
    }
    
    // Last octet
    ip = (ip << 8) | (octet & 0xFF);
    
    return ip;
}

void ip_to_string(uint32_t ip, char* buffer) {
    char temp[4];
    
    itoa((ip >> 24) & 0xFF, temp, 10);
    strcpy(buffer, temp);
    strcat(buffer, ".");
    
    itoa((ip >> 16) & 0xFF, temp, 10);
    strcat(buffer, temp);
    strcat(buffer, ".");
    
    itoa((ip >> 8) & 0xFF, temp, 10);
    strcat(buffer, temp);
    strcat(buffer, ".");
    
    itoa(ip & 0xFF, temp, 10);
    strcat(buffer, temp);
}

void mac_to_string(uint8_t* mac, char* buffer) {
    const char hex[] = "0123456789abcdef";
    buffer[0] = '\0';
    
    for (int i = 0; i < 6; i++) {
        int pos = strlen(buffer);
        buffer[pos] = hex[(mac[i] >> 4) & 0x0F];
        buffer[pos + 1] = hex[mac[i] & 0x0F];
        buffer[pos + 2] = (i < 5) ? ':' : '\0';
        buffer[pos + 3] = '\0';
    }
}