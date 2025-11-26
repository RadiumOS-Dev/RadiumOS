
// dns.c - DNS Client Implementation
#include "dns.h"
#include "netstack.h"
#include "../rtl8139/rtl8139.h"
#include "../terminal/terminal.h"
#include "../utility/utility.h"
#include "../timers/timer.h"
#include "networkrecievethread.h"
// DNS server configuration
static uint32_t dns_server = 0;
static uint16_t dns_transaction_id = 1234;

// Set DNS server
void dns_set_server(uint32_t dns_ip) {
    dns_server = dns_ip;
}

// Get DNS server
uint32_t dns_get_server(void) {
    return dns_server;
}

// Encode DNS name (convert "google.com" to length-prefixed format)
static int dns_encode_name(const char* hostname, uint8_t* buffer) {
    int pos = 0;
    int label_start = 0;
    int i = 0;
    
    while (hostname[i]) {
        if (hostname[i] == '.') {
            // Write label length
            int label_len = i - label_start;
            buffer[pos++] = label_len;
            
            // Copy label
            for (int j = 0; j < label_len; j++) {
                buffer[pos++] = hostname[label_start + j];
            }
            
            label_start = i + 1;
        }
        i++;
    }
    
    // Write final label
    int label_len = i - label_start;
    if (label_len > 0) {
        buffer[pos++] = label_len;
        for (int j = 0; j < label_len; j++) {
            buffer[pos++] = hostname[label_start + j];
        }
    }
    
    // Null terminator
    buffer[pos++] = 0;
    
    return pos;
}

// Decode DNS name from response
static int dns_decode_name(uint8_t* packet, int offset, char* name_out, int max_len) {
    int pos = offset;
    int out_pos = 0;
    bool jumped = false;
    int jump_offset = 0;
    
    while (packet[pos] != 0 && out_pos < max_len - 1) {
        // Check for compression pointer
        if ((packet[pos] & 0xC0) == 0xC0) {
            if (!jumped) {
                jump_offset = pos + 2;
            }
            // Get pointer offset
            int ptr = ((packet[pos] & 0x3F) << 8) | packet[pos + 1];
            pos = ptr;
            jumped = true;
            continue;
        }
        
        // Read label length
        int label_len = packet[pos++];
        
        // Copy label
        for (int i = 0; i < label_len && out_pos < max_len - 1; i++) {
            name_out[out_pos++] = packet[pos++];
        }
        
        // Add dot if not at end
        if (packet[pos] != 0) {
            name_out[out_pos++] = '.';
        }
    }
    
    name_out[out_pos] = '\0';
    
    return jumped ? jump_offset : pos + 1;
}

// Parse DNS response
static bool dns_parse_response(uint8_t* packet, int length, uint32_t* ip_out) {
    if (length < sizeof(dns_header_t)) {
        return false;
    }
    
    dns_header_t* header = (dns_header_t*)packet;
    
    // Check if it's a response
    if (!(ntohs(header->flags) & 0x8000)) {
        return false;
    }
    
    // Check for errors
    int rcode = ntohs(header->flags) & 0x000F;
    if (rcode != 0) {
        print("DNS error code: ");
        char err[8];
        itoa(rcode, err, 10);
        print(err);
        print("\n");
        return false;
    }
    
    int ancount = ntohs(header->ancount);
    if (ancount == 0) {
        print("DNS: No answers\n");
        return false;
    }
    
    // Skip questions section
    int pos = sizeof(dns_header_t);
    int qdcount = ntohs(header->qdcount);
    
    for (int i = 0; i < qdcount; i++) {
        // Skip name
        while (pos < length && packet[pos] != 0) {
            if ((packet[pos] & 0xC0) == 0xC0) {
                pos += 2;
                break;
            }
            pos += packet[pos] + 1;
        }
        if (pos < length && packet[pos] == 0) pos++;
        pos += 4; // Skip QTYPE and QCLASS
    }
    
    // Parse answers
    for (int i = 0; i < ancount && pos < length; i++) {
        // Skip name
        while (pos < length && packet[pos] != 0) {
            if ((packet[pos] & 0xC0) == 0xC0) {
                pos += 2;
                break;
            }
            pos += packet[pos] + 1;
        }
        if (pos < length && packet[pos] == 0) pos++;
        
        if (pos + 10 > length) break;
        
        uint16_t type = (packet[pos] << 8) | packet[pos + 1];
        pos += 2;
        uint16_t class = (packet[pos] << 8) | packet[pos + 1];
        pos += 2;
        pos += 4; // Skip TTL
        uint16_t rdlength = (packet[pos] << 8) | packet[pos + 1];
        pos += 2;
        
        if (type == DNS_TYPE_A && class == DNS_CLASS_IN && rdlength == 4) {
            // Found IPv4 address
            *ip_out = (packet[pos] << 24) | (packet[pos + 1] << 16) | 
                      (packet[pos + 2] << 8) | packet[pos + 3];
            return true;
        }
        
        pos += rdlength;
    }
    
    return false;
}

// Resolve hostname to IP address
bool dns_resolve(const char* hostname, uint32_t* ip_out) {
    if (!hostname || !ip_out) {
        return false;
    }
    
    if (dns_server == 0) {
        print("DNS server not configured\n");
        return false;
    }
    
    // Check if already an IP address
    if (hostname[0] >= '0' && hostname[0] <= '9') {
        *ip_out = ip_parse(hostname);
        return true;
    }
    
    print("Resolving ");
    print(hostname);
    print("...\n");
    
    // Build DNS query
    uint8_t query[512];
    int pos = 0;
    
    // DNS Header
    dns_header_t* header = (dns_header_t*)query;
    header->id = htons(dns_transaction_id++);
    header->flags = htons(0x0100); // Standard query, recursion desired
    header->qdcount = htons(1);
    header->ancount = 0;
    header->nscount = 0;
    header->arcount = 0;
    pos += sizeof(dns_header_t);
    
    // Question section
    pos += dns_encode_name(hostname, query + pos);
    
    // QTYPE (A record = 1)
    query[pos++] = 0;
    query[pos++] = DNS_TYPE_A;
    
    // QCLASS (IN = 1)
    query[pos++] = 0;
    query[pos++] = DNS_CLASS_IN;
    
    // Send DNS query via UDP
    udp_send(dns_server, 12345, DNS_PORT, query, pos);
    
    print("DNS query sent, waiting for response...\n");
    
    // Wait for response
    uint32_t start_time = get_time_ms();
    while (get_time_ms() - start_time < DNS_QUERY_TIMEOUT_MS) {
        // Process incoming packets
        static uint8_t rx_buffer[2048];
        int16 length;
        
        while (rtl8139_receive_packet((int8*)rx_buffer, &length)) {
            // Check if it's a DNS response
            if (length > 42) { // Ethernet + IP + UDP headers
                // Extract UDP payload
                ethernet_frame_t* eth = (ethernet_frame_t*)rx_buffer;
                if (ntohs(eth->ethertype) == ETHERTYPE_IPV4) {
                    ipv4_header_t* ip = (ipv4_header_t*)eth->payload;
                    if (ip->protocol == IP_PROTOCOL_UDP) {
                        uint8_t ip_header_len = (ip->version_ihl & 0x0F) * 4;
                        udp_header_t* udp = (udp_header_t*)(((uint8_t*)ip) + ip_header_len);
                        
                        if (ntohs(udp->src_port) == DNS_PORT) {
                            // This is a DNS response
                            uint8_t* dns_data = ((uint8_t*)udp) + 8;
                            int dns_len = ntohs(udp->length) - 8;
                            
                            if (dns_parse_response(dns_data, dns_len, ip_out)) {
                                char ip_str[32];
                                ip_to_string(*ip_out, ip_str);
                                print("Resolved to: ");
                                print(ip_str);
                                print("\n");
                                return true;
                            }
                        }
                    }
                }
            }
        }
        
        delay(10);
    }
    
    print("DNS query timeout\n");
    return false;
}

// Enhanced ping command with DNS support
void ping_dns_command(int argc, char* argv[]) {
    if (argc < 2) {
        print("Usage: ping <hostname or ip>\n");
        print("Example: ping google.com\n");
        print("Example: ping 8.8.8.8\n");
        return;
    }
    
    if (!RTL8139 || !RTL8139->initialized) {
        print("Network not initialized. Run 'network init' first.\n");
        return;
    }
    
    // Resolve hostname
    uint32_t dest_ip;
    if (!dns_resolve(argv[1], &dest_ip)) {
        print("Failed to resolve hostname\n");
        return;
    }
    
    char ip_str[32];
    ip_to_string(dest_ip, ip_str);
    
    print("\nPING ");
    print(argv[1]);
    print(" (");
    print(ip_str);
    print(") 64 bytes of data:\n\n");
    
    int replies = 0;
    uint32_t total_time = 0;
    uint32_t min_time = 0xFFFFFFFF;
    uint32_t max_time = 0;
    
    for (int i = 0; i < 4; i++) {
        uint32_t start_time = get_time_ms();
        
        // Send echo request
        icmp_send_echo_request(dest_ip, 1234, i);
        
        // Wait for reply
        bool got_reply = false;
        uint32_t rtt = 0;
        
        while (get_time_ms() - start_time < 2000) {
            network_receive_thread();
            
            // Simple check - if we get any packet within 100ms, assume it's our reply
            // In production, you'd check the ICMP ID and sequence
            rtt = get_time_ms() - start_time;
            if (rtt < 100) {
                got_reply = true;
                replies++;
                total_time += rtt;
                if (rtt < min_time) min_time = rtt;
                if (rtt > max_time) max_time = rtt;
                break;
            }
            
            delay(10);
        }
        
        if (got_reply) {
            print("64 bytes from ");
            print(ip_str);
            print(": icmp_seq=");
            char seq[8];
            itoa(i, seq, 10);
            print(seq);
            print(" ttl=64 time=");
            char time_str[16];
            itoa(rtt, time_str, 10);
            print(time_str);
            print(" ms\n");
        } else {
            print("Request timeout for icmp_seq ");
            char seq[8];
            itoa(i, seq, 10);
            print(seq);
            print("\n");
        }
        
        delay(1000);
    }
    
    print("\n--- ");
    print(argv[1]);
    print(" ping statistics ---\n");
    print("4 packets transmitted, ");
    char stats[16];
    itoa(replies, stats, 10);
    print(stats);
    print(" packets received, ");
    itoa(((4 - replies) * 100) / 4, stats, 10);
    print(stats);
    print("% packet loss\n");
    
    if (replies > 0) {
        print("round-trip min/avg/max = ");
        itoa(min_time, stats, 10);
        print(stats);
        print("/");
        itoa(total_time / replies, stats, 10);
        print(stats);
        print("/");
        itoa(max_time, stats, 10);
        print(stats);
        print(" ms\n");
    }
}

// DNS lookup command
void nslookup_command(int argc, char* argv[]) {
    if (argc < 2) {
        print("Usage: nslookup <hostname>\n");
        print("Example: nslookup google.com\n");
        return;
    }
    
    if (!RTL8139 || !RTL8139->initialized) {
        print("Network not initialized\n");
        return;
    }
    
    if (dns_server == 0) {
        print("DNS server not configured\n");
        print("Use: network dns <server_ip>\n");
        return;
    }
    
    char dns_str[32];
    ip_to_string(dns_server, dns_str);
    print("Server: ");
    print(dns_str);
    print("\n\n");
    
    uint32_t ip;
    if (dns_resolve(argv[1], &ip)) {
        print("Name: ");
        print(argv[1]);
        print("\nAddress: ");
        char ip_str[32];
        ip_to_string(ip, ip_str);
        print(ip_str);
        print("\n");
    } else {
        print("*** Can't find ");
        print(argv[1]);
        print(": Non-existent domain\n");
    }
}