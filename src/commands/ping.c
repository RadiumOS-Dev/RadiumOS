#include "../utility/utility.h"
#include "../terminal/terminal.h"
#include "../keyboard/keyboard.h"
#include "../timers/timer.h"
#include "../io/io.h"
#include "../rtl8139/rtl8139.h"

// External RTL8139 reference
extern struct rtl8139* RTL8139;

// Network structures
typedef struct {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
} __attribute__((packed)) ethernet_header_t;

typedef struct {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
} __attribute__((packed)) ip_header_t;

typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequence;
} __attribute__((packed)) icmp_header_t;

// Network constants
#define ETHERTYPE_IP    0x0800
#define IP_PROTOCOL_ICMP 1
#define ICMP_TYPE_ECHO_REQUEST 8
#define ICMP_TYPE_ECHO_REPLY   0

// Default gateway MAC (QEMU default)
uint8_t gateway_mac[6] = {0x52, 0x55, 0x0a, 0x00, 0x02, 0x02};

// Simple IP address configuration (QEMU user networking defaults)
//uint32_t my_ip = 0x0a000202;      // 10.0.2.2 (QEMU guest IP)
uint32_t gateway_ip = 0x0a000201;  // 10.0.2.1 (QEMU gateway)

// Parse IP address string (e.g., "192.168.1.1") to uint32_t
uint32_t parse_ip_address(const char* ip_str) {
    if (!ip_str) return 0;
    
    uint32_t ip = 0;
    uint32_t octet = 0;
    int octet_count = 0;
    int i = 0;
    
    while (ip_str[i] != '\0' && octet_count < 4) {
        char c = ip_str[i];
        
        if (c >= '0' && c <= '9') {
            octet = octet * 10 + (c - '0');
            if (octet > 255) {
                return 0; // Invalid octet
            }
        } else if (c == '.') {
            ip = (ip << 8) | octet;
            octet = 0;
            octet_count++;
        } else {
            return 0; // Invalid character
        }
        
        i++;
    }
    
    // Add the last octet
    if (octet_count == 3 && octet <= 255) {
        ip = (ip << 8) | octet;
        return ip;
    }
    
    return 0; // Invalid format
}

// Convert uint32_t IP to string
void ip_to_string(uint32_t ip, char* buffer) {
    uint8_t a = (ip >> 24) & 0xFF;
    uint8_t b = (ip >> 16) & 0xFF;
    uint8_t c = (ip >> 8) & 0xFF;
    uint8_t d = ip & 0xFF;
    
    char temp[4];
    buffer[0] = '\0';
    
    itoa(a, temp, 10);
    strcat(buffer, temp);
    strcat(buffer, ".");
    
    itoa(b, temp, 10);
    strcat(buffer, temp);
    strcat(buffer, ".");
    
    itoa(c, temp, 10);
    strcat(buffer, temp);
    strcat(buffer, ".");
    
    itoa(d, temp, 10);
    strcat(buffer, temp);
}

// Calculate IP checksum
uint16_t calculate_ip_checksum(ip_header_t* header) {
    uint16_t* data = (uint16_t*)header;
    uint32_t sum = 0;
    
    // Set checksum to 0 for calculation
    header->checksum = 0;
    
    // Sum all 16-bit words in header
    for (int i = 0; i < sizeof(ip_header_t) / 2; i++) {
        sum += ntohs(data[i]);
    }
    
    // Add carry bits
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return htons(~sum);
}

// Calculate ICMP checksum
uint16_t calculate_icmp_checksum(icmp_header_t* header, uint8_t* data, uint16_t data_len) {
    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)header;
    
    // Set checksum to 0 for calculation
    header->checksum = 0;
    
    // Sum ICMP header (8 bytes = 4 words)
    for (int i = 0; i < 4; i++) {
        sum += ntohs(ptr[i]);
    }
    
    // Sum data
    for (int i = 0; i < data_len / 2; i++) {
        sum += ntohs(((uint16_t*)data)[i]);
    }
    
    // Handle odd byte
    if (data_len % 2) {
        sum += (data[data_len - 1] << 8);
    }
    
    // Add carry bits
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return htons(~sum);
}

// Send ping packet
bool send_ping(uint32_t dest_ip, uint16_t identifier, uint16_t sequence) {
    if (!RTL8139 || !RTL8139->initialized) {
        warn("RTL8139 not initialized", __FILE__);
        return false;
    }
    
    // Create packet buffer
    uint8_t packet[128];
    uint16_t packet_len = 0;
    
    // Ethernet header
    ethernet_header_t* eth = (ethernet_header_t*)packet;
    memcpy(eth->dest_mac, gateway_mac, 6);
    memcpy(eth->src_mac, RTL8139->mac_address, 6);
    eth->ethertype = htons(ETHERTYPE_IP);
    packet_len += sizeof(ethernet_header_t);
    
    // IP header
    ip_header_t* ip = (ip_header_t*)(packet + packet_len);
    ip->version_ihl = 0x45;  // IPv4, 20 byte header
    ip->tos = 0;
    ip->total_length = htons(sizeof(ip_header_t) + sizeof(icmp_header_t) + 32);
    ip->identification = htons(0x1234);
    ip->flags_fragment = htons(0x4000); // Don't fragment
    ip->ttl = 64;
    ip->protocol = IP_PROTOCOL_ICMP;
    uint8_t ip_bytes[4] = {10, 0, 2, 2};
// Convert to uint32_t in host byte order
uint32_t ip_host_order = (ip_bytes[0] << 24) | (ip_bytes[1] << 16) | (ip_bytes[2] << 8) | ip_bytes[3];
// Convert to network byte order
ip->src_ip = htonl(ip_host_order);
    ip->dest_ip = htonl(dest_ip);
    ip->checksum = calculate_ip_checksum(ip);
    packet_len += sizeof(ip_header_t);
    
    // ICMP header
    icmp_header_t* icmp = (icmp_header_t*)(packet + packet_len);
    icmp->type = ICMP_TYPE_ECHO_REQUEST;
    icmp->code = 0;
    icmp->identifier = htons(identifier);
    icmp->sequence = htons(sequence);
    packet_len += sizeof(icmp_header_t);
    
    // ICMP data (32 bytes of pattern)
    uint8_t* icmp_data = packet + packet_len;
    for (int i = 0; i < 32; i++) {
        icmp_data[i] = 0x41 + (i % 26); // A-Z pattern
    }
    packet_len += 32;
    
    // Calculate ICMP checksum
    icmp->checksum = calculate_icmp_checksum(icmp, icmp_data, 32);
    
    // Debug: Print packet info
    print("Sending packet: ");
    char buffer[16];
    itoa(packet_len, buffer, 10);
    print(buffer);
    print(" bytes\n");
    
    // Send packet
    return rtl8139_send_packet((int8*)packet, packet_len);
}

// Receive and process ping reply
bool receive_ping_reply(uint16_t expected_id, uint16_t expected_seq, uint32_t* reply_time) {
    uint8_t buffer[1500];
    int16 length;
    uint32_t start_time = get_ticks();
    
    // Use a simple counter instead of relying on timer
    int timeout_counter = 5000; // Adjust this value
    
    while (timeout_counter > 0) {
        if (rtl8139_receive_packet((int8*)buffer, &length)) {
            print("Received packet: ");
            char len_str[16];
            itoa(length, len_str, 10);
            print(len_str);
            print(" bytes\n");
            
            // Parse Ethernet header
            if (length < sizeof(ethernet_header_t)) {
                timeout_counter--;
                continue;
            }
            
            ethernet_header_t* eth = (ethernet_header_t*)buffer;
            if (ntohs(eth->ethertype) != ETHERTYPE_IP) {
                print("Not IP packet\n");
                timeout_counter--;
                continue;
            }
            
            // Parse IP header
            if (length < sizeof(ethernet_header_t) + sizeof(ip_header_t)) {
                timeout_counter--;
                continue;
            }
            
            ip_header_t* ip = (ip_header_t*)(buffer + sizeof(ethernet_header_t));
            if (ip->protocol != IP_PROTOCOL_ICMP) {
                print("Not ICMP packet\n");
                timeout_counter--;
                continue;
            }
            
            // Parse ICMP header
            if (length < sizeof(ethernet_header_t) + sizeof(ip_header_t) + sizeof(icmp_header_t)) {
                timeout_counter--;
                continue;
            }
            
            icmp_header_t* icmp = (icmp_header_t*)(buffer + sizeof(ethernet_header_t) + sizeof(ip_header_t));
            
            print("ICMP type: ");
            char type_str[16];
            itoa(icmp->type, type_str, 10);
            print(type_str);
            print("\n");
            
            if (icmp->type == ICMP_TYPE_ECHO_REPLY &&
                ntohs(icmp->identifier) == expected_id &&
                ntohs(icmp->sequence) == expected_seq) {
                
                *reply_time = get_ticks() - start_time;
                return true;
            }
        }
        
        timeout_counter--;
        
        // Show progress
        if (timeout_counter % 1000 == 0) {
            print(".");
        }
    }
    
    print("\nTimeout reached\n");
    return false;
}

// Ping function that accepts IP address
void ping_ip(const char* ip_str) {
    if (!RTL8139 || !RTL8139->initialized) {
        warn("Network card not initialized", __FILE__);
        return;
    }
    
    // Parse the IP address
    uint32_t target_ip = parse_ip_address(ip_str);
    if (target_ip == 0) {
        terminal_setcolor(VGA_COLOR_RED);
        print("Invalid IP address format: ");
        print(ip_str);
        print("\n");
        terminal_setcolor(VGA_COLOR_WHITE);
        return;
    }
    
    // Convert back to string for display
    char ip_display[16];
    ip_to_string(target_ip, ip_display);
    
    info("Starting ping test", __FILE__);
    
    print("\nPING ");
    print(ip_display);
    print("\n");
    print("Network card status: ");
    if (RTL8139->initialized) {
        print("OK\n");
    } else {
        print("NOT READY\n");
        return;
    }
    
    // Show our configuration
    print("Our IP: 10.0.2.2\n");
    print("Gateway: 10.0.2.1\n");
    print("Target: ");
    print(ip_display);
    print("\n\n");
    
    uint16_t identifier = 0x1234;
    
    // Send ping packet
    print("Sending ping packet to ");
    print(ip_display);
    print("...\n");
    
    if (send_ping(target_ip, identifier, 1)) {
        print("Packet sent successfully\n");
        
        // Wait for reply
        uint32_t reply_time;
        print("Waiting for reply...\n");
        
        if (receive_ping_reply(identifier, 1, &reply_time)) {
            terminal_setcolor(VGA_COLOR_GREEN);
            print("Reply from ");
            print(ip_display);
            print("! Time: ");
            char time_str[16];
            itoa(reply_time, time_str, 10);
            print(time_str);
            print(" ticks\n");
            terminal_setcolor(VGA_COLOR_WHITE);
        } else {
            terminal_setcolor(VGA_COLOR_RED);
            print("Request timed out\n");
            terminal_setcolor(VGA_COLOR_WHITE);
        }
    } else {
        terminal_setcolor(VGA_COLOR_RED);
        print("Failed to send ping packet\n");
        terminal_setcolor(VGA_COLOR_WHITE);
    }
    
    done("Ping test completed", __FILE__);
}

// Predefined targets for convenience
void ping_predefined(const char* target) {
    if (strcmp(target, "google") == 0 || strcmp(target, "dns") == 0) {
        ping_ip("8.8.8.8");
    } else if (strcmp(target, "cloudflare") == 0) {
        ping_ip("1.1.1.1");
    } else if (strcmp(target, "gateway") == 0) {
        ping_ip("10.0.2.1");
    } else if (strcmp(target, "localhost") == 0) {
        ping_ip("127.0.0.1");
    } else {
        print("Unknown predefined target: ");
        print(target);
        print("\n");
    }
}

// Command wrapper for shell integration
void text(int argc, char* argv[]) {
    if (argc < 2) {
        print("Usage: ping <IP_address|target>\n");
        print("Examples:\n");
        print("  ping 8.8.8.8        - Ping Google DNS\n");
        print("  ping 192.168.1.1    - Ping custom IP\n");
        print("  ping 10.0.2.1       - Ping gateway\n");
        print("\nPredefined targets:\n");
        print("  google/dns     - Google DNS (8.8.8.8)\n");
                print("  cloudflare     - Cloudflare DNS (1.1.1.1)\n");
        print("  gateway        - Default gateway (10.0.2.1)\n");
        print("  localhost      - Loopback (127.0.0.1)\n");
        return;
    }
    
    const char* target = argv[1];
    
    // Check if it's a predefined target
    if (strcmp(target, "google") == 0 || strcmp(target, "dns") == 0 ||
        strcmp(target, "cloudflare") == 0 || strcmp(target, "gateway") == 0 ||
        strcmp(target, "localhost") == 0) {
        ping_predefined(target);
    } else {
        // Assume it's an IP address
        ping_ip(target);
    }
}

// Additional utility functions for network testing

// Test multiple pings to an IP
void ping_test(const char* ip_str, int count) {
    if (!RTL8139 || !RTL8139->initialized) {
        warn("Network card not initialized", __FILE__);
        return;
    }
    
    uint32_t target_ip = parse_ip_address(ip_str);
    if (target_ip == 0) {
        terminal_setcolor(VGA_COLOR_RED);
        print("Invalid IP address format: ");
        print(ip_str);
        print("\n");
        terminal_setcolor(VGA_COLOR_WHITE);
        return;
    }
    
    char ip_display[16];
    ip_to_string(target_ip, ip_display);
    
    print("PING ");
    print(ip_display);
    print(" - ");
    char count_str[8];
    itoa(count, count_str, 10);
    print(count_str);
    print(" packets\n");
    
    int successful = 0;
    uint32_t total_time = 0;
    uint16_t identifier = 0x1234;
    
    for (int i = 1; i <= count; i++) {
        print("Ping ");
        char seq_str[8];
        itoa(i, seq_str, 10);
        print(seq_str);
        print(": ");
        
        if (send_ping(target_ip, identifier, i)) {
            uint32_t reply_time;
            if (receive_ping_reply(identifier, i, &reply_time)) {
                terminal_setcolor(VGA_COLOR_GREEN);
                print("Reply from ");
                print(ip_display);
                print(" time=");
                char time_str[16];
                itoa(reply_time, time_str, 10);
                print(time_str);
                print(" ticks\n");
                terminal_setcolor(VGA_COLOR_WHITE);
                
                successful++;
                total_time += reply_time;
            } else {
                terminal_setcolor(VGA_COLOR_RED);
                print("Request timed out\n");
                terminal_setcolor(VGA_COLOR_WHITE);
            }
        } else {
            terminal_setcolor(VGA_COLOR_RED);
            print("Send failed\n");
            terminal_setcolor(VGA_COLOR_WHITE);
        }
        
        // Small delay between pings
        delay(100);
    }
    
    // Print statistics
    print("\n--- ");
    print(ip_display);
    print(" ping statistics ---\n");
    print("Packets: sent=");
    itoa(count, count_str, 10);
    print(count_str);
    print(", received=");
    itoa(successful, count_str, 10);
    print(count_str);
    print(", lost=");
    itoa(count - successful, count_str, 10);
    print(count_str);
    
    if (count > 0) {
        int loss_percent = ((count - successful) * 100) / count;
        print(" (");
        itoa(loss_percent, count_str, 10);
        print(count_str);
        print("% loss)\n");
    }
    
    if (successful > 0) {
        uint32_t avg_time = total_time / successful;
        print("Average time: ");
        itoa(avg_time, count_str, 10);
        print(count_str);
        print(" ticks\n");
    }
}

// Validate IP address format
bool is_valid_ip(const char* ip_str) {
    return parse_ip_address(ip_str) != 0;
}

// Enhanced ping command with options
void ping_command(int argc, char* argv[]) {
    if (argc < 2) {
        print("Usage: ping [options] <IP_address|target>\n");
        print("Options:\n");
        print("  -c <count>     Number of pings to send\n");
        print("  -t             Continuous ping (not implemented)\n");
        print("\nExamples:\n");
        print("  ping 8.8.8.8\n");
        print("  ping -c 5 google\n");
        print("  ping gateway\n");
        return;
    }
    
    int ping_count = 1;
    const char* target = NULL;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            // Parse count
            ping_count = 0;
            const char* count_str = argv[i + 1];
            for (int j = 0; count_str[j] != '\0'; j++) {
                if (count_str[j] >= '0' && count_str[j] <= '9') {
                    ping_count = ping_count * 10 + (count_str[j] - '0');
                } else {
                    print("Invalid count: ");
                    print(count_str);
                    print("\n");
                    return;
                }
            }
            i++; // Skip the count argument
        } else if (argv[i][0] != '-') {
            target = argv[i];
        }
    }
    
    if (!target) {
        print("No target specified\n");
        return;
    }
    
    if (ping_count <= 0 || ping_count > 100) {
        print("Invalid ping count (1-100)\n");
        return;
    }
    
    // Resolve target to IP
    char ip_str[16];
    if (strcmp(target, "google") == 0 || strcmp(target, "dns") == 0) {
        strcpy(ip_str, "8.8.8.8");
    } else if (strcmp(target, "cloudflare") == 0) {
        strcpy(ip_str, "1.1.1.1");
    } else if (strcmp(target, "gateway") == 0) {
        strcpy(ip_str, "10.0.2.1");
    } else if (strcmp(target, "localhost") == 0) {
        strcpy(ip_str, "127.0.0.1");
    } else if (is_valid_ip(target)) {
        strcpy(ip_str, target);
    } else {
        print("Invalid target: ");
        print(target);
        print("\n");
        return;
    }
    
    // Execute ping
    if (ping_count == 1) {
        ping_ip(ip_str);
    } else {
        ping_test(ip_str, ping_count);
    }
}

// Network diagnostic command
void netdiag_command(int argc, char* argv[]) {
    print("=== Network Diagnostics ===\n");
    
    if (!RTL8139 || !RTL8139->initialized) {
        terminal_setcolor(VGA_COLOR_RED);
        print("Network card: NOT INITIALIZED\n");
        terminal_setcolor(VGA_COLOR_WHITE);
        return;
    }
    
    terminal_setcolor(VGA_COLOR_GREEN);
    print("Network card: OK\n");
    terminal_setcolor(VGA_COLOR_WHITE);
    
    print("MAC Address: ");
    for (int i = 0; i < 6; i++) {
        char hex_str[4];
        itoa(RTL8139->mac_address[i], hex_str, 16);
        if (RTL8139->mac_address[i] < 16) {
            print("0");
        }
        print(hex_str);
        if (i < 5) print(":");
    }
    print("\n");
    
    print("Our IP: 10.0.2.2\n");
    print("Gateway: 10.0.2.1\n");
    
    print("\nTesting connectivity...\n");
    print("1. Testing gateway: ");
    ping_ip("10.0.2.1");
    
    print("\n2. Testing Google DNS: ");
    ping_ip("8.8.8.8");
    
    print("\n=== Diagnostics Complete ===\n");
}