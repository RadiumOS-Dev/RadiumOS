/**
 * @file rtl8139.h
 * @author Pradosh (pradoshgame@gmail.com)
 * @brief Header file for RTL8139 Networking Card driver
 * @version 0.1
 * @date 2023-12-05
 * 
 * @copyright Copyright (c) Pradosh 2023
 * 
 */

#ifndef RTL8139_H
#define RTL8139_H

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t int8;
typedef uint16_t int16;
typedef uint32_t int32;
typedef bool yes_no_t;

#define yes true
#define no false
#define null NULL

#define RTL8139_REG_MAC         0x00
#define RTL8139_REG_COMMAND     0x37
#define RTL8139_REG_TX_STATUS   0x10
#define RTL8139_REG_TX_ADDR     0x20
#define RTL8139_REG_RX_BUFFER   0x30
#define RTL8139_REG_RX_CONFIG   0x44
#define RTL8139_REG_TX_CONFIG   0x40
#define RTL8139_REG_INTERRUPT   0x3C
#define RTL8139_REG_INTERRUPT_MASK 0x3E
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

#define RTL8139_CMD_RESET       0x10
#define RTL8139_CMD_RX_ENABLE   0x08
#define RTL8139_CMD_TX_ENABLE   0x04
#define RTL8139_CMD_BUFFER_EMPTY 0x01

#define RTL8139_INT_RX_OK       0x01
#define RTL8139_INT_RX_ERR      0x02
#define RTL8139_INT_TX_OK       0x04
#define RTL8139_INT_TX_ERR      0x08
#define RTL8139_INT_RX_OVERFLOW 0x10
#define RTL8139_INT_LINK_CHANGE 0x20
#define RTL8139_INT_RX_FIFO_OVERFLOW 0x40
#define RTL8139_INT_TIMEOUT     0x4000

#define RTL8139_RX_CONFIG_ACCEPT_ALL    0x0F
#define RTL8139_RX_CONFIG_ACCEPT_PHYS   0x01
#define RTL8139_RX_CONFIG_ACCEPT_MULTI  0x02
#define RTL8139_RX_CONFIG_ACCEPT_BROAD  0x04
#define RTL8139_RX_CONFIG_ACCEPT_RUNT   0x08

#define RTL8139_TX_CONFIG_DEFAULT       0x03000700

#define RTL8139_TSD_OWN         (1 << 13)
#define RTL8139_TSD_TUN         (1 << 14)
#define RTL8139_TSD_TOK         (1 << 15)
#define RTL8139_TSD_SIZE_MASK   0x1FFF

#define RTL8139_RX_BUFFER_SIZE  8192
#define RTL8139_TX_BUFFER_SIZE  1536
#define RTL8139_MAX_PACKET_SIZE 1514

#define PCI_CONFIG_ADDRESS  0xCF8
#define PCI_CONFIG_DATA     0xCFC
#define RTL8139_VENDOR_ID   0x10EC
#define RTL8139_DEVICE_ID   0x8139
#define PCI_VENDOR_ID       0x00
#define PCI_DEVICE_ID       0x02
#define PCI_COMMAND         0x04
#define PCI_STATUS          0x06
#define PCI_BAR0            0x10
#define PCI_INTERRUPT_LINE  0x3C

struct rtl8139 {
    uint16_t io_base;
    uint8_t mac_address[6];
    uint8_t irq;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t* rx_buffer;
    uint8_t* tx_buffer;
    uint16_t rx_buffer_offset;
    uint16_t tx_buffer_offset;
    bool initialized;
};
// RTL8139 Register definitions (add these to your header)
#define RTL8139_REG_MAC         0x00    // MAC address (6 bytes)
#define RTL8139_REG_COMMAND     0x37    // Command register
#define RTL8139_REG_IMR         0x3C    // Interrupt Mask Register
#define RTL8139_REG_ISR         0x3E    // Interrupt Status Register
#define RTL8139_REG_TCR         0x40    // Transmit Configuration Register
#define RTL8139_REG_RCR         0x44    // Receive Configuration Register
#define RTL8139_REG_CONFIG1     0x52    // Configuration Register 1

// Command register bits
#define RTL8139_CMD_RESET       0x10    // Reset bit
#define RTL8139_CMD_RX_ENABLE   0x08    // Receiver Enable
#define RTL8139_CMD_TX_ENABLE   0x04    // Transmitter Enable

// Interrupt bits
#define RTL8139_INT_ROK         0x01    // Receive OK
#define RTL8139_INT_RER         0x02    // Receive Error
#define RTL8139_INT_TOK         0x04    // Transmit OK
#define RTL8139_INT_TER         0x08    // Transmit Error
#define RTL8139_INT_RXOVW       0x10    // RX Buffer Overflow
#define RTL8139_INT_PUN         0x20    // Packet Underrun
#define RTL8139_INT_FOVW        0x40    // FIFO Overflow

// Receive Configuration Register bits
#define RTL8139_RCR_AAP         0x01    // Accept All Packets
#define RTL8139_RCR_APM         0x02    // Accept Physical Match
#define RTL8139_RCR_AM          0x04    // Accept Multicast
#define RTL8139_RCR_AB          0x08    // Accept Broadcast
#define RTL8139_RCR_AR          0x10    // Accept Runt
#define RTL8139_RCR_AER         0x20    // Accept Error

// Transmit Configuration Register bits
#define RTL8139_TCR_IFG96       0x03000000  // Interframe Gap
#define RTL8139_TCR_MXDMA_2048  0x00000700  // Max DMA burst size
extern struct rtl8139* RTL8139;

uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
void pci_config_write(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);

void read_mac_address(void);
bool rtl8139_detect(void);
bool rtl8139_init(void);
bool rtl8139_init_tx_buffers(void);
bool rtl8139_send_packet(const int8* data, int16 length);
bool rtl8139_receive_packet(int8* buffer, int16* length);
bool rtl8139_tx_status(uint8_t descriptor);
void rtl8139_rx_stats(void);
void rtl8139_test_tx(void);
void rtl8139_test(void);
void rtl8139_print_status(void);
void rtl8139_enable_interrupts(uint16_t interrupt_mask);
void rtl8139_disable_interrupts(void);
uint16_t rtl8139_handle_interrupt(void);
void rtl8139_configure_receive(uint32_t config);
void rtl8139_configure_transmit(uint32_t config);
bool rtl8139_get_link_status(void);
void rtl8139_reset(void);
void rtl8139_cleanup(void);
void rtl8139_print_info(void);
void network_command(int argc, char* argv[]);

#define VGA_COLOR_GREEN     10
#define VGA_COLOR_RED       12
#define VGA_COLOR_WHITE     15

#endif // RTL8139_H
