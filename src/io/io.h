#ifndef IO_H
#define IO_H

#include <stdint.h>
#include <stdbool.h>

// Basic I/O port functions
void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);
void outw(uint16_t port, uint16_t value);
uint16_t inw(uint16_t port);
void outl(uint16_t port, uint32_t value);
uint32_t inl(uint16_t port);
uint8_t port_byte_in(uint16_t port);

// PCI Configuration Space Access Functions
uint32_t pci_read_config_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
uint16_t pci_read_config_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
uint8_t pci_read_config_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
void pci_write_config_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);
void pci_write_config_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t value);
void pci_write_config_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint8_t value);

// PCI Helper Functions
bool pci_device_exists(uint8_t bus, uint8_t device, uint8_t function);
void pci_get_device_info(uint8_t bus, uint8_t device, uint8_t function, uint16_t *vendor_id, uint16_t *device_id);
uint32_t pci_get_class_info(uint8_t bus, uint8_t device, uint8_t function);
void pci_enable_device(uint8_t bus, uint8_t device, uint8_t function);
uint32_t pci_get_bar(uint8_t bus, uint8_t device, uint8_t function, uint8_t bar_num);
void pci_set_bar(uint8_t bus, uint8_t device, uint8_t function, uint8_t bar_num, uint32_t value);
void io_wait(void);
#endif // IO_H
