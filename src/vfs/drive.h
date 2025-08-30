#ifndef DRIVE_H
#define DRIVE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Drive types
typedef enum {
    DRIVE_TYPE_UNKNOWN = 0,
    DRIVE_TYPE_FLOPPY,
    DRIVE_TYPE_HDD,
    DRIVE_TYPE_CDROM,
    DRIVE_TYPE_USB,
    DRIVE_TYPE_RAMDISK
} drive_type_t;

// Drive status
typedef enum {
    DRIVE_STATUS_UNKNOWN = 0,
    DRIVE_STATUS_READY,
    DRIVE_STATUS_NOT_READY,
    DRIVE_STATUS_ERROR,
    DRIVE_STATUS_BUSY,
    DRIVE_STATUS_NO_MEDIA
} drive_status_t;

// Drive geometry
typedef struct {
    uint32_t cylinders;
    uint32_t heads;
    uint32_t sectors_per_track;
    uint32_t bytes_per_sector;
    uint64_t total_sectors;
    uint64_t total_size;
} drive_geometry_t;

// Drive information structure
typedef struct {
    uint8_t drive_number;
    drive_type_t type;
    drive_status_t status;
    drive_geometry_t geometry;
    char model[41];          // Drive model string
    char serial[21];         // Drive serial number
    char firmware[9];        // Firmware revision
    bool is_removable;
    bool is_present;
    bool supports_lba;
    bool supports_dma;
    uint32_t max_transfer_sectors;
} drive_info_t;

// Drive operation result
typedef enum {
    DRIVE_OP_SUCCESS = 0,
    DRIVE_OP_ERROR_INVALID_DRIVE,
    DRIVE_OP_ERROR_NOT_READY,
    DRIVE_OP_ERROR_TIMEOUT,
    DRIVE_OP_ERROR_BAD_SECTOR,
    DRIVE_OP_ERROR_WRITE_PROTECTED,
    DRIVE_OP_ERROR_NO_MEDIA,
    DRIVE_OP_ERROR_HARDWARE,
    DRIVE_OP_ERROR_INVALID_PARAMS
} drive_operation_result_t;

// Maximum number of drives supported
#define MAX_DRIVES 16

// Standard sector size
#define STANDARD_SECTOR_SIZE 512

// Drive interface functions
bool drive_init(void);
void drive_shutdown(void);

// Drive detection and enumeration
int drive_detect_all(void);
bool drive_detect(uint8_t drive_number);
int drive_get_count(void);
drive_info_t* drive_get_info(uint8_t drive_number);

// Drive operations
drive_operation_result_t drive_read_sectors(uint8_t drive, uint64_t lba, uint32_t sector_count, void* buffer);
drive_operation_result_t drive_write_sectors(uint8_t drive, uint64_t lba, uint32_t sector_count, const void* buffer);
drive_operation_result_t drive_read_sector(uint8_t drive, uint64_t lba, void* buffer);
drive_operation_result_t drive_write_sector(uint8_t drive, uint64_t lba, const void* buffer);

// Drive status and control
drive_status_t drive_get_status(uint8_t drive_number);
bool drive_is_ready(uint8_t drive_number);
bool drive_is_present(uint8_t drive_number);
drive_operation_result_t drive_reset(uint8_t drive_number);
drive_operation_result_t drive_flush_cache(uint8_t drive_number);

// Geometry and capacity
uint64_t drive_get_sector_count(uint8_t drive_number);
uint32_t drive_get_sector_size(uint8_t drive_number);
uint64_t drive_get_capacity(uint8_t drive_number);

// CHS to LBA conversion (for legacy drives)
uint64_t drive_chs_to_lba(uint8_t drive, uint32_t cylinder, uint32_t head, uint32_t sector);
void drive_lba_to_chs(uint8_t drive, uint64_t lba, uint32_t* cylinder, uint32_t* head, uint32_t* sector);

// Drive identification
const char* drive_get_type_string(drive_type_t type);
const char* drive_get_status_string(drive_status_t status);
const char* drive_get_operation_result_string(drive_operation_result_t result);

// Low-level hardware interface functions (to be implemented per platform)
bool drive_hw_init(void);
bool drive_hw_detect(uint8_t drive_number, drive_info_t* info);
drive_operation_result_t drive_hw_read(uint8_t drive, uint64_t lba, uint32_t sectors, void* buffer);
drive_operation_result_t drive_hw_write(uint8_t drive, uint64_t lba, uint32_t sectors, const void* buffer);
drive_status_t drive_hw_get_status(uint8_t drive_number);
drive_operation_result_t drive_hw_reset(uint8_t drive_number);
bool disk_read(uint8_t drive, uint32_t lba, uint32_t sectors, void* buffer);
bool disk_write(uint8_t drive, uint32_t lba, uint32_t sectors, const void* buffer);
// Utility functions
void drive_print_info(uint8_t drive_number);
void drive_print_all_drives(void);
bool drive_validate_parameters(uint8_t drive, uint64_t lba, uint32_t sector_count);

// Error handling
void drive_set_last_error(drive_operation_result_t error);
drive_operation_result_t drive_get_last_error(void);
const char* drive_get_last_error_string(void);



// Global drive table
extern drive_info_t g_drives[MAX_DRIVES];
extern int g_drive_count;

#endif // DRIVE_H
