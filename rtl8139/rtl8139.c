// rtl8139.c - Complete RTL8139 Network Driver
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
#define RTL8139_REG_TSAD0       0x20
#define RTL8139_REG_TSAD1       0x24
#define RTL8139_REG_TSAD2       0x28
#define RTL8139_REG_TSAD3       0x2C
#define RTL8139_REG_TSD0        0x10
#define RTL8139_REG_TSD1        0x14
#define RTL8139_REG_TSD2        0x18
#define RTL8139_REG_TSD3        0x1C
#define RTL8139_REG_RBSTART     0x30
#define RTL8139_REG_CAPR        0x38
#define RTL8139_REG_CBR         0x3A

// Transmit Status Register bits
#define RTL8139_TSD_OWN         (1 << 13)
#define RTL8139_TSD_TUN         (1 << 14)
#define RTL8139_TSD_TOK         (1 << 15)
#define RTL8139_TSD_SIZE_MASK   0x1FFF

// Receive packet header structure
typedef struct {
    uint16_t status;
    uint16_t length;
} __attribute__((packed)) rx_packet_header_t;

// Global transmit descriptor index
static uint8_t tx_descriptor = 0;

// Allocate transmit buffers (4 buffers of 2KB each)
static uint8_t* tx_buffers[4] = {NULL, NULL, NULL, NULL};

// Forward declaration
extern void netstack_process_packet(uint8_t* data, uint16_t length);

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

// Detect RTL8139 network card
bool rtl8139_detect() {
    info("Scanning PCI bus for RTL8139 network card...", __FILE__);
    
    uint32_t devices_scanned = 0;
    
    // Scan bus 0
    for (uint8_t device = 0; device < 32; device++) {
        uint32_t vendor_device = pci_config_read(0, device, 0, PCI_VENDOR_ID);
        uint16_t vendor_id = vendor_device & 0xFFFF;
        uint16_t device_id = (vendor_device >> 16) & 0xFFFF;
        
        if (vendor_id == 0xFFFF) {
            continue;
        }
        
        devices_scanned++;
        
        if (vendor_id == RTL8139_VENDOR_ID && device_id == RTL8139_DEVICE_ID) {
            info("RTL8139 network card found!", __FILE__);
            
            RTL8139 = (struct rtl8139*)malloc(sizeof(struct rtl8139));
            if (!RTL8139) {
                warn("Failed to allocate memory for RTL8139 structure", __FILE__);
                return false;
            }
            
            memset(RTL8139, 0, sizeof(struct rtl8139));
            
            // Get I/O base address from BAR0
            uint32_t bar0 = pci_config_read(0, device, 0, PCI_BAR0);
            
            if (bar0 & 0x01) {
                RTL8139->io_base = bar0 & 0xFFFFFFFC;
            } else {
                warn("RTL8139 BAR0 is not I/O space", __FILE__);
                free(RTL8139);
                RTL8139 = NULL;
                return false;
            }
            
            // Get IRQ
            uint32_t interrupt_line = pci_config_read(0, device, 0, PCI_INTERRUPT_LINE);
            RTL8139->irq = interrupt_line & 0xFF;
            
            RTL8139->vendor_id = vendor_id;
            RTL8139->device_id = device_id;
            
            // Enable PCI device (Bus Master, I/O Space)
            uint32_t command = pci_config_read(0, device, 0, PCI_COMMAND);
            command |= 0x05;
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
        
        if (devices_scanned > 50) {
            warn("PCI scan limit reached", __FILE__);
            break;
        }
    }
    
    warn("RTL8139 network card not found", __FILE__);
    return false;
}

// Read MAC address
void read_mac_address() {
    if (!RTL8139) return;
    
    for (int i = 0; i < 6; i++) {
        RTL8139->mac_address[i] = inb(RTL8139->io_base + RTL8139_REG_MAC + i);
    }
}

// Get MAC address (for external use)
void rtl8139_get_mac_address(uint8_t* mac) {
    if (!RTL8139 || !mac) return;
    
    for (int i = 0; i < 6; i++) {
        mac[i] = RTL8139->mac_address[i];
    }
}

// Initialize TX buffers
bool rtl8139_init_tx_buffers() {
    if (!RTL8139 || RTL8139->io_base == 0) {
        warn("RTL8139 structure is NULL or I/O base not set", __FILE__);
        return false;
    }
    
    info("Initializing TX buffers...", __FILE__);
    
    static uint8_t static_tx_buffers[4][2048] __attribute__((aligned(4)));
    
    for (int i = 0; i < 4; i++) {
        tx_buffers[i] = static_tx_buffers[i];
        memset(tx_buffers[i], 0, 2048);
        
        uint16_t tsad_reg = RTL8139_REG_TSAD0 + (i * 4);
        uint16_t tsd_reg = RTL8139_REG_TSD0 + (i * 4);
        
        uint32_t buffer_addr = (uint32_t)tx_buffers[i];
        outl(RTL8139->io_base + tsad_reg, buffer_addr);
        outl(RTL8139->io_base + tsd_reg, RTL8139_TSD_TOK);
        
        uint32_t read_back = inl(RTL8139->io_base + tsad_reg);
        if (read_back != buffer_addr) {
            warn("TX buffer address write failed", __FILE__);
            return false;
        }
    }
    
    tx_descriptor = 0;
    info("TX buffers initialized successfully", __FILE__);
    return true;
}

// Initialize RTL8139 NIC
bool rtl8139_init() {
    info("Starting RTL8139 initialization...", __FILE__);
    
    if (!rtl8139_detect()) {
        warn("RTL8139 not detected", __FILE__);
        return false;
    }
    
    if (!RTL8139 || RTL8139->io_base == 0) {
        warn("Invalid RTL8139 I/O base", __FILE__);
        return false;
    }
    
    info("RTL8139 hardware initialization started", __FILE__);
    
    // Power on
    outb(RTL8139->io_base + RTL8139_REG_CONFIG1, 0x00);
    
    // Software reset
    info("Performing software reset...", __FILE__);
    outb(RTL8139->io_base + RTL8139_REG_COMMAND, RTL8139_CMD_RESET);
    
    uint32_t reset_timeout = 1000;
    while ((inb(RTL8139->io_base + RTL8139_REG_COMMAND) & RTL8139_CMD_RESET) && reset_timeout > 0) {
        reset_timeout--;
    }
    
    if (reset_timeout == 0) {
        warn("RTL8139 reset timeout", __FILE__);
        return false;
    }
    
    info("RTL8139 reset completed", __FILE__);
    
    // Read MAC address
    info("Reading MAC address...", __FILE__);
    read_mac_address();
    
    print("MAC Address: ");
    for (int i = 0; i < 6; i++) {
        char hex_str[4];
        itoa(RTL8139->mac_address[i], hex_str, 16);
        if (RTL8139->mac_address[i] < 16) print("0");
        print(hex_str);
        if (i < 5) print(":");
    }
    print("\n");
    
    // Set up receive buffer
    info("Setting up receive buffer...", __FILE__);
    static uint8_t static_rx_buffer[8192 + 16];
    RTL8139->rx_buffer = static_rx_buffer;
    memset(RTL8139->rx_buffer, 0, 8192 + 16);
    
    outl(RTL8139->io_base + RTL8139_REG_RBSTART, (uint32_t)RTL8139->rx_buffer);
    info("RX buffer address set", __FILE__);
    
    // Initialize transmit buffers
    if (!rtl8139_init_tx_buffers()) {
        warn("Failed to initialize TX buffers", __FILE__);
        return false;
    }
    
    // Set IMR (Interrupt Mask Register)
    info("Setting up interrupts...", __FILE__);
    outw(RTL8139->io_base + RTL8139_REG_IMR, 
         RTL8139_INT_ROK | RTL8139_INT_TOK | RTL8139_INT_RER | 
         RTL8139_INT_TER | RTL8139_INT_RXOVW | RTL8139_INT_PUN | 
         RTL8139_INT_FOVW);
    
    // Configure receive (RCR)
    info("Configuring receive settings...", __FILE__);
    outl(RTL8139->io_base + RTL8139_REG_RCR,
         RTL8139_RCR_AAP | RTL8139_RCR_APM | RTL8139_RCR_AM | 
         RTL8139_RCR_AB | RTL8139_RCR_AR | RTL8139_RCR_AER);
    
    // Configure transmit (TCR)
    info("Configuring transmit settings...", __FILE__);
    outl(RTL8139->io_base + RTL8139_REG_TCR,
         RTL8139_TCR_IFG96 | RTL8139_TCR_MXDMA_2048);
    
    // Reset packet counters
    outw(RTL8139->io_base + RTL8139_REG_CAPR, 0);
    outw(RTL8139->io_base + RTL8139_REG_CBR, 0);
    
    // Enable transmitter and receiver
    info("Enabling transmitter and receiver...", __FILE__);
    outb(RTL8139->io_base + RTL8139_REG_COMMAND, 
         RTL8139_CMD_RX_ENABLE | RTL8139_CMD_TX_ENABLE);
    
    // Verify initialization
    uint8_t cmd_status = inb(RTL8139->io_base + RTL8139_REG_COMMAND);
    if ((cmd_status & (RTL8139_CMD_RX_ENABLE | RTL8139_CMD_TX_ENABLE)) == 
        (RTL8139_CMD_RX_ENABLE | RTL8139_CMD_TX_ENABLE)) {
        info("RTL8139 TX/RX enabled successfully", __FILE__);
    } else {
        warn("RTL8139 TX/RX enable failed", __FILE__);
        return false;
    }
    
    RTL8139->initialized = true;
    
    done("RTL8139 hardware initialization complete!", __FILE__);
    return true;
}

// Send packet
// Debugged version of rtl8139_send_packet with detailed diagnostics

bool rtl8139_send_packet(const int8* data, int16 length) {
    char debug_buf[64];
    
    // === VALIDATION CHECKS ===
    print("\n[TX DEBUG] Starting packet send...\n");
    
    if (!RTL8139) {
        print("[TX ERROR] RTL8139 is NULL\n");
        return false;
    }
    
    if (!RTL8139->initialized) {
        print("[TX ERROR] RTL8139 not initialized\n");
        return false;
    }
    
    if (!data) {
        print("[TX ERROR] Data pointer is NULL\n");
        return false;
    }
    
    if (length <= 0 || length > 1518) {
        print("[TX ERROR] Invalid length: ");
        itoa(length, debug_buf, 10);
        print(debug_buf);
        print("\n");
        return false;
    }
    
    print("[TX] Packet length: ");
    itoa(length, debug_buf, 10);
    print(debug_buf);
    print(" bytes\n");
    
    // === TX BUFFER INITIALIZATION ===
    if (!tx_buffers[0]) {
        print("[TX] TX buffers not initialized, initializing...\n");
        if (!rtl8139_init_tx_buffers()) {
            print("[TX ERROR] Failed to initialize TX buffers\n");
            return false;
        }
    }
    
    // === DESCRIPTOR SELECTION ===
    uint8_t desc = tx_descriptor;
    print("[TX] Using descriptor: ");
    itoa(desc, debug_buf, 10);
    print(debug_buf);
    print("\n");
    
    uint16_t tsd_reg = RTL8139_REG_TSD0 + (desc * 4);
    uint16_t tsad_reg = RTL8139_REG_TSAD0 + (desc * 4);
    
    print("[TX] TSD register offset: 0x");
    itoa(tsd_reg, debug_buf, 16);
    print(debug_buf);
    print("\n");
    
    // === CHECK BUFFER ADDRESS ===
    print("[TX] Reading TSAD register...\n");
    uint32_t buffer_addr = inl(RTL8139->io_base + tsad_reg);
    print("[TX] TSAD read complete\n");
    
    print("[TX] Buffer address in TSAD: 0x");
    itoa(buffer_addr, debug_buf, 16);
    print(debug_buf);
    print("\n");
    
    uint32_t expected_addr = (uint32_t)tx_buffers[desc];
    print("[TX] Expected buffer address: 0x");
    itoa(expected_addr, debug_buf, 16);
    print(debug_buf);
    print("\n");
    
    if (buffer_addr != expected_addr) {
        print("[TX WARNING] Buffer address mismatch! Rewriting TSAD...\n");
        outl(RTL8139->io_base + tsad_reg, expected_addr);
        print("[TX] TSAD rewritten\n");
        
        // Verify
        buffer_addr = inl(RTL8139->io_base + tsad_reg);
        print("[TX] TSAD after rewrite: 0x");
        itoa(buffer_addr, debug_buf, 16);
        print(debug_buf);
        print("\n");
    }
    
    // === WAIT FOR DESCRIPTOR ===
    print("[TX] Waiting for descriptor to be available...\n");
    
    uint32_t timeout = 100000;
    uint32_t tsd;
    
    print("[TX] Reading initial TSD...\n");
    uint32_t initial_tsd = inl(RTL8139->io_base + tsd_reg);
    print("[TX] Initial TSD read complete\n");
    
    print("[TX] Initial TSD value: 0x");
    itoa(initial_tsd, debug_buf, 16);
    print(debug_buf);
    print("\n");
    
    // Check individual bits
    print("[TX] Initial TSD bits:\n");
    print("      OWN (bit 13): ");
    print((initial_tsd & RTL8139_TSD_OWN) ? "SET" : "CLEAR");
    print("\n      TOK (bit 15): ");
    print((initial_tsd & RTL8139_TSD_TOK) ? "SET" : "CLEAR");
    print("\n      TUN (bit 14): ");
    print((initial_tsd & RTL8139_TSD_TUN) ? "SET" : "CLEAR");
    print("\n");
    
    uint32_t loops = 0;
    while (timeout > 0) {
        tsd = inl(RTL8139->io_base + tsd_reg);
        
        // Debug: Print TSD value every 10000 iterations
        if (loops % 10000 == 0 && loops > 0) {
            print("[TX] Still waiting... TSD: 0x");
            itoa(tsd, debug_buf, 16);
            print(debug_buf);
            print(" (loop ");
            itoa(loops, debug_buf, 10);
            print(debug_buf);
            print(")\n");
        }
        
        // Check if descriptor is free
        if ((tsd & RTL8139_TSD_OWN) || (tsd & RTL8139_TSD_TOK) || (tsd == 0)) {
            print("[TX] Descriptor available after ");
            itoa(loops, debug_buf, 10);
            print(debug_buf);
            print(" loops\n");
            break;
        }
        
        timeout--;
        loops++;
    }
    
    if (timeout == 0) {
        print("[TX ERROR] Descriptor timeout! Final TSD: 0x");
        itoa(tsd, debug_buf, 16);
        print(debug_buf);
        print("\n");
        
        // Additional debugging
        print("[TX] Command register: 0x");
        uint8_t cmd = inb(RTL8139->io_base + RTL8139_REG_COMMAND);
        itoa(cmd, debug_buf, 16);
        print(debug_buf);
        print("\n");
        
        print("[TX] TX enabled: ");
        print((cmd & RTL8139_CMD_TX_ENABLE) ? "YES" : "NO");
        print("\n");
        
        return false;
    }
    
    print("[TX] Final TSD before write: 0x");
    itoa(tsd, debug_buf, 16);
    print(debug_buf);
    print("\n");
    
    // === PREPARE PACKET ===
    uint16_t actual_length = length < 60 ? 60 : length;
    
    if (actual_length != length) {
        print("[TX] Padding packet from ");
        itoa(length, debug_buf, 10);
        print(debug_buf);
        print(" to ");
        itoa(actual_length, debug_buf, 10);
        print(debug_buf);
        print(" bytes\n");
    }
    
    // Copy packet data
    print("[TX] Copying packet data...\n");
    memcpy(tx_buffers[desc], data, length);
    print("[TX] Data copied\n");
    
    // Zero-pad if needed
    if (actual_length > length) {
        print("[TX] Zero-padding...\n");
        memset(tx_buffers[desc] + length, 0, actual_length - length);
        print("[TX] Padding complete\n");
    }
    
    // Debug: Print first few bytes of packet
    print("[TX] Packet data (first 14 bytes): ");
    for (int i = 0; i < 14 && i < actual_length; i++) {
        uint8_t byte = ((uint8_t*)data)[i];
        if (byte < 16) print("0");
        itoa(byte, debug_buf, 16);
        print(debug_buf);
        print(" ");
    }
    print("\n");
    
    // === START TRANSMISSION ===
    print("[TX] Preparing to write TSD register...\n");
    print("[TX] TSD register address: 0x");
    itoa(RTL8139->io_base + tsd_reg, debug_buf, 16);
    print(debug_buf);
    print("\n");
    
    uint32_t tsd_value = actual_length & 0x1FFF;
    print("[TX] Value to write: 0x");
    itoa(tsd_value, debug_buf, 16);
    print(debug_buf);
    print(" (");
    itoa(tsd_value, debug_buf, 10);
    print(debug_buf);
    print(" bytes)\n");
    
    // CRITICAL: Check if I/O operations are working
    print("[TX] Testing I/O before write...\n");
    uint8_t test_cmd = inb(RTL8139->io_base + RTL8139_REG_COMMAND);
    print("[TX] Command register test read: 0x");
    itoa(test_cmd, debug_buf, 16);
    print(debug_buf);
    print("\n");
    // Verify I/O base is valid
print("[TX] IO base: 0x");
itoa(RTL8139->io_base, debug_buf, 16);
print(debug_buf);
print("\n");

if (RTL8139->io_base > 0xFFFF || RTL8139->io_base < 0x100) {
    print("[TX ERROR] Invalid I/O base address!\n");
    return false;
}

// Try a read first to see if it hangs too
print("[TX] Testing read...\n");
volatile uint32_t test = inl(RTL8139->io_base + tsd_reg);
print("[TX] Read OK: 0x");
itoa(test, debug_buf, 16);
print(debug_buf);
print("\n");
    // Write the packet size to start transmission
    print("[TX] Writing to TSD register NOW...\n");
    
    // Try the write with explicit flushing
    // Instead of: outl(RTL8139->io_base + tsd_reg, tsd_value);
print("[TX] Writing TSD byte-by-byte...\n");
outb(RTL8139->io_base + tsd_reg + 0, tsd_value & 0xFF);
print("  Byte 0 OK\n");
outb(RTL8139->io_base + tsd_reg + 1, (tsd_value >> 8) & 0xFF);
print("  Byte 1 OK\n");
outb(RTL8139->io_base + tsd_reg + 2, (tsd_value >> 16) & 0xFF);
print("  Byte 2 OK\n");
outb(RTL8139->io_base + tsd_reg + 3, (tsd_value >> 24) & 0xFF);
print("  Byte 3 OK\n");
    // Force a read to ensure write completed (PCI posting)
    print("[TX] Write issued, forcing flush...\n");
    volatile uint32_t flush = inl(RTL8139->io_base + tsd_reg);
    print("[TX] Flush complete, read back: 0x");
    itoa(flush, debug_buf, 16);
    print(debug_buf);
    print("\n");
    
    // Small delay
    print("[TX] Waiting for hardware to process...\n");
    for (volatile int i = 0; i < 10000; i++);
    print("[TX] Delay complete\n");
    
    // Check TSD after write
    print("[TX] Reading TSD after transmission start...\n");
    uint32_t tsd_after = inl(RTL8139->io_base + tsd_reg);
    print("[TX] TSD after write: 0x");
    itoa(tsd_after, debug_buf, 16);
    print(debug_buf);
    print("\n");
    
    print("[TX] TSD after write bits:\n");
    print("      OWN (bit 13): ");
    print((tsd_after & RTL8139_TSD_OWN) ? "SET" : "CLEAR");
    print("\n      TOK (bit 15): ");
    print((tsd_after & RTL8139_TSD_TOK) ? "SET" : "CLEAR");
    print("\n      TUN (bit 14): ");
    print((tsd_after & RTL8139_TSD_TUN) ? "SET" : "CLEAR");
    print("\n      Size: ");
    itoa(tsd_after & RTL8139_TSD_SIZE_MASK, debug_buf, 10);
    print(debug_buf);
    print(" bytes\n");
    
    // === UPDATE DESCRIPTOR INDEX ===
    uint8_t old_desc = tx_descriptor;
    tx_descriptor = (tx_descriptor + 1) % 4;
    
    print("[TX] Moved from descriptor ");
    itoa(old_desc, debug_buf, 10);
    print(debug_buf);
    print(" to ");
    itoa(tx_descriptor, debug_buf, 10);
    print(debug_buf);
    print("\n");
    
    print("[TX] Packet send initiated successfully\n\n");
    return true;
}

// Additional helper function to check all TX descriptors
void rtl8139_dump_tx_descriptors() {
    if (!RTL8139 || !RTL8139->initialized) {
        print("RTL8139 not initialized\n");
        return;
    }
    
    print("\n=== TX Descriptor Status ===\n");
    char buf[16];
    
    for (int i = 0; i < 4; i++) {
        uint16_t tsd_reg = RTL8139_REG_TSD0 + (i * 4);
        uint16_t tsad_reg = RTL8139_REG_TSAD0 + (i * 4);
        
        uint32_t tsd = inl(RTL8139->io_base + tsd_reg);
        uint32_t tsad = inl(RTL8139->io_base + tsad_reg);
        
        print("Descriptor ");
        itoa(i, buf, 10);
        print(buf);
        if (i == tx_descriptor) {
            print(" [NEXT]");
        }
        print(":\n");
        
        print("  TSAD: 0x");
        itoa(tsad, buf, 16);
        print(buf);
        print("\n  TSD:  0x");
        itoa(tsd, buf, 16);
        print(buf);
        print("\n  Status: ");
        
        if (tsd == 0) {
            print("UNUSED");
        } else if (tsd & RTL8139_TSD_TOK) {
            print("COMPLETE (TOK)");
        } else if (tsd & RTL8139_TSD_OWN) {
            print("AVAILABLE (OWN)");
        } else if (tsd & RTL8139_TSD_TUN) {
            print("ERROR (TUN)");
        } else {
            print("BUSY");
        }
        
        print("\n  Size: ");
        itoa(tsd & RTL8139_TSD_SIZE_MASK, buf, 10);
        print(buf);
        print(" bytes\n\n");
    }
    
    print("Current descriptor index: ");
    itoa(tx_descriptor, buf, 10);
    print(buf);
    print("\n");
    print("============================\n\n");
}

// Function to check if TX is actually working
void rtl8139_check_tx_status() {
    if (!RTL8139 || !RTL8139->initialized) {
        print("RTL8139 not initialized\n");
        return;
    }
    
    char buf[16];
    
    print("\n=== TX Status Check ===\n");
    
    uint8_t cmd = inb(RTL8139->io_base + RTL8139_REG_COMMAND);
    print("Command Register: 0x");
    itoa(cmd, buf, 16);
    print(buf);
    print("\n  TX Enabled: ");
    print((cmd & RTL8139_CMD_TX_ENABLE) ? "YES" : "NO");
    print("\n  RX Enabled: ");
    print((cmd & RTL8139_CMD_RX_ENABLE) ? "YES" : "NO");
    print("\n");
    
    uint32_t tcr = inl(RTL8139->io_base + RTL8139_REG_TCR);
    print("TCR (Transmit Config): 0x");
    itoa(tcr, buf, 16);
    print(buf);
    print("\n");
    
    uint16_t isr = inw(RTL8139->io_base + RTL8139_REG_ISR);
    print("ISR (Interrupt Status): 0x");
    itoa(isr, buf, 16);
    print(buf);
    print("\n  TOK (TX OK): ");
    print((isr & RTL8139_INT_TOK) ? "YES" : "NO");
    print("\n  TER (TX Error): ");
    print((isr & RTL8139_INT_TER) ? "YES" : "NO");
    print("\n");
    
    print("======================\n\n");
}

// Handle received packets
void rtl8139_handle_receive(void) {
    if (!RTL8139 || !RTL8139->rx_buffer) {
        return;
    }
    
    // Process all available packets
    while (true) {
        uint16_t capr = inw(RTL8139->io_base + RTL8139_REG_CAPR);
        uint16_t cbr = inw(RTL8139->io_base + RTL8139_REG_CBR);
        
        // No more packets
        if (capr == cbr) {
            break;
        }
        
        // Calculate position
        uint16_t current_pos = (capr + 0x10) % 8192;
        
        // Read packet header
        rx_packet_header_t* header = (rx_packet_header_t*)(RTL8139->rx_buffer + current_pos);
        
        // Check validity
        if (!(header->status & 0x01)) {
            break;
        }
        
        // Get packet length (subtract CRC)
        uint16_t packet_length = header->length - 4;
        
        // Validate length
        if (packet_length < 14 || packet_length > 1518) {
            // Skip invalid packet
            current_pos = (current_pos + header->length + 4 + 3) & ~3;
            if (current_pos >= 8192) current_pos -= 8192;
            outw(RTL8139->io_base + RTL8139_REG_CAPR, current_pos - 0x10);
            continue;
        }
        
        // Get packet data
        uint8_t* packet_data = RTL8139->rx_buffer + current_pos + sizeof(rx_packet_header_t);
        
        // Create temporary buffer
        uint8_t packet_buffer[1600];
        
        // Handle buffer wrap-around
        if (current_pos + sizeof(rx_packet_header_t) + packet_length > 8192) {
            uint16_t first_part = 8192 - (current_pos + sizeof(rx_packet_header_t));
            memcpy(packet_buffer, packet_data, first_part);
            memcpy(packet_buffer + first_part, RTL8139->rx_buffer, packet_length - first_part);
        } else {
            memcpy(packet_buffer, packet_data, packet_length);
        }
        
        // Pass to network stack
        netstack_process_packet(packet_buffer, packet_length);
        
        // Update CAPR
        current_pos = (current_pos + header->length + 4 + 3) & ~3;
        if (current_pos >= 8192) current_pos -= 8192;
        outw(RTL8139->io_base + RTL8139_REG_CAPR, current_pos - 0x10);
    }
}

// IRQ handler
void rtl8139_irq_handler(void) {
    if (!RTL8139 || !RTL8139->initialized) {
        return;
    }
    
    // Read interrupt status
    uint16_t isr = inw(RTL8139->io_base + RTL8139_REG_ISR);
    
    if (isr == 0) {
        return;
    }
    
    // Acknowledge interrupt
    outw(RTL8139->io_base + RTL8139_REG_ISR, isr);
    
    // Handle receive
    if (isr & RTL8139_INT_ROK) {
        rtl8139_handle_receive();
    }
    
    // Handle errors
    if (isr & (RTL8139_INT_RER | RTL8139_INT_TER | RTL8139_INT_RXOVW)) {
        if (isr & RTL8139_INT_RER) {
            warn("RTL8139: RX Error", __FILE__);
        }
        if (isr & RTL8139_INT_TER) {
            warn("RTL8139: TX Error", __FILE__);
        }
        if (isr & RTL8139_INT_RXOVW) {
            warn("RTL8139: RX Overflow", __FILE__);
        }
    }
}

// Poll for packets (use if interrupts not working)
void rtl8139_poll(void) {
    if (!RTL8139 || !RTL8139->initialized) {
        return;
    }
    
    // Check for packets
    uint16_t isr = inw(RTL8139->io_base + RTL8139_REG_ISR);
    
    if (isr & RTL8139_INT_ROK) {
        // Clear interrupt
        outw(RTL8139->io_base + RTL8139_REG_ISR, RTL8139_INT_ROK);
        
        // Handle packets
        rtl8139_handle_receive();
    }
}

// Print status
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
    char buffer[16];
    print("  Vendor ID: 0x");
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
        if (RTL8139->mac_address[i] < 16) print("0");
        print(buffer);
        if (i < 5) print(":");
    }
    print("\n  Status: ");
    
    terminal_setcolor(VGA_COLOR_GREEN);
    print("Initialized and Ready");
    terminal_setcolor(VGA_COLOR_WHITE);
    print("\n");
}

// Test functions
void rtl8139_test() {
    if (!RTL8139 || !RTL8139->initialized) {
        print("RTL8139 not initialized\n");
        return;
    }
    
    print("RTL8139 Test:\n");
    uint8_t cmd = inb(RTL8139->io_base + RTL8139_REG_COMMAND);
    
    print("Command Register: 0x");
    char buffer[8];
    itoa(cmd, buffer, 16);
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

void rtl8139_test_io_functions(void) {
    print("\n=== Testing I/O Functions ===\n");
    print("Test 1: outb to port 0x80\n");
    outb(0x80, 0x42);
    print("  outb completed\n");
    
    print("Test 2: inb from port 0x80\n");
    uint8_t val = inb(0x80);
    char buf[16];
    print("  Value: 0x");
    itoa(val, buf, 16);
    print(buf);
    print("\n");
    
    if (RTL8139) {
        print("\nTest 3: Reading RTL8139 MAC\n");
        print("  MAC: ");
        for (int i = 0; i < 6; i++) {
            uint8_t mac_byte = inb(RTL8139->io_base + i);
            if (mac_byte < 16) print("0");
            itoa(mac_byte, buf, 16);
            print(buf);
            if (i < 5) print(":");
        }
        print("\n");
    }
    
    print("=== I/O Tests Complete ===\n\n");
}

void rtl8139_verify_pci_config(void) {
    if (!RTL8139) {
        print("RTL8139 not detected\n");
        return;
    }
    
    print("\n=== PCI Configuration ===\n");
    
    for (uint8_t device = 0; device < 32; device++) {
        uint32_t vendor_device = pci_config_read(0, device, 0, PCI_VENDOR_ID);
        uint16_t vendor_id = vendor_device & 0xFFFF;
        uint16_t device_id = (vendor_device >> 16) & 0xFFFF;
        
        if (vendor_id == RTL8139_VENDOR_ID && device_id == RTL8139_DEVICE_ID) {
            uint32_t cmd = pci_config_read(0, device, 0, PCI_COMMAND);
            char buf[16];
            
            print("Command Register: 0x");
            itoa(cmd, buf, 16);
            print(buf);
            print("\n  I/O Space: ");
            print((cmd & 0x01) ? "YES" : "NO");
            print("\n  Bus Master: ");
            print((cmd & 0x04) ? "YES" : "NO");
            print("\n");
            
            if (!(cmd & 0x01)) {
                print("Enabling I/O Space...\n");
                cmd |= 0x05;
                pci_config_write(0, device, 0, PCI_COMMAND, cmd);
            }
            break;
        }
    }
    
    print("========================\n\n");
}

// Legacy receive function (for backward compatibility)
bool rtl8139_receive_packet(int8* buffer, int16* length) {
    if (!RTL8139 || !RTL8139->initialized || !buffer || !length) {
        return false;
    }
    
    if (!RTL8139->rx_buffer) {
        return false;
    }
    
    uint16_t capr = inw(RTL8139->io_base + RTL8139_REG_CAPR);
    uint16_t cbr = inw(RTL8139->io_base + RTL8139_REG_CBR);
    
    // No packet available
    if (capr == cbr) {
        return false;
    }
    
    uint16_t current_pos = (capr + 0x10) % 8192;
    
    // Read packet header
    rx_packet_header_t* header = (rx_packet_header_t*)(RTL8139->rx_buffer + current_pos);
    
    // Check validity
    if (!(header->status & 0x01)) {
        return false;
    }
    
    // Get packet length (subtract CRC)
    uint16_t packet_length = header->length - 4;
    
    // Validate
    if (packet_length > 1518 || packet_length < 14) {
        // Skip invalid packet
        current_pos = (current_pos + header->length + 4 + 3) & ~3;
        outw(RTL8139->io_base + RTL8139_REG_CAPR, current_pos - 0x10);
        return false;
    }
    
    // Copy packet data
    uint8_t* packet_data = RTL8139->rx_buffer + current_pos + sizeof(rx_packet_header_t);
    
    // Handle wrap-around
    if (current_pos + sizeof(rx_packet_header_t) + packet_length > 8192) {
        uint16_t first_part = 8192 - (current_pos + sizeof(rx_packet_header_t));
        memcpy(buffer, packet_data, first_part);
        memcpy(buffer + first_part, RTL8139->rx_buffer, packet_length - first_part);
    } else {
        memcpy(buffer, packet_data, packet_length);
    }
    
    *length = packet_length;
    
    // Update CAPR
    current_pos = (current_pos + header->length + 4 + 3) & ~3;
    if (current_pos >= 8192) {
        current_pos -= 8192;
    }
    
    outw(RTL8139->io_base + RTL8139_REG_CAPR, current_pos - 0x10);
    
    return true;
}

// RX statistics
void rtl8139_rx_stats() {
    if (!RTL8139 || !RTL8139->initialized) {
        print("RTL8139 not initialized\n");
        return;
    }
    
    uint16_t capr = inw(RTL8139->io_base + RTL8139_REG_CAPR);
    uint16_t cbr = inw(RTL8139->io_base + RTL8139_REG_CBR);
    
    print("RX Buffer Stats:\n");
    print("  CAPR: 0x");
    char buffer[16];
    itoa(capr, buffer, 16);
    print(buffer);
    print("\n  CBR: 0x");
    itoa(cbr, buffer, 16);
    print(buffer);
    print("\n");
    
    if (capr == cbr) {
        print("  Status: No packets\n");
    } else {
        print("  Status: Packets available\n");
    }
}

// Test TX
void rtl8139_test_tx() {
    if (!RTL8139 || !RTL8139->initialized) {
        print("RTL8139 not initialized\n");
        return;
    }
    
    uint8_t test_packet[64];
    memset(test_packet, 0, 64);
    
    // Broadcast MAC
    for (int i = 0; i < 6; i++) {
        test_packet[i] = 0xFF;
    }
    
    // Source MAC
    for (int i = 0; i < 6; i++) {
        test_packet[6 + i] = RTL8139->mac_address[i];
    }
    
    // EtherType
    test_packet[12] = 0x08;
    test_packet[13] = 0x00;
    
    // Payload
    for (int i = 14; i < 64; i++) {
        test_packet[i] = i - 14;
    }
    
    print("Sending test packet...\n");
    if (rtl8139_send_packet((int8*)test_packet, 64)) {
        print("Test packet sent\n");
    } else {
        print("Failed to send\n");
    }
}

// Cleanup
void rtl8139_cleanup() {
    if (RTL8139) {
        for (int i = 0; i < 4; i++) {
            tx_buffers[i] = NULL;
        }
        
        RTL8139->rx_buffer = NULL;
        RTL8139->tx_buffer = NULL;
        
        free(RTL8139);
        RTL8139 = NULL;
    }
}