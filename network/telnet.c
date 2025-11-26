// telnet.c - Telnet Client Implementation
#include "telnet.h"
#include "netstack.h"
#include "../terminal/terminal.h"
#include "../utility/utility.h"
#include "../timers/timer.h"
#include "../keyboard/keyboard.h"

// Debug flag
#define TELNET_DEBUG 1

// Global telnet client instance
static telnet_client_t telnet_client_storage;
telnet_client_t* telnet_client = NULL;

void telnet_init(void) {
    if (telnet_client) {
        // Already initialized
        return;
    }
    
    // Use static allocation instead of malloc
    telnet_client = &telnet_client_storage;
    
    memset(telnet_client, 0, sizeof(telnet_client_t));
    telnet_client->state = TELNET_STATE_CLOSED;
    telnet_client->terminal_width = 80;
    telnet_client->terminal_height = 25;
    telnet_client->local_echo = true;
    strcpy(telnet_client->terminal_type, "vt100");
    
    print("Telnet client initialized\n");
}

// Connect to remote telnet server
bool telnet_connect(const char* host_ip, uint16_t port) {
    if (!telnet_client) {
        print("ERROR: Telnet client not initialized\n");
        return false;
    }
    
    if (telnet_client->state != TELNET_STATE_CLOSED) {
        print("ERROR: Already connected\n");
        return false;
    }
    
    // Parse IP address
    telnet_client->remote_ip = ip_parse(host_ip);
    telnet_client->remote_port = port;
    telnet_client->local_port = 10000 + (get_time_ms() % 10000);
    
    // Clear buffers
    telnet_client->rx_write_pos = 0;
    telnet_client->rx_read_pos = 0;
    telnet_client->tx_length = 0;
    telnet_client->input_pos = 0;
    
    // Clear IAC state
    telnet_client->in_iac = false;
    telnet_client->iac_command = 0;
    telnet_client->in_subnegotiation = false;
    telnet_client->subneg_pos = 0;
    
    // Clear options
    for (int i = 0; i < 256; i++) {
        telnet_client->options[i].local_enabled = false;
        telnet_client->options[i].remote_enabled = false;
    }
    
    telnet_client->state = TELNET_STATE_CONNECTING;
    telnet_client->connect_time = get_time_ms();
    telnet_client->last_activity = get_time_ms();
    telnet_client->last_keepalive = get_time_ms();
    
    print("\nConnecting to ");
    print(host_ip);
    print(":");
    char buffer[16];
    itoa(port, buffer, 10);
    print(buffer);
    print("...\n");
    
    // Send initial connection packet (with UDP, we just send empty packet to establish connection)
    uint8_t hello[] = "\r\n";
    udp_send(telnet_client->remote_ip, telnet_client->local_port, 
             telnet_client->remote_port, hello, 2);
    
    telnet_client->state = TELNET_STATE_CONNECTED;
    
    print("Connected! Type 'Ctrl+D' or type 'quit' to disconnect.\n");
    print("========================================\n\n");
    
    return true;
}

// Disconnect
void telnet_disconnect(void) {
    if (!telnet_client || telnet_client->state == TELNET_STATE_CLOSED) {
        return;
    }
    
    if (TELNET_DEBUG) {
        print("\n\nDisconnecting...\n");
    }
    
    // Send goodbye (if connected)
    if (telnet_client->state == TELNET_STATE_CONNECTED) {
        telnet_send("quit\r\n", 6);
    }
    
    telnet_client->state = TELNET_STATE_CLOSED;
    
    print("Disconnected.\n");
}

// Check if connected
bool telnet_is_connected(void) {
    return (telnet_client && telnet_client->state == TELNET_STATE_CONNECTED);
}

// Send raw data to server
void telnet_send(const char* data, uint16_t length) {
    if (!telnet_client || telnet_client->state != TELNET_STATE_CONNECTED) {
        return;
    }
    
    // Send via UDP (or TCP when available)
    udp_send(telnet_client->remote_ip, telnet_client->local_port,
             telnet_client->remote_port, (uint8_t*)data, length);
    
    telnet_client->last_activity = get_time_ms();
}

// Send single character
void telnet_send_char(char c) {
    telnet_send(&c, 1);
}

// Send IAC command
static void telnet_send_iac(uint8_t command, uint8_t option) {
    uint8_t buffer[3];
    buffer[0] = TELNET_IAC;
    buffer[1] = command;
    buffer[2] = option;
    
    telnet_send((char*)buffer, 3);
    
    if (TELNET_DEBUG) {
        char buf[64];
        print("[IAC ");
        switch(command) {
            case TELNET_WILL: print("WILL"); break;
            case TELNET_WONT: print("WONT"); break;
            case TELNET_DO:   print("DO"); break;
            case TELNET_DONT: print("DONT"); break;
            default: 
                itoa(command, buf, 10);
                print(buf);
        }
        print(" ");
        itoa(option, buf, 10);
        print(buf);
        print("]\n");
    }
}

// Send WILL option
void telnet_send_will(uint8_t option) {
    telnet_send_iac(TELNET_WILL, option);
    telnet_client->options[option].local_enabled = true;
}

// Send WONT option
void telnet_send_wont(uint8_t option) {
    telnet_send_iac(TELNET_WONT, option);
    telnet_client->options[option].local_enabled = false;
}

// Send DO option
void telnet_send_do(uint8_t option) {
    telnet_send_iac(TELNET_DO, option);
}

// Send DONT option
void telnet_send_dont(uint8_t option) {
    telnet_send_iac(TELNET_DONT, option);
}

// Send terminal type
void telnet_send_terminal_type(void) {
    uint8_t buffer[64];
    buffer[0] = TELNET_IAC;
    buffer[1] = TELNET_SB;
    buffer[2] = TELNET_OPT_TERMINAL_TYPE;
    buffer[3] = 0; // IS
    
    uint16_t pos = 4;
    for (int i = 0; telnet_client->terminal_type[i] && i < 32; i++) {
        buffer[pos++] = telnet_client->terminal_type[i];
    }
    
    buffer[pos++] = TELNET_IAC;
    buffer[pos++] = TELNET_SE;
    
    telnet_send((char*)buffer, pos);
}

// Send window size (NAWS)
void telnet_send_window_size(void) {
    uint8_t buffer[9];
    buffer[0] = TELNET_IAC;
    buffer[1] = TELNET_SB;
    buffer[2] = TELNET_OPT_WINDOW_SIZE;
    buffer[3] = (telnet_client->terminal_width >> 8) & 0xFF;
    buffer[4] = telnet_client->terminal_width & 0xFF;
    buffer[5] = (telnet_client->terminal_height >> 8) & 0xFF;
    buffer[6] = telnet_client->terminal_height & 0xFF;
    buffer[7] = TELNET_IAC;
    buffer[8] = TELNET_SE;
    
    telnet_send((char*)buffer, 9);
}

// Process IAC command
static void telnet_process_iac(uint8_t command, uint8_t option) {
    if (TELNET_DEBUG) {
        char buf[64];
        print("[RX IAC ");
        switch(command) {
            case TELNET_WILL: print("WILL"); break;
            case TELNET_WONT: print("WONT"); break;
            case TELNET_DO:   print("DO"); break;
            case TELNET_DONT: print("DONT"); break;
            default: 
                itoa(command, buf, 10);
                print(buf);
        }
        print(" ");
        itoa(option, buf, 10);
        print(buf);
        print("]\n");
    }
    
    switch (command) {
        case TELNET_WILL:
            // Server will use option
            if (option == TELNET_OPT_ECHO) {
                // Server will echo - disable local echo
                telnet_client->local_echo = false;
                telnet_send_do(option);
                telnet_client->options[option].remote_enabled = true;
            } else if (option == TELNET_OPT_SUPPRESS_GA) {
                telnet_send_do(option);
                telnet_client->options[option].remote_enabled = true;
            } else {
                // Don't want other options
                telnet_send_dont(option);
            }
            break;
            
        case TELNET_WONT:
            // Server won't use option
            if (option == TELNET_OPT_ECHO) {
                telnet_client->local_echo = true;
            }
            telnet_client->options[option].remote_enabled = false;
            break;
            
        case TELNET_DO:
            // Server wants us to use option
            if (option == TELNET_OPT_TERMINAL_TYPE) {
                telnet_send_will(option);
                telnet_send_terminal_type();
            } else if (option == TELNET_OPT_WINDOW_SIZE) {
                telnet_send_will(option);
                telnet_send_window_size();
            } else if (option == TELNET_OPT_SUPPRESS_GA) {
                telnet_send_will(option);
            } else {
                // We won't do other options
                telnet_send_wont(option);
            }
            break;
            
        case TELNET_DONT:
            // Server doesn't want us to use option
            telnet_send_wont(option);
            telnet_client->options[option].local_enabled = false;
            break;
            
        case TELNET_AYT:
            // Are You There
            telnet_send("\r\n[Yes]\r\n", 9);
            break;
            
        case TELNET_NOP:
            // No operation
            break;
            
        default:
            break;
    }
}

// Process incoming packet
void telnet_process_packet(uint32_t src_ip, uint16_t src_port, uint8_t* data, uint16_t length) {
    if (!telnet_client || telnet_client->state != TELNET_STATE_CONNECTED) {
        return;
    }
    
    // Verify packet is from our server
    if (src_ip != telnet_client->remote_ip || src_port != telnet_client->remote_port) {
        return;
    }
    
    telnet_client->last_activity = get_time_ms();
    
    // Process each byte
    for (uint16_t i = 0; i < length; i++) {
        uint8_t byte = data[i];
        
        // Handle IAC processing
        if (telnet_client->in_iac) {
            if (telnet_client->in_subnegotiation) {
                // In subnegotiation
                if (byte == TELNET_IAC) {
                    // Might be end of subneg or escaped IAC
                    if (i + 1 < length && data[i + 1] == TELNET_SE) {
                        // End of subnegotiation
                        telnet_client->in_subnegotiation = false;
                        telnet_client->in_iac = false;
                        i++; // Skip SE
                        continue;
                    } else if (i + 1 < length && data[i + 1] == TELNET_IAC) {
                        // Escaped IAC in subneg
                        if (telnet_client->subneg_pos < 255) {
                            telnet_client->subneg_buffer[telnet_client->subneg_pos++] = TELNET_IAC;
                        }
                        i++; // Skip second IAC
                        continue;
                    }
                }
                
                // Add to subneg buffer
                if (telnet_client->subneg_pos < 255) {
                    telnet_client->subneg_buffer[telnet_client->subneg_pos++] = byte;
                }
                continue;
            }
            
            if (telnet_client->iac_command == 0) {
                // This is the command byte
                telnet_client->iac_command = byte;
                
                if (byte == TELNET_IAC) {
                    // Escaped IAC - output literal 255
                    terminal_putchar(255);
                    telnet_client->in_iac = false;
                    telnet_client->iac_command = 0;
                } else if (byte == TELNET_SB) {
                    // Start subnegotiation
                    telnet_client->in_subnegotiation = true;
                    telnet_client->subneg_pos = 0;
                } else if (byte == TELNET_AYT || byte == TELNET_NOP || 
                          byte == TELNET_GA || byte == TELNET_BRK) {
                    // Commands without option byte
                    telnet_process_iac(byte, 0);
                    telnet_client->in_iac = false;
                    telnet_client->iac_command = 0;
                }
                continue;
            } else {
                // This is the option byte
                telnet_process_iac(telnet_client->iac_command, byte);
                telnet_client->in_iac = false;
                telnet_client->iac_command = 0;
                continue;
            }
        }
        
        // Check for IAC start
        if (byte == TELNET_IAC) {
            telnet_client->in_iac = true;
            telnet_client->iac_command = 0;
            continue;
        }
        
        // Regular character - display it
        if (byte == '\r') {
            // Carriage return - might be followed by \n
            continue;
        } else if (byte == '\n') {
            terminal_putchar('\n');
        } else if (byte == '\b' || byte == 0x7F) {
            // Backspace
            terminal_putchar('\b');
            terminal_putchar(' ');
            terminal_putchar('\b');
        } else if (byte >= 32 && byte < 127) {
            // Printable character
            terminal_putchar(byte);
        } else if (byte == 0x1B) {
            // Escape - start of ANSI sequence
            telnet_client->escape_mode = true;
            terminal_putchar(byte);
        } else if (telnet_client->escape_mode) {
            // Part of ANSI escape sequence
            terminal_putchar(byte);
            if ((byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z')) {
                telnet_client->escape_mode = false;
            }
        }
    }
}

// Process keyboard input
void telnet_process_input(char key) {
    if (!telnet_client || telnet_client->state != TELNET_STATE_CONNECTED) {
        return;
    }
    
    // Special keys
    if (key == 0x04) {
        // Ctrl+D - disconnect
        telnet_disconnect();
        return;
    }
    
    if (key == '\b' || key == 0x7F) {
        // Backspace
        if (telnet_client->input_pos > 0) {
            telnet_client->input_pos--;
            
            // Echo backspace locally if needed
            if (telnet_client->local_echo) {
                terminal_putchar('\b');
                terminal_putchar(' ');
                terminal_putchar('\b');
            }
            
            // Send backspace to server
            telnet_send_char('\b');
        }
        return;
    }
    
    if (key == '\n' || key == '\r') {
        // Enter - send line
        telnet_client->input_buffer[telnet_client->input_pos] = '\0';
        
        // Echo locally if needed
        if (telnet_client->local_echo) {
            terminal_putchar('\n');
        }
        
        // Check for local commands
        if (strcmp(telnet_client->input_buffer, "quit") == 0 ||
            strcmp(telnet_client->input_buffer, "exit") == 0) {
            telnet_disconnect();
            return;
        }
        
        // Send to server
        telnet_send(telnet_client->input_buffer, telnet_client->input_pos);
        telnet_send("\r\n", 2);
        
        // Clear input buffer
        telnet_client->input_pos = 0;
        return;
    }
    
    if (key == '\t') {
        // Tab - send to server
        telnet_send_char('\t');
        
        if (telnet_client->local_echo) {
            print("    "); // 4 spaces
        }
        return;
    }
    
    // Regular character
    if (key >= 32 && key < 127) {
        if (telnet_client->input_pos < TELNET_INPUT_BUFFER_SIZE - 1) {
            telnet_client->input_buffer[telnet_client->input_pos++] = key;
            
            // Echo locally if needed
            if (telnet_client->local_echo) {
                terminal_putchar(key);
            }
            
            // Send to server
            telnet_send_char(key);
        }
    }
}

// Update function (keepalive, timeouts, etc.)
void telnet_update(void) {
    if (!telnet_client || telnet_client->state != TELNET_STATE_CONNECTED) {
        return;
    }
    
    uint32_t now = get_time_ms();
    
    // Send keepalive every 30 seconds
    if (now - telnet_client->last_keepalive > 30000) {
        telnet_send_iac(TELNET_NOP, 0);
        telnet_client->last_keepalive = now;
    }
    
    // Timeout after 5 minutes of inactivity
    if (now - telnet_client->last_activity > 300000) {
        print("\n\nConnection timeout (5 minutes inactivity)\n");
        telnet_disconnect();
    }
}

// Display connection status
void telnet_display_status(void) {
    if (!telnet_client) {
        print("Telnet client not initialized\n");
        return;
    }
    
    print("\n=== Telnet Client Status ===\n");
    
    print("State: ");
    switch (telnet_client->state) {
        case TELNET_STATE_CLOSED:
            terminal_setcolor(VGA_COLOR_RED);
            print("Disconnected");
            break;
        case TELNET_STATE_CONNECTING:
            terminal_setcolor(VGA_COLOR_LIGHT_BROWN);
            print("Connecting");
            break;
        case TELNET_STATE_CONNECTED:
            terminal_setcolor(VGA_COLOR_GREEN);
            print("Connected");
            break;
        case TELNET_STATE_CLOSING:
            terminal_setcolor(VGA_COLOR_LIGHT_BROWN);
            print("Closing");
            break;
    }
    terminal_setcolor(VGA_COLOR_WHITE);
    print("\n");
    
    if (telnet_client->state != TELNET_STATE_CLOSED) {
        char buffer[64];
        
        print("Remote: ");
        ip_to_string(telnet_client->remote_ip, buffer);
        print(buffer);
        print(":");
        itoa(telnet_client->remote_port, buffer, 10);
        print(buffer);
        print("\n");
        
        print("Local Port: ");
        itoa(telnet_client->local_port, buffer, 10);
        print(buffer);
        print("\n");
        
        uint32_t duration = (get_time_ms() - telnet_client->connect_time) / 1000;
        print("Connected: ");
        itoa(duration, buffer, 10);
        print(buffer);
        print(" seconds\n");
        
        print("Local Echo: ");
        print(telnet_client->local_echo ? "ON" : "OFF");
        print("\n");
        
        print("Terminal: ");
        print(telnet_client->terminal_type);
        print(" (");
        itoa(telnet_client->terminal_width, buffer, 10);
        print(buffer);
        print("x");
        itoa(telnet_client->terminal_height, buffer, 10);
        print(buffer);
        print(")\n");
    }
    
    print("============================\n\n");
}