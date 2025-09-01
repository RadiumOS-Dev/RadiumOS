#ifndef WIFI_H
#define WIFI_H

#include <stdint.h>
#include <stdbool.h>

// PCI Configuration
#define INTEL_VENDOR_ID     0x8086
#define WIFI_DEVICE_ID_7260 0x08B1
#define WIFI_DEVICE_ID_7265 0x095A
#define WIFI_DEVICE_ID_8260 0x24F3
#define WIFI_DEVICE_ID_8265 0x24FD

// CSR (Control Status Register) offsets
#define CSR_HW_IF_CONFIG_REG    0x000
#define CSR_INT_COALESCING      0x004
#define CSR_INT                 0x008
#define CSR_INT_MASK            0x00C
#define CSR_FH_INT_STATUS       0x010
#define CSR_GPIO_IN             0x018
#define CSR_RESET               0x020
#define CSR_GP_CNTRL            0x024
#define CSR_HW_REV              0x028
#define CSR_EEPROM_REG          0x02C
#define CSR_EEPROM_GP           0x030
#define CSR_OTP_GP_REG          0x034
#define CSR_GIO_REG             0x03C
#define CSR_GP_UCODE_REG        0x048
#define CSR_GP_DRIVER_REG       0x050
#define CSR_UCODE_DRV_GP1       0x054
#define CSR_UCODE_DRV_GP2       0x058
#define CSR_LED_REG             0x094
#define CSR_DRAM_INT_TBL_REG    0x0A0
#define CSR_GIO_CHICKEN_BITS    0x100

// CSR_HW_IF_CONFIG_REG bits
#define CSR_HW_IF_CONFIG_REG_BIT_HAP_WAKE_L1A   (0x00080000)
#define CSR_HW_IF_CONFIG_REG_BIT_EEPROM_OWN_SEM (0x00200000)
#define CSR_HW_IF_CONFIG_REG_BIT_NIC_READY      (0x00400000)
#define CSR_HW_IF_CONFIG_REG_BIT_NIC_PREPARE_DONE (0x02000000)
#define CSR_HW_IF_CONFIG_REG_PREPARE            (0x08000000)

// CSR_RESET bits
#define CSR_RESET_REG_FLAG_NEVO_RESET           (0x00000001)
#define CSR_RESET_REG_FLAG_FORCE_NMI            (0x00000002)
#define CSR_RESET_REG_FLAG_SW_RESET             (0x00000080)
#define CSR_RESET_REG_FLAG_MASTER_DISABLED      (0x00000100)
#define CSR_RESET_REG_FLAG_STOP_MASTER          (0x00000200)

// CSR_GP_CNTRL bits
#define CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY   (0x00000001)
#define CSR_GP_CNTRL_REG_FLAG_INIT_DONE         (0x00000004)
#define CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ    (0x00000008)
#define CSR_GP_CNTRL_REG_FLAG_GOING_TO_SLEEP    (0x00000010)

// Interrupt bits
#define CSR_INT_BIT_FH_RX       (1 << 31)
#define CSR_INT_BIT_HW_ERR      (1 << 29)
#define CSR_INT_BIT_RX_PERIODIC (1 << 28)
#define CSR_INT_BIT_FH_TX       (1 << 27)
#define CSR_INT_BIT_SCD         (1 << 26)
#define CSR_INT_BIT_SW_ERR      (1 << 25)
#define CSR_INT_BIT_PAGING      (1 << 20)
#define CSR_INT_BIT_WAKEUP      (1 << 19)
#define CSR_INT_BIT_ALIVE       (1 << 0)

// FH (Flow Handler) registers
#define FH_MEM_LOWER_BOUND      0x1000
#define FH_MEM_UPPER_BOUND      0x2000

// RX registers
#define FH_MEM_RCSR_LOWER_BOUND         0x1400
#define FH_MEM_RCSR_UPPER_BOUND         0x1500
#define FH_MEM_RCSR_CHNL0_CONFIG_REG    0x1400
#define FH_MEM_RCSR_CHNL0_RBDCB_BASE_REG 0x1404
#define FH_MEM_RCSR_CHNL0_WPTR          0x1408
#define FH_MEM_RCSR_CHNL0_RPTR_ADDR_REG 0x140C

// TX registers
#define FH_MEM_TFDIB_REG1_ADDR_BITSHIFT 28
#define FH_MEM_TFDIB_DRAM_ADDR_LSB_MSK  0xFFFFFFFF
#define FH_TCSR_LOWER_BOUND             0x1640
#define FH_TCSR_UPPER_BOUND             0x1800

#define FH_TCSR_CHNL_TX_CONFIG_REG(_chnl) \
    (FH_TCSR_LOWER_BOUND + 0x20 * (_chnl))
#define FH_TCSR_CHNL_TX_CREDIT_REG(_chnl) \
    (FH_TCSR_LOWER_BOUND + 0x20 * (_chnl) + 0x4)
#define FH_TCSR_CHNL_TX_BUF_STS_REG(_chnl) \
    (FH_TCSR_LOWER_BOUND + 0x20 * (_chnl) + 0x8)

// EEPROM/OTP access
#define CSR_EEPROM_REG_READ_VALID_MSK   0x00000001
#define CSR_EEPROM_REG_BIT_CMD          0x00000002
#define CSR_EEPROM_REG_MSK_ADDR         0x0000FFFC
#define CSR_EEPROM_REG_MSK_DATA         0xFFFF0000

#define CSR_OTP_GP_REG_DEVICE_SELECT    0x00010000
#define CSR_OTP_GP_REG_OTP_ACCESS_MODE  0x00020000
#define CSR_OTP_GP_REG_ECC_CORR_STATUS_MSK 0x00100000
#define CSR_OTP_GP_REG_ECC_UNCORR_STATUS_MSK 0x00200000

// uCode communication
#define UCODE_VALID_OK      0x00000001
#define UCODE_CALIB_CFG_RX_BB_IDX   0
#define UCODE_CALIB_CFG_DC_IDX      1
#define UCODE_CALIB_CFG_LO_IDX      2
#define UCODE_CALIB_CFG_TX_IQ_IDX   3
#define UCODE_CALIB_CFG_RX_IQ_IDX   4
#define UCODE_CALIB_CFG_NOISE_IDX   5
#define UCODE_CALIB_CFG_CRYSTAL_IDX 6
#define UCODE_CALIB_CFG_TEMP_OFFSET_IDX 7
#define UCODE_CALIB_CFG_PAPD_IDX    8

// Command queue
#define IWL_MAX_CMD_SIZE 4096
#define IWL_CMD_QUEUE_NUM 4
#define IWL_TX_QUEUE_SIZE 256
#define IWL_RX_QUEUE_SIZE 256

// Host command opcodes
#define REPLY_ALIVE                 0x01
#define REPLY_ERROR                 0x02
#define REPLY_ECHO                  0x03
#define REPLY_RXON                  0x10
#define REPLY_RXON_ASSOC            0x11
#define REPLY_QOS_PARAM             0x13
#define REPLY_RXON_TIMING           0x14
#define REPLY_ADD_STA               0x18
#define REPLY_REMOVE_STA            0x19
#define REPLY_REMOVE_ALL_STA        0x1a
#define REPLY_WEPKEY                0x20
#define REPLY_3945_RX               0x1b
#define REPLY_TX                    0x1c
#define REPLY_RATE_SCALE            0x47
#define REPLY_LEDS_CMD              0x48
#define REPLY_TX_LINK_QUALITY_CMD   0x4e
#define REPLY_CHANNEL_SWITCH        0x72
#define REPLY_SPECTRUM_MEASUREMENT_CMD 0x74
#define REPLY_RX_PHY_CMD            0xc0
#define REPLY_RX_MPDU_CMD           0xc1
#define REPLY_RX_COMPRESSED_BA      0xc5
#define REPLY_COMPRESSED_BA         0xc5

// WiFi device states
typedef enum {
    WIFI_STATE_UNINITIALIZED,
    WIFI_STATE_INITIALIZING,
    WIFI_STATE_READY,
    WIFI_STATE_SCANNING,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_ERROR
} wifi_state_t;

// Command structure
typedef struct {
    uint8_t cmd;
    uint8_t flags;
    uint16_t len;
    uint32_t data[];
} __attribute__((packed)) iwl_cmd_t;

// RX/TX Buffer Descriptor
typedef struct {
    uint32_t addr;
    uint16_t len;
    uint16_t flags;
} __attribute__((packed)) iwl_tfd_tb_t;

// Transmit Frame Descriptor
typedef struct {
    uint8_t num_tbs;
    uint8_t reserved[3];
    iwl_tfd_tb_t tbs[20];
} __attribute__((packed)) iwl_tfd_t;

// Receive Buffer Descriptor
typedef struct {
    uint32_t addr;
    uint32_t reserved;
} __attribute__((packed)) iwl_rx_bd_t;

// Command queue structure
typedef struct {
    iwl_tfd_t *tfds;
    iwl_cmd_t **cmd_ptrs;
    uint32_t *meta;
    uint16_t write_ptr;
    uint16_t read_ptr;
    uint16_t n_bd;
    uint16_t n_window;
    uint32_t id;
    bool active;
} iwl_tx_queue_t;

// RX queue structure
typedef struct {
    iwl_rx_bd_t *bd;
    void **rx_buf;
    uint32_t read;
    uint32_t write;
    uint32_t free_count;
    uint32_t write_actual;
    bool need_update;
} iwl_rx_queue_t;

// WiFi device structure
typedef struct {
    // PCI information
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    
    // Memory mapped I/O
    volatile uint32_t *hw_base;
    uint32_t hw_phys_addr;
    uint32_t hw_len;
    
    // Device state
    wifi_state_t state;
    bool initialized;
    bool enabled;
    bool ucode_loaded;
    
    // Hardware information
    uint32_t hw_rev;
    uint8_t mac_address[6];
    uint16_t eeprom_size;
    uint16_t *eeprom_data;
    
    // Command and data queues
    iwl_tx_queue_t *txq;
    iwl_rx_queue_t rxq;
    uint32_t cmd_queue;
    
    // uCode
    void *ucode_code;
    void *ucode_data;
    void *ucode_init;
    void *ucode_init_data;
    void *ucode_boot;
    uint32_t ucode_code_len;
    uint32_t ucode_data_len;
    uint32_t ucode_init_len;
    uint32_t ucode_init_data_len;
    uint32_t ucode_boot_len;
    
    // Statistics
    uint32_t tx_packets;
    uint32_t rx_packets;
    uint32_t tx_errors;
    uint32_t rx_errors;
    
} wifi_device_t;

// Function prototypes
int wifi_init(void);
int wifi_probe_device(uint8_t bus, uint8_t device, uint8_t function);
int wifi_device_init(wifi_device_t *wifi_dev);
void wifi_device_cleanup(wifi_device_t *wifi_dev);

// Hardware access functions
uint32_t iwl_read32(wifi_device_t *dev, uint32_t offset);
void iwl_write32(wifi_device_t *dev, uint32_t offset, uint32_t value);
int iwl_poll_bit(wifi_device_t *dev, uint32_t addr, uint32_t bits, uint32_t mask, int timeout);

// Device control functions
int iwl_prepare_card_hw(wifi_device_t *dev);
int iwl_apm_init(wifi_device_t *dev);
void iwl_apm_stop(wifi_device_t *dev);
int iwl_start_hw(wifi_device_t *dev);
void iwl_stop_hw(wifi_device_t *dev);

// EEPROM/OTP functions
int iwl_eeprom_init(wifi_device_t *dev);
void iwl_eeprom_free(wifi_device_t *dev);
uint16_t iwl_eeprom_read16(wifi_device_t *dev, uint16_t addr);

// uCode functions
int iwl_load_ucode(wifi_device_t *dev);
int iwl_verify_ucode(wifi_device_t *dev);
int iwl_start_ucode(wifi_device_t *dev);

// Queue management
int iwl_tx_queue_init(wifi_device_t *dev, iwl_tx_queue_t *txq, int slots_num, uint32_t txq_id);
void iwl_tx_queue_free(wifi_device_t *dev, iwl_tx_queue_t *txq);
int iwl_rx_queue_alloc(wifi_device_t *dev);
void iwl_rx_queue_free(wifi_device_t *dev);

// Command interface
int iwl_send_cmd(wifi_device_t *dev, iwl_cmd_t *cmd);
int iwl_send_cmd_sync(wifi_device_t *dev, iwl_cmd_t *cmd);

// Interrupt handler
void iwl_interrupt_handler(wifi_device_t *dev);

// Global device pointer
extern wifi_device_t *g_wifi_device;

#endif // WIFI_H
