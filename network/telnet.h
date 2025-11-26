// telnet.h - Telnet Client Implementation
#ifndef TELNET_H
#define TELNET_H

#include <stdint.h>
#include <stdbool.h>
// Telnet protocol constants
#define TELNET_PORT 23

// Telnet commands (IAC = Interpret As Command)
#define TELNET_IAC      255  // Interpret as command
#define TELNET_DONT     254  // Don't use option
#define TELNET_DO       253  // Use option
#define TELNET_WONT     252  // Won't use option
#define TELNET_WILL     251  // Will use option
#define TELNET_SB       250  // Subnegotiation begin
#define TELNET_GA       249  // Go ahead
#define TELNET_EL       248  // Erase line
#define TELNET_EC       247  // Erase character
#define TELNET_AYT      246  // Are you there
#define TELNET_AO       245  // Abort output
#define TELNET_IP       244  // Interrupt process
#define TELNET_BRK      243  // Break
#define TELNET_DM       242  // Data mark
#define TELNET_NOP      241  // No operation
#define TELNET_SE       240  // Subnegotiation end
#define TELNET_EOR      239  // End of record
#define TELNET_ABORT    238  // Abort
#define TELNET_SUSP     237  // Suspend
#define TELNET_EOF      236  // End of file

// Telnet options
#define TELNET_OPT_BINARY           0   // Binary transmission
#define TELNET_OPT_ECHO             1   // Echo
#define TELNET_OPT_SUPPRESS_GA      3   // Suppress go ahead
#define TELNET_OPT_STATUS           5   // Status
#define TELNET_OPT_TIMING_MARK      6   // Timing mark
#define TELNET_OPT_TERMINAL_TYPE    24  // Terminal type
#define TELNET_OPT_WINDOW_SIZE      31  // Window size (NAWS)
#define TELNET_OPT_TERMINAL_SPEED   32  // Terminal speed
#define TELNET_OPT_LINEMODE         34  // Line mode
#define TELNET_OPT_ENVIRON          36  // Environment variables

// Connection states
#define TELNET_STATE_CLOSED         0
#define TELNET_STATE_CONNECTING     1
#define TELNET_STATE_CONNECTED      2
#define TELNET_STATE_CLOSING        3

// Buffer sizes
#define TELNET_RX_BUFFER_SIZE   4096
#define TELNET_TX_BUFFER_SIZE   2048
#define TELNET_INPUT_BUFFER_SIZE 256

// Telnet option negotiation state
typedef struct {
    bool local_enabled;   // We are using this option
    bool remote_enabled;  // Remote is using this option
} telnet_option_state_t;

// Telnet client connection
typedef struct {
    // Connection state
    uint8_t state;
    uint32_t remote_ip;
    uint16_t remote_port;
    uint16_t local_port;
    
    // Buffers
    uint8_t rx_buffer[TELNET_RX_BUFFER_SIZE];
    uint16_t rx_write_pos;
    uint16_t rx_read_pos;
    
    uint8_t tx_buffer[TELNET_TX_BUFFER_SIZE];
    uint16_t tx_length;
    
    // Input line buffer (what user is typing)
    char input_buffer[TELNET_INPUT_BUFFER_SIZE];
    uint16_t input_pos;
    
    // Option states
    telnet_option_state_t options[256];
    
    // IAC processing state
    bool in_iac;
    uint8_t iac_command;
    bool in_subnegotiation;
    uint8_t subneg_buffer[256];
    uint16_t subneg_pos;
    
    // Terminal settings
    char terminal_type[32];
    uint16_t terminal_width;
    uint16_t terminal_height;
    
    // Timing
    uint32_t last_activity;
    uint32_t connect_time;
    uint32_t last_keepalive;
    
    // Display state
    bool local_echo;        // Should we echo typed characters locally
    bool escape_mode;       // In escape sequence processing
    
} telnet_client_t;

// Global telnet client instance
extern telnet_client_t* telnet_client;

// API Functions

// Initialize telnet client
void telnet_init(void);

// Connect to remote telnet server
bool telnet_connect(const char* host_ip, uint16_t port);

// Disconnect
void telnet_disconnect(void);

// Check if connected
bool telnet_is_connected(void);

// Send data to server
void telnet_send(const char* data, uint16_t length);
void telnet_send_char(char c);

// Process incoming data (called by network stack)
void telnet_process_packet(uint32_t src_ip, uint16_t src_port, uint8_t* data, uint16_t length);

// Process keyboard input (called by main loop)
void telnet_process_input(char key);

// Update function (called periodically)
void telnet_update(void);

// Option negotiation
void telnet_send_will(uint8_t option);
void telnet_send_wont(uint8_t option);
void telnet_send_do(uint8_t option);
void telnet_send_dont(uint8_t option);

// Special commands
void telnet_send_terminal_type(void);
void telnet_send_window_size(void);

// Display functions
void telnet_display_status(void);

#endif // TELNET_H