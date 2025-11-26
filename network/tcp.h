#ifndef TCP_H
#define TCP_H

#include <stdint.h>
#include <stdbool.h>
#include "netstack.h"

// TCP Header Structure
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
#define TCP_FIN 0x01  // Finish - no more data
#define TCP_SYN 0x02  // Synchronize - establish connection
#define TCP_RST 0x04  // Reset - abort connection
#define TCP_PSH 0x08  // Push - send data immediately
#define TCP_ACK 0x10  // Acknowledgment
#define TCP_URG 0x20  // Urgent pointer field valid
#define TCP_ECE 0x40  // ECN Echo
#define TCP_CWR 0x80  // Congestion Window Reduced

// TCP Connection States
typedef enum {
    TCP_CLOSED,       // No connection
    TCP_LISTEN,       // Server waiting for connection
    TCP_SYN_SENT,     // SYN sent, waiting for SYN-ACK
    TCP_SYN_RECEIVED, // SYN received, SYN-ACK sent
    TCP_ESTABLISHED,  // Connection established
    TCP_FIN_WAIT_1,   // FIN sent, waiting for ACK
    TCP_FIN_WAIT_2,   // FIN ACKed, waiting for remote FIN
    TCP_CLOSING,      // Both sides closing simultaneously
    TCP_TIME_WAIT,    // Waiting for final packets
    TCP_CLOSE_WAIT,   // Remote closed, local can still send
    TCP_LAST_ACK      // Waiting for final ACK
} tcp_state_t;

// TCP Connection Structure
typedef struct {
    uint32_t remote_ip;       // Remote IP address
    uint16_t local_port;      // Local port number
    uint16_t remote_port;     // Remote port number
    tcp_state_t state;        // Connection state
    uint32_t seq_num;         // Our sequence number
    uint32_t ack_num;         // Acknowledgment number
    uint16_t window_size;     // Receive window size
    bool active;              // Is this connection active?
    uint32_t last_activity;   // Timestamp of last activity
    
    // Receive buffer
    uint8_t rx_buffer[8192];  // Received data buffer
    uint16_t rx_length;       // Length of received data
    bool rx_complete;         // Has connection been closed?
} tcp_connection_t;

// Maximum number of simultaneous TCP connections
#define MAX_TCP_CONNECTIONS 4

// TCP Functions

/**
 * Initialize TCP subsystem
 */
void tcp_init(void);

/**
 * Process incoming TCP packet
 * Called by IPv4 layer when TCP packet is received
 * 
 * @param ip_header Pointer to IPv4 header
 * @param tcp_data Pointer to TCP data (header + payload)
 * @param length Length of TCP data
 */
void tcp_process(ipv4_header_t* ip_header, uint8_t* tcp_data, uint16_t length);

/**
 * Connect to remote host
 * Initiates TCP three-way handshake
 * 
 * @param remote_ip Remote IP address
 * @param remote_port Remote port number
 * @param local_port Local port number to use
 * @return Pointer to connection on success, NULL on failure
 */
tcp_connection_t* tcp_connect(uint32_t remote_ip, uint16_t remote_port, uint16_t local_port);

/**
 * Send data over established TCP connection
 * 
 * @param conn Pointer to TCP connection
 * @param data Pointer to data to send
 * @param length Length of data
 * @return Number of bytes sent, or -1 on error
 */
int tcp_send_data(tcp_connection_t* conn, uint8_t* data, uint16_t length);

/**
 * Receive data from TCP connection
 * Blocks until data is received or timeout occurs
 * 
 * @param conn Pointer to TCP connection
 * @param buffer Buffer to store received data
 * @param buffer_size Size of buffer
 * @param timeout_ms Timeout in milliseconds
 * @return Number of bytes received, 0 on timeout, -1 on error
 */
int tcp_receive_data(tcp_connection_t* conn, uint8_t* buffer, uint16_t buffer_size, uint32_t timeout_ms);

/**
 * Close TCP connection
 * Initiates graceful connection close
 * 
 * @param conn Pointer to TCP connection
 */
void tcp_close(tcp_connection_t* conn);

/**
 * Get TCP connection by ports
 * Internal function to find or create connection
 * 
 * @param remote_ip Remote IP address
 * @param local_port Local port
 * @param remote_port Remote port
 * @return Pointer to connection, or NULL if no slots available
 */
tcp_connection_t* tcp_get_connection(uint32_t remote_ip, uint16_t local_port, uint16_t remote_port);

/**
 * Send TCP packet
 * Internal function to construct and send TCP packet
 * 
 * @param conn Pointer to connection
 * @param flags TCP flags (SYN, ACK, FIN, etc.)
 * @param data Payload data (can be NULL)
 * @param data_length Length of payload
 */
void tcp_send_packet(tcp_connection_t* conn, uint8_t flags, uint8_t* data, uint16_t data_length);

/**
 * Calculate TCP checksum
 * Includes pseudo-header as per TCP specification
 * 
 * @param src_ip Source IP address
 * @param dest_ip Destination IP address
 * @param tcp Pointer to TCP header
 * @param tcp_length Length of TCP header
 * @param data Pointer to data
 * @param data_length Length of data
 * @return Calculated checksum
 */
uint16_t tcp_checksum(uint32_t src_ip, uint32_t dest_ip, tcp_header_t* tcp, 
                      uint16_t tcp_length, uint8_t* data, uint16_t data_length);

/**
 * Display TCP connection status
 * Debug function to show all active connections
 */
void tcp_dump_connections(void);

/**
 * Clean up stale connections
 * Closes connections that have been inactive for too long
 */
void tcp_cleanup_stale(void);

/**
 * Convert TCP state to string
 * 
 * @param state TCP state enum
 * @return String representation of state
 */
const char* tcp_state_to_string(tcp_state_t state);



#endif // TCP_H