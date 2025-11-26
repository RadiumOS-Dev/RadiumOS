// network_receive.h - Network Receive Thread Header
#ifndef NETWORK_RECEIVE_H
#define NETWORK_RECEIVE_H

#include <stdint.h>
#include <stdbool.h>

// Captured packet structure
typedef struct {
    uint8_t data[1518];      // Maximum Ethernet frame size
    uint16_t length;         // Actual packet length
    uint32_t timestamp;      // Time when packet was captured (ms)
} captured_packet_t;

// Basic receive thread - processes all incoming packets
// Call this periodically (e.g., every 50ms) or in a dedicated task
void network_receive_thread(void);

// Continuous receive task - runs forever in a loop
// Use this as a kernel task: create_task(..., network_receive_task, ...)
void network_receive_task(uint32_t task_id);

// Get network statistics
// Parameters can be NULL if not needed
void network_get_stats(uint32_t* packets, uint32_t* bytes, 
                       uint32_t* dropped, uint32_t* last_time);

// Reset network statistics counters
void network_reset_stats(void);

// Print network statistics to terminal
void network_print_stats(void);

// Advanced receive with custom filter
// filter: callback function that returns true if packet should be processed
// If filter is NULL, all packets are processed
void network_receive_thread_filtered(bool (*filter)(uint8_t* packet, uint16_t length));

// Packet capture functions
void network_start_capture(void);
void network_stop_capture(void);
uint32_t network_get_captured_packets(captured_packet_t** packets);
void network_dump_captured_packets(void);

// Receive thread with capture support
void network_receive_thread_with_capture(void);

// Capture command for CLI
void network_capture_command(int argc, char* argv[]);

// Predefined packet filters
bool filter_only_arp(uint8_t* packet, uint16_t length);
bool filter_only_icmp(uint8_t* packet, uint16_t length);
bool filter_only_udp(uint8_t* packet, uint16_t length);

#endif // NETWORK_RECEIVE_H