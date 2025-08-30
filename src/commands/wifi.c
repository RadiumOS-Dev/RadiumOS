#include "wifi.h"
#include "../terminal/terminal.h"
#include "../wifi/wifi.h"
#include "../utility/utility.h"
#include "../timers/timer.h"

void wifi_command(int argc, char* argv[]) {
    if (argc < 2) {
        wifi_show_help();
        return;
    }
    
    char* subcommand = argv[1];
    
    if (strcmp(subcommand, "help") == 0) {
        wifi_show_help();
    }
    else if (strcmp(subcommand, "status") == 0) {
        wifi_show_status();
    }
    else if (strcmp(subcommand, "scan") == 0) {
        wifi_scan_networks();
    }
    else if (strcmp(subcommand, "connect") == 0) {
        if (argc < 3) {
            print("Error: SSID required for connect command\n");
            print("Usage: wifi connect <SSID> [password]\n");
            return;
        }
        
        char* ssid = argv[2];
        char* password = (argc >= 4) ? argv[3] : NULL;
        wifi_connect_network(ssid, password);
    }
    else if (strcmp(subcommand, "disconnect") == 0) {
        wifi_disconnect_network();
    }
    else if (strcmp(subcommand, "info") == 0) {
        wifi_show_info();
    }
    else if (strcmp(subcommand, "enable") == 0) {
        wifi_enable_device();
    }
    else if (strcmp(subcommand, "disable") == 0) {
        wifi_disable_device();
    }
    else if (strcmp(subcommand, "init") == 0) {
        print("Initializing WiFi subsystem...\n");
        wifi_init();
    }
    else if (strcmp(subcommand, "reset") == 0) {
        if (!g_wifi_device) {
            terminal_setcolor(VGA_COLOR_RED);
            print("Error: No WiFi device found\n");
            terminal_setcolor(VGA_COLOR_LIGHT_GREY);
            return;
        }
        
        print("Resetting WiFi device...\n");
        if (iwl_prepare_card_hw(g_wifi_device) == 0) {
            terminal_setcolor(VGA_COLOR_GREEN);
            print("WiFi device reset successful\n");
            terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        } else {
            terminal_setcolor(VGA_COLOR_RED);
            print("WiFi device reset failed\n");
            terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        }
    }
    else if (strcmp(subcommand, "debug") == 0) {
        if (!g_wifi_device) {
            terminal_setcolor(VGA_COLOR_RED);
            print("Error: No WiFi device found\n");
            terminal_setcolor(VGA_COLOR_LIGHT_GREY);
            return;
        }
        
        print("=== WiFi Debug Information ===\n");
        print("Device ID: 0x");
        print_hex(g_wifi_device->device_id);
        print("\nVendor ID: 0x");
        print_hex(g_wifi_device->vendor_id);
        print("\nHW Revision: 0x");
        print_hex(g_wifi_device->hw_rev);
        print("\nPCI Location: ");
        print_hex_byte(g_wifi_device->bus);
        print(":");
        print_hex_byte(g_wifi_device->device);
        print(".");
        print_hex_byte(g_wifi_device->function);
        print("\nHW Base Address: 0x");
        print_hex(g_wifi_device->hw_phys_addr);
        print("\nInitialized: ");
        print(g_wifi_device->initialized ? "Yes" : "No");
        print("\nEnabled: ");
        print(g_wifi_device->enabled ? "Yes" : "No");
        print("\nuCode Loaded: ");
        print(g_wifi_device->ucode_loaded ? "Yes" : "No");
        print("\n");
    }
    else {
        terminal_setcolor(VGA_COLOR_RED);
        print("Error: Unknown WiFi command '");
        print(subcommand);
        print("'\n");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        wifi_show_help();
    }
}

void wifi_show_help(void) {
    print("WiFi Command Usage:\n");
    print("  wifi help          - Show this help message\n");
    print("  wifi init          - Initialize WiFi subsystem\n");
    print("  wifi status        - Show WiFi device status\n");
    print("  wifi scan          - Scan for available networks\n");
    print("  wifi connect <SSID> [password] - Connect to network\n");
    print("  wifi disconnect    - Disconnect from current network\n");
    print("  wifi info          - Show detailed device information\n");
    print("  wifi enable        - Enable WiFi device\n");
    print("  wifi disable       - Disable WiFi device\n");
    print("  wifi reset         - Reset WiFi device\n");
    print("  wifi debug         - Show debug information\n");
    print("\nExamples:\n");
    print("  wifi scan\n");
    print("  wifi connect MyNetwork mypassword\n");
    print("  wifi connect OpenNetwork\n");
}

void wifi_show_status(void) {
    if (!g_wifi_device) {
        terminal_setcolor(VGA_COLOR_RED);
        print("WiFi Status: Not initialized\n");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        print("Run 'wifi init' to initialize the WiFi subsystem\n");
        return;
    }
    
    print("=== WiFi Status ===\n");
    
    // Device state
    print("Device State: ");
    switch (g_wifi_device->state) {
        case WIFI_STATE_UNINITIALIZED:
            terminal_setcolor(VGA_COLOR_RED);
            print("Uninitialized");
            break;
        case WIFI_STATE_INITIALIZING:
            terminal_setcolor(VGA_COLOR_LIGHT_BROWN);
            print("Initializing");
            break;
        case WIFI_STATE_READY:
            terminal_setcolor(VGA_COLOR_GREEN);
            print("Ready");
            break;
        case WIFI_STATE_SCANNING:
            terminal_setcolor(VGA_COLOR_CYAN);
            print("Scanning");
            break;
        case WIFI_STATE_CONNECTING:
            terminal_setcolor(VGA_COLOR_LIGHT_BROWN);
            print("Connecting");
            break;
        case WIFI_STATE_CONNECTED:
            terminal_setcolor(VGA_COLOR_GREEN);
            print("Connected");
            break;
        case WIFI_STATE_ERROR:
            terminal_setcolor(VGA_COLOR_RED);
            print("Error");
            break;
        default:
            terminal_setcolor(VGA_COLOR_RED);
            print("Unknown");
            break;
    }
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    print("\n");
    
    // Device enabled/disabled
    print("Device: ");
    if (g_wifi_device->enabled) {
        terminal_setcolor(VGA_COLOR_GREEN);
        print("Enabled");
    } else {
        terminal_setcolor(VGA_COLOR_RED);
        print("Disabled");
    }
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    print("\n");
    
    // MAC Address
    print("MAC Address: ");
    for (int i = 0; i < 6; i++) {
        print_hex_byte(g_wifi_device->mac_address[i]);
        if (i < 5) print(":");
    }
    print("\n");
    
    // Statistics
    print("TX Packets: ");
    print_uint(g_wifi_device->tx_packets);
    print("\nRX Packets: ");
    print_uint(g_wifi_device->rx_packets);
    print("\nTX Errors: ");
    print_uint(g_wifi_device->tx_errors);
    print("\nRX Errors: ");
    print_uint(g_wifi_device->rx_errors);
    print("\n");
}

void wifi_scan_networks(void) {
    if (!g_wifi_device) {
        terminal_setcolor(VGA_COLOR_RED);
        print("Error: WiFi not initialized\n");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        return;
    }
    
    if (!g_wifi_device->enabled) {
        terminal_setcolor(VGA_COLOR_RED);
        print("Error: WiFi device is disabled\n");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        print("Run 'wifi enable' first\n");
        return;
    }
    
    if (!g_wifi_device->ucode_loaded) {
        terminal_setcolor(VGA_COLOR_RED);
        print("Error: WiFi firmware not loaded\n");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        print("Cannot scan without firmware\n");
        return;
    }
    
    print("Starting WiFi scan...\n");
    
    // Set device state to scanning
    g_wifi_device->state = WIFI_STATE_SCANNING;
    
    // Send scan command to hardware
    iwl_cmd_t scan_cmd;
    scan_cmd.cmd = REPLY_SPECTRUM_MEASUREMENT_CMD;
    scan_cmd.flags = 0;
    scan_cmd.len = 0;
    
    if (iwl_send_cmd_sync(g_wifi_device, &scan_cmd) != 0) {
        terminal_setcolor(VGA_COLOR_RED);
        print("Error: Failed to send scan command to hardware\n");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        g_wifi_device->state = WIFI_STATE_READY;
        return;
    }
    
    print("Scan command sent to hardware\n");
    print("Waiting for scan results...\n");
    
    // Wait for scan completion (in a real implementation, this would be interrupt-driven)
    delay(2000);
    
    print("Scan complete - check hardware for results\n");
    print("Note: Actual scan results require firmware and proper interrupt handling\n");
    
    // Reset device state
    g_wifi_device->state = WIFI_STATE_READY;
}

void wifi_connect_network(const char* ssid, const char* password) {
    if (!g_wifi_device) {
        terminal_setcolor(VGA_COLOR_RED);
        print("Error: WiFi not initialized\n");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        return;
    }
    
    if (!g_wifi_device->enabled) {
        terminal_setcolor(VGA_COLOR_RED);
        print("Error: WiFi device is disabled\n");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        return;
    }
    
    if (!g_wifi_device->ucode_loaded) {
        terminal_setcolor(VGA_COLOR_RED);
        print("Error: WiFi firmware not loaded\n");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        return;
    }
    
    print("Connecting to network: ");
    terminal_setcolor(VGA_COLOR_CYAN);
    print(ssid);
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    print("\n");
    
    // Set device state
    g_wifi_device->state = WIFI_STATE_CONNECTING;
    
    // Send connection command to hardware
    iwl_cmd_t connect_cmd;
    connect_cmd.cmd = REPLY_RXON;
    connect_cmd.flags = 0;
    connect_cmd.len = strlen(ssid) + (password ? strlen(password) : 0);
    
    if (iwl_send_cmd_sync(g_wifi_device, &connect_cmd) != 0) {
        terminal_setcolor(VGA_COLOR_RED);
        print("Error: Failed to send connect command to hardware\n");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        g_wifi_device->state = WIFI_STATE_READY;
        return;
    }
    
    if (password) {
        print("Using password authentication\n");
    } else {
        print("Connecting to open network\n");
    }
    
    print("Connection command sent to hardware\n");
    print("Note: Actual connection requires firmware and proper authentication\n");
    
    // For demonstration, assume connection succeeds if command was sent
    g_wifi_device->state = WIFI_STATE_READY;
}

void wifi_disconnect_network(void) {
    if (!g_wifi_device) {
        terminal_setcolor(VGA_COLOR_RED);
        print("Error: WiFi not initialized\n");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        return;
    }
    
    print("Sending disconnect command to hardware...\n");
    
    // Send disconnect command to hardware
    iwl_cmd_t disconnect_cmd;
        disconnect_cmd.cmd = REPLY_RXON;
    disconnect_cmd.flags = 0x01; // Disconnect flag
    disconnect_cmd.len = 0;
    
    if (iwl_send_cmd_sync(g_wifi_device, &disconnect_cmd) != 0) {
        terminal_setcolor(VGA_COLOR_RED);
        print("Error: Failed to send disconnect command to hardware\n");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        return;
    }
    
    // Update device state
    g_wifi_device->state = WIFI_STATE_READY;
    
    terminal_setcolor(VGA_COLOR_GREEN);
    print("Disconnect command sent to hardware\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
}

void wifi_show_info(void) {
    if (!g_wifi_device) {
        terminal_setcolor(VGA_COLOR_RED);
        print("Error: WiFi not initialized\n");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        return;
    }
    
    print("=== WiFi Device Information ===\n");
    
    // Device identification
    print("Device: Intel Dual Band Wireless\n");
    print("Device ID: 0x");
    print_hex(g_wifi_device->device_id);
    print("\nVendor ID: 0x");
    print_hex(g_wifi_device->vendor_id);
    print("\n");
    
    // Hardware info
    print("Hardware Revision: 0x");
    print_hex(g_wifi_device->hw_rev);
    print("\nPCI Location: ");
    print_hex_byte(g_wifi_device->bus);
    print(":");
    print_hex_byte(g_wifi_device->device);
    print(".");
    print_hex_byte(g_wifi_device->function);
    print("\n");
    
    // Memory info
    print("Hardware Base: 0x");
    print_hex(g_wifi_device->hw_phys_addr);
    print("\nMemory Size: ");
    print_uint(g_wifi_device->hw_len);
    print(" bytes\n");
    
    // EEPROM info
    if (g_wifi_device->eeprom_data) {
        print("EEPROM Size: ");
        print_uint(g_wifi_device->eeprom_size);
        print(" words\n");
    }
    
    // Device capabilities
    print("Firmware Loaded: ");
    print(g_wifi_device->ucode_loaded ? "Yes" : "No");
    print("\nDevice Initialized: ");
    print(g_wifi_device->initialized ? "Yes" : "No");
    print("\n");
}

void wifi_enable_device(void) {
    if (!g_wifi_device) {
        terminal_setcolor(VGA_COLOR_RED);
        print("Error: WiFi not initialized\n");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        return;
    }
    
    if (g_wifi_device->enabled) {
        terminal_setcolor(VGA_COLOR_LIGHT_BROWN);
        print("WiFi device is already enabled\n");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        return;
    }
    
    print("Enabling WiFi device...\n");
    
    if (iwl_start_hw(g_wifi_device) == 0) {
        terminal_setcolor(VGA_COLOR_GREEN);
        print("WiFi device enabled successfully\n");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        
        // Load firmware if not already loaded
        if (!g_wifi_device->ucode_loaded) {
            print("Loading firmware...\n");
            if (iwl_load_ucode(g_wifi_device) == 0) {
                print("Firmware loaded successfully\n");
            } else {
                terminal_setcolor(VGA_COLOR_RED);
                print("Warning: Firmware loading failed\n");
                terminal_setcolor(VGA_COLOR_LIGHT_GREY);
            }
        }
    } else {
        terminal_setcolor(VGA_COLOR_RED);
        print("Failed to enable WiFi device\n");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    }
}

void wifi_disable_device(void) {
    if (!g_wifi_device) {
        terminal_setcolor(VGA_COLOR_RED);
        print("Error: WiFi not initialized\n");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        return;
    }
    
    if (!g_wifi_device->enabled) {
        terminal_setcolor(VGA_COLOR_LIGHT_BROWN);
        print("WiFi device is already disabled\n");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        return;
    }
    
    print("Disabling WiFi device...\n");
    
    iwl_stop_hw(g_wifi_device);
    
    terminal_setcolor(VGA_COLOR_GREEN);
    print("WiFi device disabled successfully\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
}
