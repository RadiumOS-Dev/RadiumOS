// tcp.c - Simple TCP implementation for HTTP client
#include "netstack.h"
#include "../terminal/terminal.h"
#include "../utility/utility.h"
#include "../timers/timer.h"

// TCP Header
typedef struct __attribute__((packed)) {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t data_offset_flags_high; // data offset (4 bits) + reserved (3 bits) + NS flag
    uint8_t flags;                   // CWR, ECE, URG, ACK, PSH, RST, SYN, FIN
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_pointer;
} tcp_header_t;

// TCP Flags
#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10
#define TCP_URG 0x20

// TCP Connection States
typedef enum {
    TCP_CLOSED,
    TCP_SYN_SENT,
    TCP_ESTABLISHED,
    TCP_FIN_WAIT_1,
    TCP_FIN_WAIT_2,
    TCP_CLOSING,
    TCP_TIME_WAIT,
    TCP_CLOSE_WAIT,
    TCP_LAST_ACK
} tcp_state_t;

// TCP Connection
typedef struct {
    uint32_t remote_ip;
    uint16_t local_port;
    uint16_t remote_port;
    tcp_state_t state;
    uint32_t seq_num;
    uint32_t ack_num;
    uint16_t window_size;
    bool active;
    uint32_t last_activity;
    
    // Receive buffer
    uint8_t rx_buffer[8192];
    uint16_t rx_length;
    bool rx_complete;
} tcp_connection_t;

#define MAX_TCP_CONNECTIONS 4
static tcp_connection_t tcp_connections[MAX_TCP_CONNECTIONS] = {0};
static uint32_t tcp_seq_base = 12345; // Initial sequence number
static inline const char* tcp_state_to_string_inline(tcp_state_t state) {
    switch (state) {
        case TCP_CLOSED: return "CLOSED";
        case TCP_SYN_SENT: return "SYN_SENT";
        case TCP_ESTABLISHED: return "ESTABLISHED";
        case TCP_FIN_WAIT_1: return "FIN_WAIT_1";
        case TCP_FIN_WAIT_2: return "FIN_WAIT_2";
        case TCP_CLOSING: return "CLOSING";
        case TCP_TIME_WAIT: return "TIME_WAIT";
        case TCP_CLOSE_WAIT: return "CLOSE_WAIT";
        case TCP_LAST_ACK: return "LAST_ACK";
        default: return "UNKNOWN";
    }
}
// Calculate TCP checksum (includes pseudo-header)
uint16_t tcp_checksum(uint32_t src_ip, uint32_t dest_ip, tcp_header_t* tcp, uint16_t tcp_length, uint8_t* data, uint16_t data_length) {
    uint32_t sum = 0;
    
    // Pseudo-header
    sum += (src_ip >> 16) & 0xFFFF;
    sum += src_ip & 0xFFFF;
    sum += (dest_ip >> 16) & 0xFFFF;
    sum += dest_ip & 0xFFFF;
    sum += htons(IP_PROTOCOL_TCP);
    sum += htons(tcp_length + data_length);
    
    // TCP header
    uint16_t* ptr = (uint16_t*)tcp;
    for (int i = 0; i < tcp_length / 2; i++) {
        sum += ptr[i];
    }
    
    // Data
    ptr = (uint16_t*)data;
    int remaining = data_length;
    while (remaining > 1) {
        sum += *ptr++;
        remaining -= 2;
    }
    
    if (remaining > 0) {
        sum += *(uint8_t*)ptr;
    }
    
    // Fold carry
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

// Find or create TCP connection
tcp_connection_t* tcp_get_connection(uint32_t remote_ip, uint16_t local_port, uint16_t remote_port) {
    // Find existing connection
    for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
        if (tcp_connections[i].active &&
            tcp_connections[i].remote_ip == remote_ip &&
            tcp_connections[i].local_port == local_port &&
            tcp_connections[i].remote_port == remote_port) {
            return &tcp_connections[i];
        }
    }
    
    // Find free slot
    for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
        if (!tcp_connections[i].active) {
            tcp_connections[i].remote_ip = remote_ip;
            tcp_connections[i].local_port = local_port;
            tcp_connections[i].remote_port = remote_port;
            tcp_connections[i].state = TCP_CLOSED;
            tcp_connections[i].seq_num = tcp_seq_base++;
            tcp_connections[i].ack_num = 0;
            tcp_connections[i].window_size = 8192;
            tcp_connections[i].active = true;
            tcp_connections[i].rx_length = 0;
            tcp_connections[i].rx_complete = false;
            tcp_connections[i].last_activity = get_time_ms();
            return &tcp_connections[i];
        }
    }
    
    return NULL;
}

// Send TCP packet
void tcp_send_packet(tcp_connection_t* conn, uint8_t flags, uint8_t* data, uint16_t data_length) {
    uint8_t packet[1500];
    tcp_header_t* tcp = (tcp_header_t*)packet;
    
    tcp->src_port = htons(conn->local_port);
    tcp->dest_port = htons(conn->remote_port);
    tcp->seq_num = htonl(conn->seq_num);
    tcp->ack_num = htonl(conn->ack_num);
    tcp->data_offset_flags_high = 0x50; // 20 bytes header, no options
    tcp->flags = flags;
    tcp->window_size = htons(conn->window_size);
    tcp->checksum = 0;
    tcp->urgent_pointer = 0;
    
    // Copy data
    if (data && data_length > 0) {
        memcpy(packet + 20, data, data_length);
    }
    
    // Calculate checksum
    tcp->checksum = tcp_checksum(net_config.ip_address, conn->remote_ip, 
                                  tcp, 20, data, data_length);
    
    // Send via IP
    ipv4_send(conn->remote_ip, IP_PROTOCOL_TCP, packet, 20 + data_length);
    
    // Update sequence number if we sent data or SYN/FIN
    if (data_length > 0 || (flags & (TCP_SYN | TCP_FIN))) {
        conn->seq_num += (data_length > 0) ? data_length : 1;
    }
    
    conn->last_activity = get_time_ms();
}

// Process incoming TCP packet
void tcp_process(ipv4_header_t* ip_header, uint8_t* tcp_data, uint16_t length) {
    if (length < 20) return;
    
    tcp_header_t* tcp = (tcp_header_t*)tcp_data;
    uint16_t src_port = ntohs(tcp->src_port);
    uint16_t dest_port = ntohs(tcp->dest_port);
    uint32_t seq = ntohl(tcp->seq_num);
    uint32_t ack = ntohl(tcp->ack_num);
    uint8_t flags = tcp->flags;
    uint16_t header_len = ((tcp->data_offset_flags_high >> 4) & 0x0F) * 4;
    
    uint32_t src_ip = ntohl(ip_header->src_ip);
    
    // Find connection
    tcp_connection_t* conn = tcp_get_connection(src_ip, dest_port, src_port);
    if (!conn) {
        print("TCP: No connection slots available\n");
        return;
    }
    
    // Update last activity
    conn->last_activity = get_time_ms();
    
    // State machine
    if (flags & TCP_RST) {
        print("TCP: Connection reset\n");
        conn->state = TCP_CLOSED;
        conn->active = false;
        return;
    }
    
    if (conn->state == TCP_SYN_SENT && (flags & TCP_SYN) && (flags & TCP_ACK)) {
        // SYN-ACK received
        print("TCP: SYN-ACK received, connection established\n");
        conn->ack_num = seq + 1;
        conn->state = TCP_ESTABLISHED;
        
        // Send ACK
        tcp_send_packet(conn, TCP_ACK, NULL, 0);
    }
    else if (conn->state == TCP_ESTABLISHED) {
        if (flags & TCP_ACK) {
            // Update ack number
            if (flags & TCP_PSH || length > header_len) {
                // Data received
                uint16_t data_len = length - header_len;
                uint8_t* data = tcp_data + header_len;
                
                printr("TCP: Received %d bytes\n", data_len);
                
                // Copy to buffer
                if (conn->rx_length + data_len < sizeof(conn->rx_buffer)) {
                    memcpy(conn->rx_buffer + conn->rx_length, data, data_len);
                    conn->rx_length += data_len;
                }
                
                // Update ack
                conn->ack_num = seq + data_len;
                
                // Send ACK
                tcp_send_packet(conn, TCP_ACK, NULL, 0);
            }
            
            if (flags & TCP_FIN) {
                print("TCP: FIN received, closing connection\n");
                conn->ack_num = seq + 1;
                conn->state = TCP_CLOSE_WAIT;
                conn->rx_complete = true;
                
                // Send ACK
                tcp_send_packet(conn, TCP_ACK, NULL, 0);
                
                // Send FIN
                tcp_send_packet(conn, TCP_FIN | TCP_ACK, NULL, 0);
                conn->state = TCP_LAST_ACK;
            }
        }
    }
    else if (conn->state == TCP_LAST_ACK && (flags & TCP_ACK)) {
        print("TCP: Connection closed\n");
        conn->state = TCP_CLOSED;
        conn->active = false;
    }
}

// Connect to remote host
tcp_connection_t* tcp_connect(uint32_t remote_ip, uint16_t remote_port, uint16_t local_port) {
    char ip_str[32];
    ip_to_string(remote_ip, ip_str);
    printr("TCP: Connecting to %s:%d from port %d\n", ip_str, remote_port, local_port);
    
    tcp_connection_t* conn = tcp_get_connection(remote_ip, local_port, remote_port);
    if (!conn) {
        print("TCP: Cannot create connection\n");
        return NULL;
    }
    
    // Send SYN
    conn->state = TCP_SYN_SENT;
    tcp_send_packet(conn, TCP_SYN, NULL, 0);
    
    // Wait for SYN-ACK (simplified)
    uint32_t start = get_time_ms();
    while (get_time_ms() - start < 5000) {
        if (conn->state == TCP_ESTABLISHED) {
            print("TCP: Connected!\n");
            return conn;
        }
        delay(10);
    }
    
    print("TCP: Connection timeout\n");
    conn->active = false;
    return NULL;
}

// Send data over TCP
int tcp_send_data(tcp_connection_t* conn, uint8_t* data, uint16_t length) {
    if (!conn || conn->state != TCP_ESTABLISHED) {
        print("TCP: Not connected\n");
        return -1;
    }
    
    printr("TCP: Sending %d bytes\n", length);
    
    // Send in chunks if necessary
    uint16_t sent = 0;
    while (sent < length) {
        uint16_t chunk = (length - sent > 1400) ? 1400 : (length - sent);
        tcp_send_packet(conn, TCP_PSH | TCP_ACK, data + sent, chunk);
        sent += chunk;
        delay(10); // Small delay between chunks
    }
    
    return sent;
}

// Receive data from TCP
int tcp_receive_data(tcp_connection_t* conn, uint8_t* buffer, uint16_t buffer_size, uint32_t timeout_ms) {
    if (!conn || conn->state != TCP_ESTABLISHED) {
        print("TCP: Not connected\n");
        return -1;
    }
    
    print("TCP: Waiting for data...\n");
    
    uint32_t start = get_time_ms();
    while (get_time_ms() - start < timeout_ms) {
        if (conn->rx_complete || conn->rx_length > 0) {
            uint16_t copy_len = (conn->rx_length < buffer_size) ? conn->rx_length : buffer_size;
            memcpy(buffer, conn->rx_buffer, copy_len);
            printr("TCP: Received %d bytes total\n", copy_len);
            return copy_len;
        }
        delay(10);
    }
    
    print("TCP: Receive timeout\n");
    return 0;
}

// Close TCP connection
void tcp_close(tcp_connection_t* conn) {
    if (!conn || conn->state == TCP_CLOSED) {
        return;
    }
    
    print("TCP: Closing connection\n");
    
    if (conn->state == TCP_ESTABLISHED) {
        tcp_send_packet(conn, TCP_FIN | TCP_ACK, NULL, 0);
        conn->state = TCP_FIN_WAIT_1;
        
        // Wait for close (simplified)
        delay(1000);
    }
    
    conn->active = false;
    conn->state = TCP_CLOSED;
}

// Initialize TCP subsystem
void tcp_init(void) {
    for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
        tcp_connections[i].active = false;
        tcp_connections[i].state = TCP_CLOSED;
    }
    print("TCP initialized\n");
}

// Display all TCP connections
void tcp_dump_connections(void) {
    char buffer[64];
    print("\n=== TCP Connections ===\n");
    
    bool found = false;
    for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
        if (tcp_connections[i].active) {
            found = true;
            
            printr("Connection %d:\n", i);
            print("  Remote: ");
            ip_to_string(tcp_connections[i].remote_ip, buffer);
            print(buffer);
            printr(":%d\n", tcp_connections[i].remote_port);
            printr("  Local Port: %d\n", tcp_connections[i].local_port);
            print("  State: ");
            print(tcp_state_to_string_inline(tcp_connections[i].state));

            printr("\n  SEQ: %u, ACK: %u\n", 
                   tcp_connections[i].seq_num, tcp_connections[i].ack_num);
            printr("  RX Buffer: %d bytes\n", tcp_connections[i].rx_length);
        }
    }
    
    if (!found) {
        print("(no active connections)\n");
    }
    print("======================\n\n");
}

// Clean up stale connections
void tcp_cleanup_stale(void) {
    uint32_t current_time = get_time_ms();
    
    for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
        if (tcp_connections[i].active) {
            // Timeout after 60 seconds of inactivity
            if (current_time - tcp_connections[i].last_activity > 60000) {
                char buffer[32];
                print("TCP: Cleaning up stale connection to ");
                ip_to_string(tcp_connections[i].remote_ip, buffer);
                print(buffer);
                printr(":%d\n", tcp_connections[i].remote_port);
                
                tcp_connections[i].active = false;
                tcp_connections[i].state = TCP_CLOSED;
            }
        }
    }
}

// Convert state to string
const char* tcp_state_to_string(tcp_state_t state) {
    return tcp_state_to_string_inline(state);
}

// Add to netstack.h:
// #define IP_PROTOCOL_TCP 6
// void tcp_process(ipv4_header_t* ip_header, uint8_t* tcp_data, uint16_t length);

// Add to ipv4_process() in netstack.c:
// case IP_PROTOCOL_TCP:
//     tcp_process(ip_header, payload, payload_len);
//     break;