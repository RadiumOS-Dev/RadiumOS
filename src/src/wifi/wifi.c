#include "wifi.h"
#include "../utility/utility.h"
#include "../terminal/terminal.h"
#include "../timers/timer.h"
#include "../io/io.h"
#include "../memory/memory.h"
// OS-specific function declarations (implement these in your OS)


// Global device pointer
wifi_device_t *g_wifi_device = NULL;

// Hardware access functions
uint32_t iwl_read32(wifi_device_t *dev, uint32_t offset) {
    if (!dev || !dev->hw_base || offset >= dev->hw_len) {
        return 0xFFFFFFFF;
    }
    return dev->hw_base[offset / 4];
}

void iwl_write32(wifi_device_t *dev, uint32_t offset, uint32_t value) {
    if (!dev || !dev->hw_base || offset >= dev->hw_len) {
        return;
    }
    dev->hw_base[offset / 4] = value;
}

int iwl_poll_bit(wifi_device_t *dev, uint32_t addr, uint32_t bits, uint32_t mask, int timeout) {
    uint32_t start_time = get_time_ms();
    
    do {
        uint32_t val = iwl_read32(dev, addr);
        if ((val & mask) == (bits & mask)) {
            return 0;
        }
        delay_us(10);
    } while ((get_time_ms() - start_time) < timeout);
    
    return -1;
}

// Initialize WiFi subsystem
int wifi_init(void) {
    print("Initializing Intel WiFi driver...\n");
    
    // Scan PCI bus for Intel WiFi devices
    for (uint8_t bus = 0; bus < 256; bus++) {
        print("\nreached this point\n");
        for (uint8_t device = 0; device < 32; device++) {
            print("\nreached this point 2\n");
            for (uint8_t function = 0; function < 8; function++) {
                print("\nreached this point 3\n");
                uint16_t vendor_id = pci_read_config_word(bus, device, function, 0x00);
                if (vendor_id == 0xFFFF) continue;
                
                print("Found device: ");
                print_hex_byte(bus);
                print(":");
                print_hex_byte(device);
                print(".");
                print_hex_byte(function);
                print(" Vendor ID: ");
                print_hex(vendor_id);
                print("\n");
                
                if (vendor_id == INTEL_VENDOR_ID) {
                    if (wifi_probe_device(bus, device, function) == 0) {
                        print("Found Intel WiFi device at ");
                        print_hex_byte(bus);
                        print(":");
                        print_hex_byte(device);
                        print(".");
                        print_hex_byte(function);
                        print("\n");
                        return 0;
                    }
                }
            }
        }
    }
    
    print("No Intel WiFi devices found\n");
    return -1;
}


// Probe for supported WiFi device
int wifi_probe_device(uint8_t bus, uint8_t device, uint8_t function) {
    uint16_t device_id = pci_read_config_word(bus, device, function, 0x02);
    
    // Check if this is a supported Intel WiFi device
    switch (device_id) {
        case WIFI_DEVICE_ID_7260:
        case WIFI_DEVICE_ID_7265:
        case WIFI_DEVICE_ID_8260:
        case WIFI_DEVICE_ID_8265:
            break;
        default:
            return -1; // Unsupported device
    }
    
    // Allocate device structure
    wifi_device_t *wifi_dev = (wifi_device_t *)kmalloc(sizeof(wifi_device_t));
    if (!wifi_dev) {
        print("Failed to allocate memory for WiFi device\n");
        return -1;
    }
    
    memset(wifi_dev, 0, sizeof(wifi_device_t));
    
    // Initialize device structure
    wifi_dev->vendor_id = INTEL_VENDOR_ID;
    wifi_dev->device_id = device_id;
    wifi_dev->bus = bus;
    wifi_dev->device = device;
    wifi_dev->function = function;
    wifi_dev->state = WIFI_STATE_UNINITIALIZED;
    wifi_dev->cmd_queue = IWL_CMD_QUEUE_NUM;
    
    // Initialize the device
    if (wifi_device_init(wifi_dev) != 0) {
        free(wifi_dev);
        return -1;
    }
    
    g_wifi_device = wifi_dev;
    return 0;
}

// Initialize WiFi device
int wifi_device_init(wifi_device_t *wifi_dev) {
    print("Initializing WiFi device ");
    print_hex(wifi_dev->vendor_id);
    print(":");
    print_hex(wifi_dev->device_id);
    print("\n");
    
    wifi_dev->state = WIFI_STATE_INITIALIZING;
    
    // Read PCI BAR0 (hardware base address)
    uint32_t bar0 = pci_read_config_dword(wifi_dev->bus, wifi_dev->device, wifi_dev->function, 0x10);
    if (bar0 & 0x1) {
        print("BAR0 is not memory space\n");
        return -1;
    }
    
    wifi_dev->hw_phys_addr = bar0 & 0xFFFFFFF0;
    wifi_dev->hw_len = 0x8000; // 32KB typical size
    wifi_dev->hw_base = (volatile uint32_t *)map_physical_memory(wifi_dev->hw_phys_addr, wifi_dev->hw_len);
    if (!wifi_dev->hw_base) {
        print("Failed to map hardware memory\n");
        return -1;
    }
    
    // Enable PCI bus mastering and memory space
    uint16_t command = pci_read_config_word(wifi_dev->bus, wifi_dev->device, wifi_dev->function, 0x04);
    command |= 0x06; // Enable memory space and bus mastering
    pci_write_config_dword(wifi_dev->bus, wifi_dev->device, wifi_dev->function, 0x04, command);
    
    // Read hardware revision
    wifi_dev->hw_rev = iwl_read32(wifi_dev, CSR_HW_REV);
    print("Hardware revision: 0x");
    print_hex(wifi_dev->hw_rev);
    print("\n");
    
    // Prepare card hardware
    if (iwl_prepare_card_hw(wifi_dev) != 0) {
        print("Failed to prepare card hardware\n");
        wifi_device_cleanup(wifi_dev);
        return -1;
    }
    
    // Initialize APM (Always Power Management)
    if (iwl_apm_init(wifi_dev) != 0) {
        print("Failed to initialize APM\n");
        wifi_device_cleanup(wifi_dev);
        return -1;
    }
    
    // Initialize EEPROM
    if (iwl_eeprom_init(wifi_dev) != 0) {
        print("Failed to initialize EEPROM\n");
        wifi_device_cleanup(wifi_dev);
        return -1;
    }
    
    // Allocate queues
    wifi_dev->txq = (iwl_tx_queue_t *)kmalloc(sizeof(iwl_tx_queue_t) * 5);
    if (!wifi_dev->txq) {
        print("Failed to allocate TX queues\n");
        wifi_device_cleanup(wifi_dev);
        return -1;
    }
    
    if (iwl_rx_queue_alloc(wifi_dev) != 0) {
        print("Failed to allocate RX queue\n");
        wifi_device_cleanup(wifi_dev);
        return -1;
    }
    
    wifi_dev->initialized = true;
    wifi_dev->state = WIFI_STATE_READY;
    
    print("WiFi device initialized successfully\n");
    return 0;
}

// Cleanup WiFi device
void wifi_device_cleanup(wifi_device_t *wifi_dev) {
    if (!wifi_dev) return;
    
    iwl_stop_hw(wifi_dev);
    iwl_apm_stop(wifi_dev);
    
    if (wifi_dev->txq) {
        for (int i = 0; i < 5; i++) {
            iwl_tx_queue_free(wifi_dev, &wifi_dev->txq[i]);
        }
        free(wifi_dev->txq);
        wifi_dev->txq = NULL;
    }
    
    iwl_rx_queue_free(wifi_dev);
    iwl_eeprom_free(wifi_dev);
    
    if (wifi_dev->hw_base) {
        unmap_memory((void *)wifi_dev->hw_base, wifi_dev->hw_len);
        wifi_dev->hw_base = NULL;
    }
    
    wifi_dev->initialized = false;
    wifi_dev->state = WIFI_STATE_UNINITIALIZED;
}

// Prepare card hardware
int iwl_prepare_card_hw(wifi_device_t *dev) {
    print("Preparing card hardware...\n");
    
    // Set CSR_HW_IF_CONFIG_REG_PREPARE bit
    uint32_t hw_if_config = iwl_read32(dev, CSR_HW_IF_CONFIG_REG);
    hw_if_config |= CSR_HW_IF_CONFIG_REG_PREPARE;
    iwl_write32(dev, CSR_HW_IF_CONFIG_REG, hw_if_config);
    
    // Wait for NIC to be ready
    if (iwl_poll_bit(dev, CSR_HW_IF_CONFIG_REG, 
                     CSR_HW_IF_CONFIG_REG_BIT_NIC_PREPARE_DONE,
                     CSR_HW_IF_CONFIG_REG_BIT_NIC_PREPARE_DONE, 150) < 0) {
        print("Hardware preparation timeout\n");
        return -1;
    }
    
    print("Card hardware prepared\n");
    return 0;
}

// Initialize APM (Always Power Management)
int iwl_apm_init(wifi_device_t *dev) {
    print("Initializing APM...\n");
    
    // Disable L0S exit timer
    iwl_write32(dev, CSR_GIO_CHICKEN_BITS, 
                iwl_read32(dev, CSR_GIO_CHICKEN_BITS) | 0x00000001);
    
    // Set FH_UCODE_LOAD_STATUS to 0x1
    iwl_write32(dev, CSR_UCODE_DRV_GP1, 0x1);
    
    // Enable DMA
    iwl_write32(dev, CSR_RESET, 0);
    
    // Enable MAC clock
    uint32_t gp_cntrl = iwl_read32(dev, CSR_GP_CNTRL);
    gp_cntrl |= CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ;
    iwl_write32(dev, CSR_GP_CNTRL, gp_cntrl);
    
    // Wait for MAC clock to be ready
    if (iwl_poll_bit(dev, CSR_GP_CNTRL,
                     CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY,
                     CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY, 25) < 0) {
        print("MAC clock not ready\n");
        return -1;
    }
    
    print("APM initialized\n");
    return 0;
}

// Stop APM
void iwl_apm_stop(wifi_device_t *dev) {
    print("Stopping APM...\n");
    
    // Clear INIT_DONE bit
    iwl_write32(dev, CSR_RESET, CSR_RESET_REG_FLAG_SW_RESET);
    
    // Stop MAC access
    uint32_t gp_cntrl = iwl_read32(dev, CSR_GP_CNTRL);
    gp_cntrl &= ~CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ;
    iwl_write32(dev, CSR_GP_CNTRL, gp_cntrl);
    
    print("APM stopped\n");
}

// Start hardware
int iwl_start_hw(wifi_device_t *dev) {
    print("Starting hardware...\n");
    
    // Clear interrupts
    iwl_write32(dev, CSR_INT, 0xFFFFFFFF);
    iwl_write32(dev, CSR_FH_INT_STATUS, 0xFFFFFFFF);
    
    // Enable interrupts
    uint32_t int_mask = CSR_INT_BIT_HW_ERR | CSR_INT_BIT_SW_ERR | 
                       CSR_INT_BIT_FH_RX | CSR_INT_BIT_FH_TX |
                       CSR_INT_BIT_ALIVE;
    iwl_write32(dev, CSR_INT_MASK, int_mask);
    
    dev->enabled = true;
    print("Hardware started\n");
    return 0;
}

// Stop hardware
void iwl_stop_hw(wifi_device_t *dev) {
    print("Stopping hardware...\n");
    
    // Disable interrupts
    iwl_write32(dev, CSR_INT_MASK, 0x00000000);
    iwl_write32(dev, CSR_INT, 0xFFFFFFFF);
    iwl_write32(dev, CSR_FH_INT_STATUS, 0xFFFFFFFF);
    
    dev->enabled = false;
    print("Hardware stopped\n");
}

// Initialize EEPROM
int iwl_eeprom_init(wifi_device_t *dev) {
    print("Initializing EEPROM...\n");
    
    // Check if EEPROM is present
    uint32_t gp_cntrl = iwl_read32(dev, CSR_GP_CNTRL);
    if (!(gp_cntrl & CSR_GP_CNTRL_REG_FLAG_INIT_DONE)) {
        print("EEPROM not ready\n");
        return -1;
    }
    
    // Allocate EEPROM buffer
    dev->eeprom_size = 512; // Typical size
    dev->eeprom_data = (uint16_t *)kmalloc(dev->eeprom_size * sizeof(uint16_t));
    if (!dev->eeprom_data) {
        print("Failed to allocate EEPROM buffer\n");
        return -1;
    }
    
    // Read EEPROM data
    for (uint16_t addr = 0; addr < dev->eeprom_size; addr++) {
        dev->eeprom_data[addr] = iwl_eeprom_read16(dev, addr);
    }
    
    // Extract MAC address from EEPROM (typically at offset 0x15)
        // Extract MAC address from EEPROM (typically at offset 0x15)
    uint16_t mac_offset = 0x15;
    for (int i = 0; i < 3; i++) {
        uint16_t mac_word = dev->eeprom_data[mac_offset + i];
        dev->mac_address[i * 2] = mac_word & 0xFF;
        dev->mac_address[i * 2 + 1] = (mac_word >> 8) & 0xFF;
    }
    
    print("MAC Address: ");
    for (int i = 0; i < 6; i++) {
        print_hex_byte(dev->mac_address[i]);
        if (i < 5) print(":");
    }
    print("\n");
    
    print("EEPROM initialized\n");
    return 0;
}

// Free EEPROM
void iwl_eeprom_free(wifi_device_t *dev) {
    if (dev->eeprom_data) {
        free(dev->eeprom_data);
        dev->eeprom_data = NULL;
    }
}

// Read 16-bit value from EEPROM
uint16_t iwl_eeprom_read16(wifi_device_t *dev, uint16_t addr) {
    uint32_t reg_val;
    
    // Set address and read command
    reg_val = (addr << 1) | CSR_EEPROM_REG_BIT_CMD;
    iwl_write32(dev, CSR_EEPROM_REG, reg_val);
    
    // Wait for read to complete
    if (iwl_poll_bit(dev, CSR_EEPROM_REG,
                     CSR_EEPROM_REG_READ_VALID_MSK,
                     CSR_EEPROM_REG_READ_VALID_MSK, 10) < 0) {
        return 0xFFFF;
    }
    
    reg_val = iwl_read32(dev, CSR_EEPROM_REG);
    return (uint16_t)((reg_val & CSR_EEPROM_REG_MSK_DATA) >> 16);
}

// Load uCode
int iwl_load_ucode(wifi_device_t *dev) {
    print("Loading uCode...\n");
    
    // This is a simplified version - actual implementation would load
    // firmware from files and write to device memory
    
    // Set uCode loaded flag
    iwl_write32(dev, CSR_UCODE_DRV_GP1, UCODE_VALID_OK);
    
    dev->ucode_loaded = true;
    print("uCode loaded\n");
    return 0;
}

// Verify uCode
int iwl_verify_ucode(wifi_device_t *dev) {
    print("Verifying uCode...\n");
    
    uint32_t gp1 = iwl_read32(dev, CSR_UCODE_DRV_GP1);
    if (!(gp1 & UCODE_VALID_OK)) {
        print("uCode verification failed\n");
        return -1;
    }
    
    print("uCode verified\n");
    return 0;
}

// Start uCode
int iwl_start_ucode(wifi_device_t *dev) {
    print("Starting uCode...\n");
    
    // Clear INIT_DONE bit
    iwl_write32(dev, CSR_GP_CNTRL, 
                iwl_read32(dev, CSR_GP_CNTRL) & ~CSR_GP_CNTRL_REG_FLAG_INIT_DONE);
    
    // Start uCode
    iwl_write32(dev, CSR_RESET, 0);
    
    // Wait for INIT_DONE
    if (iwl_poll_bit(dev, CSR_GP_CNTRL,
                     CSR_GP_CNTRL_REG_FLAG_INIT_DONE,
                     CSR_GP_CNTRL_REG_FLAG_INIT_DONE, 100) < 0) {
        print("uCode start timeout\n");
        return -1;
    }
    
    print("uCode started\n");
    return 0;
}

// Initialize TX queue
int iwl_tx_queue_init(wifi_device_t *dev, iwl_tx_queue_t *txq, int slots_num, uint32_t txq_id) {
    // Allocate TFD array
    txq->tfds = (iwl_tfd_t *)kmalloc(sizeof(iwl_tfd_t) * slots_num);
    if (!txq->tfds) {
        print("Failed to allocate TFDs\n");
        return -1;
    }
    
    // Allocate command pointer array
    txq->cmd_ptrs = (iwl_cmd_t **)kmalloc(sizeof(iwl_cmd_t *) * slots_num);
    if (!txq->cmd_ptrs) {
        free(txq->tfds);
        print("Failed to allocate command pointers\n");
        return -1;
    }
    
    // Allocate metadata array
    txq->meta = (uint32_t *)kmalloc(sizeof(uint32_t) * slots_num);
    if (!txq->meta) {
        free(txq->tfds);
        free(txq->cmd_ptrs);
        print("Failed to allocate metadata\n");
        return -1;
    }
    
    memset(txq->tfds, 0, sizeof(iwl_tfd_t) * slots_num);
    memset(txq->cmd_ptrs, 0, sizeof(iwl_cmd_t *) * slots_num);
    memset(txq->meta, 0, sizeof(uint32_t) * slots_num);
    
    txq->n_bd = slots_num;
    txq->n_window = slots_num;
    txq->id = txq_id;
    txq->active = true;
    txq->read_ptr = 0;
    txq->write_ptr = 0;
    
    return 0;
}

// Free TX queue
void iwl_tx_queue_free(wifi_device_t *dev, iwl_tx_queue_t *txq) {
    if (!txq) return;
    
    if (txq->tfds) {
        free(txq->tfds);
        txq->tfds = NULL;
    }
    
    if (txq->cmd_ptrs) {
        for (int i = 0; i < txq->n_bd; i++) {
            if (txq->cmd_ptrs[i]) {
                free(txq->cmd_ptrs[i]);
            }
        }
        free(txq->cmd_ptrs);
        txq->cmd_ptrs = NULL;
    }
    
    if (txq->meta) {
        free(txq->meta);
        txq->meta = NULL;
    }
    
    txq->active = false;
}

// Allocate RX queue
int iwl_rx_queue_alloc(wifi_device_t *dev) {
    // Allocate RX buffer descriptors
    dev->rxq.bd = (iwl_rx_bd_t *)kmalloc(sizeof(iwl_rx_bd_t) * IWL_RX_QUEUE_SIZE);
    if (!dev->rxq.bd) {
        print("Failed to allocate RX buffer descriptors\n");
        return -1;
    }
    
    // Allocate RX buffer pointers
    dev->rxq.rx_buf = (void **)kmalloc(sizeof(void *) * IWL_RX_QUEUE_SIZE);
    if (!dev->rxq.rx_buf) {
        free(dev->rxq.bd);
        print("Failed to allocate RX buffer pointers\n");
        return -1;
    }
    
    memset(dev->rxq.bd, 0, sizeof(iwl_rx_bd_t) * IWL_RX_QUEUE_SIZE);
    memset(dev->rxq.rx_buf, 0, sizeof(void *) * IWL_RX_QUEUE_SIZE);
    
    dev->rxq.read = 0;
    dev->rxq.write = 0;
    dev->rxq.free_count = IWL_RX_QUEUE_SIZE;
    dev->rxq.write_actual = 0;
    dev->rxq.need_update = false;
    
    return 0;
}

// Free RX queue
void iwl_rx_queue_free(wifi_device_t *dev) {
    if (dev->rxq.bd) {
        free(dev->rxq.bd);
        dev->rxq.bd = NULL;
    }
    
    if (dev->rxq.rx_buf) {
        for (int i = 0; i < IWL_RX_QUEUE_SIZE; i++) {
            if (dev->rxq.rx_buf[i]) {
                free(dev->rxq.rx_buf[i]);
            }
        }
        free(dev->rxq.rx_buf);
        dev->rxq.rx_buf = NULL;
    }
}

// Send command
int iwl_send_cmd(wifi_device_t *dev, iwl_cmd_t *cmd) {
    if (!dev || !cmd || !dev->enabled) {
        return -1;
    }
    
    iwl_tx_queue_t *txq = &dev->txq[dev->cmd_queue];
    if (!txq->active) {
        return -1;
    }
    
    // Get next write slot
    uint16_t idx = txq->write_ptr;
    
    // Store command
    txq->cmd_ptrs[idx] = cmd;
    
    // Setup TFD
    iwl_tfd_t *tfd = &txq->tfds[idx];
    tfd->num_tbs = 1;
    tfd->tbs[0].addr = (uint32_t)cmd;
    tfd->tbs[0].len = sizeof(iwl_cmd_t) + cmd->len;
    
    // Update write pointer
    txq->write_ptr = (txq->write_ptr + 1) % txq->n_bd;
    
    // Ring doorbell
    iwl_write32(dev, FH_TCSR_CHNL_TX_CONFIG_REG(dev->cmd_queue), 
                txq->write_ptr);
    
    return 0;
}

// Send command synchronously
int iwl_send_cmd_sync(wifi_device_t *dev, iwl_cmd_t *cmd) {
    int ret = iwl_send_cmd(dev, cmd);
    if (ret != 0) {
        return ret;
    }
    
    // Wait for command completion (simplified)
    delay_ms(10);
    
    return 0;
}

// Interrupt handler
void iwl_interrupt_handler(wifi_device_t *dev) {
    if (!dev || !dev->enabled) {
        return;
    }
    
    // Read interrupt status
    uint32_t inta = iwl_read32(dev, CSR_INT);
    uint32_t inta_fh = iwl_read32(dev, CSR_FH_INT_STATUS);
    
    // Clear interrupts
    iwl_write32(dev, CSR_INT, inta);
    iwl_write32(dev, CSR_FH_INT_STATUS, inta_fh);
    
    // Handle specific interrupts
    if (inta & CSR_INT_BIT_HW_ERR) {
        print("WiFi hardware error interrupt\n");
        dev->state = WIFI_STATE_ERROR;
    }
    
    if (inta & CSR_INT_BIT_SW_ERR) {
        print("WiFi software error interrupt\n");
        dev->state = WIFI_STATE_ERROR;
    }
    
    if (inta & CSR_INT_BIT_FH_RX) {
        // Handle RX interrupt
        dev->rx_packets++;
    }
    
    if (inta & CSR_INT_BIT_FH_TX) {
        // Handle TX interrupt
        dev->tx_packets++;
    }
    
    if (inta & CSR_INT_BIT_ALIVE) {
        print("WiFi alive interrupt\n");
    }
}
