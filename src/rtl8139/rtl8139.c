#include "../terminal/terminal.h"
#include "../keyboard/keyboard.h"
#include "../timers/timer.h"
#include "../io/io.h"
#include "../utility/utility.h"
#include "rtl8139.h"

struct rtl8139* RTL8139 = NULL;

// PCI Configuration Space offsets
#define PCI_CONFIG_ADDRESS  0xCF8
#define PCI_CONFIG_DATA     0xCFC

// RTL8139 PCI IDs
#define RTL8139_VENDOR_ID   0x10EC
#define RTL8139_DEVICE_ID   0x8139

// PCI configuration registers
#define PCI_VENDOR_ID       0x00
#define PCI_DEVICE_ID       0x02
#define PCI_COMMAND         0x04
#define PCI_STATUS          0x06
#define PCI_BAR0            0x10
#define PCI_INTERRUPT_LINE  0x3C

// RTL8139 Register definitions
#define RTL8139_REG_TSAD0       0x20    // Transmit Start Address of Descriptor 0
#define RTL8139_REG_TSAD1       0x24    // Transmit Start Address of Descriptor 1
#define RTL8139_REG_TSAD2       0x28    // Transmit Start Address of Descriptor 2
#define RTL8139_REG_TSAD3       0x2C    // Transmit Start Address of Descriptor 3
#define RTL8139_REG_TSD0        0x10    // Transmit Status of Descriptor 0
#define RTL8139_REG_TSD1        0x14    // Transmit Status of Descriptor 1
#define RTL8139_REG_TSD2        0x18    // Transmit Status of Descriptor 2
#define RTL8139_REG_TSD3        0x1C    // Transmit Status of Descriptor 3
#define RTL8139_REG_RBSTART     0x30    // Receive Buffer Start
#define RTL8139_REG_CAPR        0x38    // Current Address of Packet Read
#define RTL8139_REG_CBR         0x3A    // Current Buffer Address

// Transmit Status Register bits
#define RTL8139_TSD_OWN         (1 << 13)   // Descriptor owned by NIC
#define RTL8139_TSD_TUN         (1 << 14)   // Transmit FIFO underrun
#define RTL8139_TSD_TOK         (1 << 15)   // Transmit OK
#define RTL8139_TSD_SIZE_MASK   0x1FFF      // Size mask (bits 0-12)

// Receive packet header structure
typedef struct {
    uint16_t status;
    uint16_t length;
} __attribute__((packed)) rx_packet_header_t;

// Global transmit descriptor index
static uint8_t tx_descriptor = 0;

// Allocate transmit buffers (4 buffers of 2KB each)
static uint8_t* tx_buffers[4] = {NULL, NULL, NULL, NULL};

// Read from PCI configuration space
uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = (1 << 31) | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

// Write to PCI configuration space
void pci_config_write(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    uint32_t address = (1 << 31) | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

// Detect RTL8139 network card (optimized scan)
bool rtl8139_detect() {
    info("Scanning PCI bus for RTL8139 network card...", __FILE__);
    
    uint32_t devices_scanned = 0;
    
    // Scan only bus 0 first (most devices are here)
    for (uint8_t device = 0; device < 32; device++) {
        // Check function 0 first
        uint32_t vendor_device = pci_config_read(0, device, 0, PCI_VENDOR_ID);
        uint16_t vendor_id = vendor_device & 0xFFFF;
        uint16_t device_id = (vendor_device >> 16) & 0xFFFF;
        
        // Skip if no device
        if (vendor_id == 0xFFFF) {
            continue;
        }
        
        devices_scanned++;
        
        if (vendor_id == RTL8139_VENDOR_ID && device_id == RTL8139_DEVICE_ID) {
            info("RTL8139 network card found!", __FILE__);
            
            // Allocate RTL8139 structure
            RTL8139 = (struct rtl8139*)malloc(sizeof(struct rtl8139));
            if (!RTL8139) {
                warn("Failed to allocate memory for RTL8139 structure", __FILE__);
                return false;
            }
            
            // Initialize structure
            memset(RTL8139, 0, sizeof(struct rtl8139));
            
            // Get I/O base address from BAR0
            uint32_t bar0 = pci_config_read(0, device, 0, PCI_BAR0);
            
            // Check if it's an I/O BAR (bit 0 set)
            if (bar0 & 0x01) {
                RTL8139->io_base = bar0 & 0xFFFFFFFC; // Mask off lower bits
            } else {
                warn("RTL8139 BAR0 is not I/O space", __FILE__);
                free(RTL8139);
                RTL8139 = NULL;
                return false;
            }
            
            // Get IRQ
            uint32_t interrupt_line = pci_config_read(0, device, 0, PCI_INTERRUPT_LINE);
            RTL8139->irq = interrupt_line & 0xFF;
            
            // Store PCI info
            RTL8139->vendor_id = vendor_id;
            RTL8139->device_id = device_id;
            
            // Enable PCI device (Bus Master, I/O Space)
            uint32_t command = pci_config_read(0, device, 0, PCI_COMMAND);
            command |= 0x05; // Enable I/O Space and Bus Master
            pci_config_write(0, device, 0, PCI_COMMAND, command);
            
            char buffer[32];
            print("RTL8139 I/O Base: 0x");
            itoa(RTL8139->io_base, buffer, 16);
            print(buffer);
            print(", IRQ: ");
            itoa(RTL8139->irq, buffer, 10);
            print(buffer);
            print("\n");
            
            return true;
        }
        
        // Limit scan to prevent hanging
        if (devices_scanned > 50) {
            warn("PCI scan limit reached", __FILE__);
            break;
        }
    }
    
    warn("RTL8139 network card not found", __FILE__);
    return false;
}

void read_mac_address() {
    if (!RTL8139) return;
    
    for (int i = 0; i < 6; i++) {
        RTL8139->mac_address[i] = inb(RTL8139->io_base + RTL8139_REG_MAC + i);
    }
}

// Initialize RTL8139 NIC (safe version with hardware checks)
bool rtl8139_init() {
    info("Starting RTL8139 initialization...", __FILE__);
    
    // First detect the card
    if (!rtl8139_detect()) {
        warn("RTL8139 Card not detected, skipping initialization", __FILE__);
        return false;
    }
    
    if (!RTL8139 || RTL8139->io_base == 0) {
        warn("RTL8139 invalid I/O base address", __FILE__);
        return false;
    }
    
    info("RTL8139 hardware initialization started!", __FILE__);
    
    // Test if we can read from the device first
    info("Testing device accessibility...", __FILE__);
    uint8_t test_read = inb(RTL8139->io_base);
    char buffer[16];
    itoa(test_read, buffer, 16);
    print("Test read from I/O base: 0x");
    print(buffer);
    print("\n");
    
    // Step 1: Power on the device
    info("Powering on RTL8139...", __FILE__);
    outb(RTL8139->io_base + RTL8139_REG_CONFIG1, 0x00);
    
    // Step 2: Software reset
    info("Performing software reset...", __FILE__);
    outb(RTL8139->io_base + RTL8139_REG_COMMAND, RTL8139_CMD_RESET);
    
    // Wait for reset to complete (with timeout to prevent hanging)
    uint32_t reset_timeout = 1000;
    while ((inb(RTL8139->io_base + RTL8139_REG_COMMAND) & RTL8139_CMD_RESET) && reset_timeout > 0) {
        reset_timeout--;
        if (reset_timeout % 100 == 0) {
            print("Waiting for reset...\n");
        }
    }
    
    if (reset_timeout == 0) {
        warn("RTL8139 reset timeout", __FILE__);
        return false;
    }
    
    info("RTL8139 reset completed", __FILE__);
    
    // Step 3: Read MAC address (after reset)
    info("Reading MAC address...", __FILE__);
    read_mac_address();
    
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
    
    // Step 4: Set up receive buffer (8KB + 16 bytes for overflow)
    info("Setting up receive buffer...", __FILE__);
    static uint8_t static_rx_buffer[8192 + 16];
    RTL8139->rx_buffer = static_rx_buffer;
    memset(RTL8139->rx_buffer, 0, 8192 + 16);
    
    // Set receive buffer start address
    outl(RTL8139->io_base + RTL8139_REG_RBSTART, (uint32_t)RTL8139->rx_buffer);
    info("RX buffer address set", __FILE__);
    
    // Step 5: Initialize transmit buffers
    info("Initializing transmit buffers...", __FILE__);
    if (!rtl8139_init_tx_buffers()) {
        warn("Failed to initialize TX buffers", __FILE__);
        return false;
    }
    
    // Step 6: Set IMR (Interrupt Mask Register) - enable interrupts
    info("Setting up interrupts...", __FILE__);
    outw(RTL8139->io_base + RTL8139_REG_IMR, 
         RTL8139_INT_ROK |      // Receive OK
         RTL8139_INT_TOK |      // Transmit OK
         RTL8139_INT_RER |      // Receive Error
         RTL8139_INT_TER |      // Transmit Error
         RTL8139_INT_RXOVW |    // RX Buffer Overflow
         RTL8139_INT_PUN |      // Packet Underrun
         RTL8139_INT_FOVW);     // FIFO Overflow
    
    // Step 7: Configure receive (RCR)
    info("Configuring receive settings...", __FILE__);
    outl(RTL8139->io_base + RTL8139_REG_RCR,
         RTL8139_RCR_AAP |      // Accept All Packets
         RTL8139_RCR_APM |      // Accept Physical Match
         RTL8139_RCR_AM |       // Accept Multicast
         RTL8139_RCR_AB |       // Accept Broadcast
         RTL8139_RCR_AR |       // Accept Runt packets
         RTL8139_RCR_AER);      // Accept Error packets
    
    // Step 8: Configure transmit (TCR)
    info("Configuring transmit settings...", __FILE__);
    outl(RTL8139->io_base + RTL8139_REG_TCR,
         RTL8139_TCR_IFG96 |    // Interframe Gap
         RTL8139_TCR_MXDMA_2048); // Max DMA burst size
    
    // Step 9: Reset packet counters
    info("Resetting packet counters...", __FILE__);
    outw(RTL8139->io_base + RTL8139_REG_CAPR, 0);
    outw(RTL8139->io_base + RTL8139_REG_CBR, 0);
    
    // Step 10: Enable transmitter and receiver
    info("Enabling transmitter and receiver...", __FILE__);
    outb(RTL8139->io_base + RTL8139_REG_COMMAND, 
         RTL8139_CMD_RX_ENABLE | RTL8139_CMD_TX_ENABLE);
    
    // Step 11: Verify initialization
    info("Verifying initialization...", __FILE__);
    uint8_t cmd_status = inb(RTL8139->io_base + RTL8139_REG_COMMAND);
    if ((cmd_status & (RTL8139_CMD_RX_ENABLE | RTL8139_CMD_TX_ENABLE)) == 
        (RTL8139_CMD_RX_ENABLE | RTL8139_CMD_TX_ENABLE)) {
        info("RTL8139 TX/RX enabled successfully", __FILE__);
    } else {
        warn("RTL8139 TX/RX enable failed", __FILE__);
        return false;
    }
    
    // Mark as initialized
    RTL8139->initialized = true;
    
    done("RTL8139 hardware initialization complete!", __FILE__);
    return true;
}



// Simple RTL8139 test function
void rtl8139_test() {
    if (!RTL8139 || !RTL8139->initialized) {
        print("RTL8139 not initialized\n");
        return;
    }
    
    print("RTL8139 Test:\n");
    
    // Read some registers to verify functionality
    uint8_t cmd = inb(RTL8139->io_base + RTL8139_REG_COMMAND);
    uint16_t status = inw(RTL8139->io_base + 0x3E);
    
    print("Command Register: 0x");
    char buffer[8];
    itoa(cmd, buffer, 16);
    print(buffer);
    print("\nStatus Register: 0x");
    itoa(status, buffer, 16);
    print(buffer);
    print("\n");
    
    if (cmd & (RTL8139_CMD_RX_ENABLE | RTL8139_CMD_TX_ENABLE)) {
        terminal_setcolor(VGA_COLOR_GREEN);
        print("RTL8139 is active and ready\n");
    } else {
        terminal_setcolor(VGA_COLOR_RED);
        print("RTL8139 is not active\n");
    }
    terminal_setcolor(VGA_COLOR_WHITE);
}

// Get RTL8139 status
void rtl8139_print_status() {
    if (!RTL8139) {
        print("RTL8139: Not detected\n");
        return;
    }
    
    if (!RTL8139->initialized) {
        print("RTL8139: Detected but not initialized\n");
        return;
    }
    
    print("RTL8139 Network Card Status:\n");
    print("  Vendor ID: 0x");
    char buffer[16];
    itoa(RTL8139->vendor_id, buffer, 16);
    print(buffer);
    print("\n  Device ID: 0x");
    itoa(RTL8139->device_id, buffer, 16);
    print(buffer);
    print("\n  I/O Base: 0x");
    itoa(RTL8139->io_base, buffer, 16);
    print(buffer);
    print("\n  IRQ: ");
    itoa(RTL8139->irq, buffer, 10);
    print(buffer);
    print("\n  MAC Address: ");
    
    for (int i = 0; i < 6; i++) {
        itoa(RTL8139->mac_address[i], buffer, 16);
        if (RTL8139->mac_address[i] < 16) {
            print("0");
        }
        print(buffer);
        if (i < 5) print(":");
    }
    print("\n  Status: ");
    
    terminal_setcolor(VGA_COLOR_GREEN);
    print("Initialized and Ready");
    terminal_setcolor(VGA_COLOR_WHITE);
    print("\n");
}
// Fixed TX buffer initialization
bool rtl8139_init_tx_buffers() {
    if (!RTL8139) {
        warn("RTL8139 structure is NULL", __FILE__);
        return false;
    }
    
    if (RTL8139->io_base == 0) {
        warn("RTL8139 I/O base not set", __FILE__);
        return false;
    }
    
    info("Initializing TX buffers with hardware setup...", __FILE__);
    
    // Static buffer array - each buffer must be 2KB aligned for RTL8139
    static uint8_t static_tx_buffers[4][2048] __attribute__((aligned(4)));
    
    // Configure each transmit descriptor
    for (int i = 0; i < 4; i++) {
        tx_buffers[i] = static_tx_buffers[i];
        memset(tx_buffers[i], 0, 2048);
        
        // Calculate register addresses
        uint16_t tsad_reg = RTL8139_REG_TSAD0 + (i * 4);
        uint16_t tsd_reg = RTL8139_REG_TSD0 + (i * 4);
        
        // Set buffer address in hardware
        uint32_t buffer_addr = (uint32_t)tx_buffers[i];
        outl(RTL8139->io_base + tsad_reg, buffer_addr);
        
        // Initialize descriptor status (mark as available)
        // Clear the descriptor first, then set TOK bit
        outl(RTL8139->io_base + tsd_reg, RTL8139_TSD_TOK);
        
        // Verify the write worked
        uint32_t read_back = inl(RTL8139->io_base + tsad_reg);
        if (read_back != buffer_addr) {
            warn("TX buffer address write failed", __FILE__);
            char buffer[32];
            print("Expected: 0x");
            itoa(buffer_addr, buffer, 16);
            print(buffer);
            print(", Got: 0x");
            itoa(read_back, buffer, 16);
            print(buffer);
            print("\n");
            return false;
        }
        
        char buffer[16];
        print("TX Buffer ");
        itoa(i, buffer, 10);
        print(buffer);
        print(" at 0x");
        itoa(buffer_addr, buffer, 16);
        print(buffer);
        print(" -> TSAD");
        itoa(i, buffer, 10);
        print(buffer);
        print(" (verified)\n");
    }
    
    // Reset TX descriptor index
    tx_descriptor = 0;
    
    info("RTL8139 TX buffers initialized successfully", __FILE__);
    return true;
}

// Real hardware send packet function
bool rtl8139_send_packet(const int8* data, int16 length) {
    if (!RTL8139 || !RTL8139->initialized) {
        warn("RTL8139 Card is not initialized", __FILE__);
        return false;
    }
    
    if (!data || length <= 0 || length > 1518) {
        warn("Invalid packet data or length", __FILE__);
        return false;
    }
    
    // Initialize TX buffers if not done already
    if (!tx_buffers[0]) {
        if (!rtl8139_init_tx_buffers()) {
            return false;
        }
    }
    
    // Get current transmit descriptor
    uint8_t desc = tx_descriptor;
    uint16_t tsd_reg = RTL8139_REG_TSD0 + (desc * 4);
    uint16_t tsad_reg = RTL8139_REG_TSAD0 + (desc * 4);
    
    // Check if descriptor is available (with timeout to prevent hanging)
    uint32_t timeout = 1000;
    while (timeout > 0) {
        uint32_t tsd_status = inl(RTL8139->io_base + tsd_reg);
        
        // Check if descriptor is free (OWN bit clear or TOK bit set)
        if ((tsd_status & RTL8139_TSD_OWN) == 0 || (tsd_status & RTL8139_TSD_TOK)) {
            break; // Descriptor is available
        }
        
        timeout--;
        if (timeout % 100 == 0) {
            print("Waiting for TX descriptor...\n");
        }
    }
    
    if (timeout == 0) {
        warn("TX descriptor timeout", __FILE__);
        return false;
    }
    
    // Ensure minimum packet size (64 bytes for Ethernet)
    uint16_t actual_length = length;
    if (actual_length < 64) {
        actual_length = 64;
    }
    
    // Copy packet data to transmit buffer
    memcpy(tx_buffers[desc], data, length);
    
    // Pad with zeros if necessary
    if (actual_length > length) {
        memset(tx_buffers[desc] + length, 0, actual_length - length);
    }
    
    // Set transmit buffer address (if not already set)
    outl(RTL8139->io_base + tsad_reg, (uint32_t)tx_buffers[desc]);
    
    // Set transmit descriptor with length and start transmission
    uint32_t tsd_value = actual_length & RTL8139_TSD_SIZE_MASK;
    outl(RTL8139->io_base + tsd_reg, tsd_value);
    
    info("Packet transmitted via hardware", __FILE__);
    
    // Move to next descriptor
    tx_descriptor = (tx_descriptor + 1) % 4;
    
    return true;
}

// Receive a packet
bool rtl8139_receive_packet(int8* buffer, int16* length) {
    if (!RTL8139 || RTL8139->io_base == 0 || !RTL8139->initialized) {
        warn("RTL8139 Card is not initialized", __FILE__);
        return false;
    }
    
    if (!buffer || !length) {
        warn("Invalid buffer or length pointer", __FILE__);
        return false;
    }
    
    // Check if receive buffer is initialized
    if (!RTL8139->rx_buffer) {
        warn("RX buffer not initialized", __FILE__);
        return false;
    }
    
    // Get current buffer read pointer
    uint16_t capr = inw(RTL8139->io_base + RTL8139_REG_CAPR);
    uint16_t cbr = inw(RTL8139->io_base + RTL8139_REG_CBR);
    
    // Check if there's data to read
    if (capr == cbr) {
        return false; // No packet available
    }
    
    // Calculate current position in receive buffer
    uint16_t current_pos = (capr + 0x10) % 8192;
    
    // Read packet header
    rx_packet_header_t* header = (rx_packet_header_t*)(RTL8139->rx_buffer + current_pos);
    
    // Check if packet is valid
    if (!(header->status & 0x01)) {
        // Packet not ready or invalid
        return false;
    }
    
    // Get packet length (subtract CRC)
    uint16_t packet_length = header->length - 4;
    
    // Validate packet length
    if (packet_length > 1518 || packet_length < 14) {
        warn("Invalid packet length received", __FILE__);
        // Skip this packet
        current_pos = (current_pos + header->length + 4 + 3) & ~3; // Align to 4 bytes
        outw(RTL8139->io_base + RTL8139_REG_CAPR, current_pos - 0x10);
        return false;
    }
    
    // Copy packet data to buffer
    uint8_t* packet_data = RTL8139->rx_buffer + current_pos + sizeof(rx_packet_header_t);
    
    // Handle buffer wrap-around
    if (current_pos + sizeof(rx_packet_header_t) + packet_length > 8192) {
        // Packet wraps around
        uint16_t first_part = 8192 - (current_pos + sizeof(rx_packet_header_t));
        memcpy(buffer, packet_data, first_part);
        memcpy(buffer + first_part, RTL8139->rx_buffer, packet_length - first_part);
    } else {
        // Packet doesn't wrap
        memcpy(buffer, packet_data, packet_length);
    }
    
    *length = packet_length;
    
    // Update CAPR to indicate packet has been read
    current_pos = (current_pos + header->length + 4 + 3) & ~3; // Align to 4 bytes
    if (current_pos >= 8192) {
        current_pos -= 8192;
    }
    
    outw(RTL8139->io_base + RTL8139_REG_CAPR, current_pos - 0x10);
    
    return true;
}

bool rtl8139_tx_status(uint8_t descriptor) {
    if (!RTL8139 || descriptor >= 4) {
        return false;
    }
    
    uint16_t tsd_reg = RTL8139_REG_TSD0 + (descriptor * 4);
    uint32_t status = inl(RTL8139->io_base + tsd_reg);
    uint8_t last_desc = (tx_descriptor == 0) ? 3 : (tx_descriptor - 1);
if (rtl8139_tx_status(last_desc)) {
    print("Test packet transmission confirmed\n");
} else {
    print("Test packet transmission status unknown\n");
}
    // Check if Transmit OK bit is set
    return (status & RTL8139_TSD_TOK) != 0;
}
// Get receive buffer statistics
void rtl8139_rx_stats() {
    if (!RTL8139 || !RTL8139->initialized) {
        print("RTL8139 not initialized\n");
        return;
    }
    
    uint16_t capr = inw(RTL8139->io_base + RTL8139_REG_CAPR);
    uint16_t cbr = inw(RTL8139->io_base + RTL8139_REG_CBR);
    
    print("RX Buffer Stats:\n");
    print("  CAPR (Current Address Packet Read): ");
    char buffer[16];
    itoa(capr, buffer, 16);
    print("0x");
    print(buffer);
    print("\n  CBR (Current Buffer Address): ");
    itoa(cbr, buffer, 16);
    print("0x");
    print(buffer);
    print("\n");
    
    if (capr == cbr) {
        print("  Status: No packets available\n");
    } else {
        print("  Status: Packets available for reading\n");
    }
}

// Test packet transmission
void rtl8139_test_tx() {
    if (!RTL8139 || !RTL8139->initialized) {
        print("RTL8139 not initialized\n");
        return;
    }
    
    // Create a simple test packet (Ethernet frame)
    uint8_t test_packet[64];
    memset(test_packet, 0, 64);
    
    // Destination MAC (broadcast)
    for (int i = 0; i < 6; i++) {
        test_packet[i] = 0xFF;
    }
    
    // Source MAC (our MAC)
    for (int i = 0; i < 6; i++) {
        test_packet[6 + i] = RTL8139->mac_address[i];
    }
    
    // EtherType (0x0800 for IPv4)
    test_packet[12] = 0x08;
    test_packet[13] = 0x00;
    
    // Simple payload
    for (int i = 14; i < 64; i++) {
        test_packet[i] = i - 14;
    }
    
    print("Sending test packet...\n");
    if (rtl8139_send_packet((int8*)test_packet, 64)) {
        print("Test packet sent successfully\n");
        
        // Check transmission status after a delay
        delay(10);
        if (rtl8139_tx_status(tx_descriptor == 0 ? 3 : tx_descriptor - 1)) {
            print("Test packet transmission confirmed\n");
        } else {
            print("Test packet transmission status unknown\n");
        }
    } else {
        print("Failed to send test packet\n");
    }
}

// Packet monitoring function: continuously receive and display packets
void rtl8139_monitor_packets() {
    char input[COMMAND_BUFFER_SIZE];
    if (!RTL8139 || !RTL8139->initialized) {
        print("RTL8139 not initialized\n");
        return;
    }

    print("Starting packet monitoring. Press any key to stop...\n");

    uint8_t packet_buffer[1600]; // buffer for received packets (max Ethernet frame size)

    while (true) {
        // Check if a key is pressed to exit monitoring
        if (keyboard_input(input) == -1) {
            print("Packet monitoring stopped by user.\n");
            break;
        }

        // Read CAPR and CBR registers
        uint16_t capr = inw(RTL8139->io_base + RTL8139_REG_CAPR);
        uint16_t cbr = inw(RTL8139->io_base + RTL8139_REG_CBR);

        // Print CAPR and CBR for debugging
        print("CAPR: 0x");
        char capr_str[8];
        itoa(capr, capr_str, 16);
        print(capr_str);
        print(", CBR: 0x");
        char cbr_str[8];
        itoa(cbr, cbr_str, 16);
        print(cbr_str);
        print("\n");

        if (capr == cbr) {
            // No new packets
            delay(50);
            continue;
        }

        // Calculate current position in receive buffer
        uint16_t current_pos = (capr + 0x10) % 8192;

        // Read packet header from receive buffer
        rx_packet_header_t* header = (rx_packet_header_t*)(RTL8139->rx_buffer + current_pos);

        // Check if packet is valid
        if (!(header->status & 0x01)) {
            print("Packet status invalid or not ready\n");
            delay(50);
            continue;
        }

        // Get packet length (subtract CRC)
        uint16_t packet_length = header->length - 4;

        // Validate packet length
        if (packet_length > 1518 || packet_length < 14) {
            print("Invalid packet length, skipping\n");
            // Advance CAPR to skip this packet
            current_pos = (current_pos + header->length + 4 + 3) & ~3; // Align to 4 bytes
            if (current_pos >= 8192) current_pos -= 8192;
            outw(RTL8139->io_base + RTL8139_REG_CAPR, current_pos - 0x10);
            delay(50);
            continue;
        }

        // Copy packet data to local buffer, handle wrap-around
        uint8_t* packet_data = RTL8139->rx_buffer + current_pos + sizeof(rx_packet_header_t);
        if (current_pos + sizeof(rx_packet_header_t) + packet_length > 8192) {
            uint16_t first_part = 8192 - (current_pos + sizeof(rx_packet_header_t));
            memcpy(packet_buffer, packet_data, first_part);
            memcpy(packet_buffer + first_part, RTL8139->rx_buffer, packet_length - first_part);
        } else {
            memcpy(packet_buffer, packet_data, packet_length);
        }

        // Print packet info
        print("Packet received: length = ");
        char len_str[8];
        itoa(packet_length, len_str, 10);
        print(len_str);
        print(" bytes\n");

        print("Data (first 16 bytes): ");
        for (int i = 0; i < 16 && i < packet_length; i++) {
            char hex_byte[4];
            itoa(packet_buffer[i], hex_byte, 16);
            if (packet_buffer[i] < 16) print("0");
            print(hex_byte);
            print(" ");
        }
        print("\n");

        // Advance CAPR to indicate packet has been read
        current_pos = (current_pos + header->length + 4 + 3) & ~3; // Align to 4 bytes
        if (current_pos >= 8192) current_pos -= 8192;
        outw(RTL8139->io_base + RTL8139_REG_CAPR, current_pos - 0x10);

        delay(10);
    }
}


// Enhanced network command with more options
void network_command(int argc, char* argv[]) {
    if (argc < 2) {
        print("Usage: network <command>\n");
        print("Commands:\n");
        print("  status    - Show network card status\n");
        print("  test      - Test network card functionality\n");
        print("  testtx    - Test packet transmission\n");
        print("  rxstats   - Show receive buffer statistics\n");
        print("  init      - Re-initialize network card\n");
        print("  detect    - Detect network card only\n");
        return;
    }
    
    if (strcmp(argv[1], "status") == 0) {
        rtl8139_print_status();
    } else if (strcmp(argv[1], "test") == 0) {
        rtl8139_test();
    } else if (strcmp(argv[1], "testtx") == 0) {
        rtl8139_test_tx();
    } else if (strcmp(argv[1], "rxstats") == 0) {
        rtl8139_rx_stats();
    } else if (strcmp(argv[1], "init") == 0) {
        if (rtl8139_init()) {
            print("RTL8139 re-initialization successful\n");
        } else {
            print("RTL8139 re-initialization failed\n");
        }
    } else if (strcmp(argv[1], "detect") == 0) {
        if (rtl8139_detect()) {
            print("RTL8139 detection successful\n");
        } else {
            print("RTL8139 detection failed\n");
        }
    } else if (strcmp(argv[1], "monitor") == 0) {
        rtl8139_monitor_packets();
    } else {
        print("Unknown network command: ");
        print(argv[1]);
        print("\n");
    }
}


// Updated cleanup function (no need to free static buffers)
void rtl8139_cleanup() {
    if (RTL8139) {
        // TX buffers are static, so just set pointers to NULL
        for (int i = 0; i < 4; i++) {
            tx_buffers[i] = NULL;
        }
        
        // RX buffer might be static too, so check before freeing
        if (RTL8139->rx_buffer) {
            // Only free if it was dynamically allocated
            // Since we're using static buffers, just set to NULL
            RTL8139->rx_buffer = NULL;
        }
        
        if (RTL8139->tx_buffer) {
            RTL8139->tx_buffer = NULL;
        }
        
        free(RTL8139);
        RTL8139 = NULL;
    }
}
