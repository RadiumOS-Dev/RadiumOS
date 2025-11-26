#include "io.h"

// PCI Configuration Space Access
#define PCI_CONFIG_ADDRESS  0xCF8
#define PCI_CONFIG_DATA     0xCFC

void port_byte_out(uint16_t port, uint8_t data) {
    __asm__ volatile ("outb %0, %1" : : "a"(data), "Nd"(port));
}

// Function to write a byte to a specified I/O port
void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

// Function to read a byte from a specified I/O port
uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Function to write a word (16 bits) to a specified I/O port
void outw(uint16_t port, uint16_t value) {
    asm volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

// Function to read a word (16 bits) from a specified I/O port
uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Function to write a double word (32 bits) to a specified I/O port
void outl(uint16_t port, uint32_t value) {
    asm volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

// Function to read a double word (32 bits) from a specified I/O port
uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Function to read a byte from a specified I/O port 
uint8_t port_byte_in(uint16_t port) {
    uint8_t result;
    asm volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

// PCI Configuration Space Access Functions

// Helper function to create PCI configuration address
static uint32_t pci_config_address(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    return (uint32_t)(
        (1 << 31) |                    // Enable bit
        ((uint32_t)bus << 16) |        // Bus number
        ((uint32_t)device << 11) |     // Device number
        ((uint32_t)function << 8) |    // Function number
        (offset & 0xFC)                // Register offset (aligned to 4 bytes)
    );
}

// Read a 32-bit value from PCI configuration space
uint32_t pci_read_config_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = pci_config_address(bus, device, function, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

// Read a 16-bit value from PCI configuration space
uint16_t pci_read_config_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = pci_config_address(bus, device, function, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    
    // Read the 32-bit value and extract the appropriate 16-bit portion
    uint32_t data = inl(PCI_CONFIG_DATA);
    return (uint16_t)((data >> ((offset & 2) * 8)) & 0xFFFF);
}

// Read an 8-bit value from PCI configuration space
uint8_t pci_read_config_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = pci_config_address(bus, device, function, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    
    // Read the 32-bit value and extract the appropriate 8-bit portion
    uint32_t data = inl(PCI_CONFIG_DATA);
    return (uint8_t)((data >> ((offset & 3) * 8)) & 0xFF);
}

// Write a 32-bit value to PCI configuration space
void pci_write_config_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    uint32_t address = pci_config_address(bus, device, function, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

// Write a 16-bit value to PCI configuration space
void pci_write_config_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t value) {
    uint32_t address = pci_config_address(bus, device, function, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    
    // Read the current 32-bit value
    uint32_t data = inl(PCI_CONFIG_DATA);
    
    // Clear the target 16-bit portion and set the new value
    uint32_t shift = (offset & 2) * 8;
    data &= ~(0xFFFF << shift);
    data |= ((uint32_t)value << shift);
    
    // Write back the modified 32-bit value
    outl(PCI_CONFIG_DATA, data);
}

// Write an 8-bit value to PCI configuration space
void pci_write_config_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint8_t value) {
    uint32_t address = pci_config_address(bus, device, function, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    
    // Read the current 32-bit value
    uint32_t data = inl(PCI_CONFIG_DATA);
    
    // Clear the target 8-bit portion and set the new value
    uint32_t shift = (offset & 3) * 8;
    data &= ~(0xFF << shift);
    data |= ((uint32_t)value << shift);
    
    // Write back the modified 32-bit value
    outl(PCI_CONFIG_DATA, data);
}

// Check if a PCI device exists at the specified location
bool pci_device_exists(uint8_t bus, uint8_t device, uint8_t function) {
    uint16_t vendor_id = pci_read_config_word(bus, device, function, 0x00);
    return (vendor_id != 0xFFFF);
}

// Get PCI device vendor and device IDs
void pci_get_device_info(uint8_t bus, uint8_t device, uint8_t function, uint16_t *vendor_id, uint16_t *device_id) {
    if (vendor_id) {
        *vendor_id = pci_read_config_word(bus, device, function, 0x00);
    }
    if (device_id) {
        *device_id = pci_read_config_word(bus, device, function, 0x02);
    }
}

// Get PCI device class information
uint32_t pci_get_class_info(uint8_t bus, uint8_t device, uint8_t function) {
    return pci_read_config_dword(bus, device, function, 0x08);
}

// Enable PCI device (set command register bits)
void pci_enable_device(uint8_t bus, uint8_t device, uint8_t function) {
    uint16_t command = pci_read_config_word(bus, device, function, 0x04);
    command |= 0x07; // Enable I/O space, memory space, and bus mastering
    pci_write_config_word(bus, device, function, 0x04, command);
}

// Get PCI BAR (Base Address Register) value
uint32_t pci_get_bar(uint8_t bus, uint8_t device, uint8_t function, uint8_t bar_num) {
    if (bar_num > 5) return 0; // Invalid BAR number
    uint8_t bar_offset = 0x10 + (bar_num * 4);
    return pci_read_config_dword(bus, device, function, bar_offset);
}

// Set PCI BAR value
void pci_set_bar(uint8_t bus, uint8_t device, uint8_t function, uint8_t bar_num, uint32_t value) {
    if (bar_num > 5) return; // Invalid BAR number
    uint8_t bar_offset = 0x10 + (bar_num * 4);
    pci_write_config_dword(bus, device, function, bar_offset, value);
}

void io_wait(void) {
    // Port 0x80 is used for POST codes on many systems
    // Writing to it creates a small delay (~1μs) which is useful for:
    // - Giving slow hardware time to respond
    // - Ensuring proper timing between I/O operations
    // - Preventing race conditions with hardware
    asm volatile ("outb %%al, $0x80" : : "a"(0));
}