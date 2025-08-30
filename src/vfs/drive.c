#include "drive.h"
#include "../errors/error.h"
#include "../terminal/terminal.h"


// Global drive table
drive_info_t g_drives[MAX_DRIVES];
int g_drive_count = 0;
static drive_operation_result_t last_error = DRIVE_OP_SUCCESS;

// Placeholder for system timer (implement based on your timer system)
uint64_t get_system_time_ms(void) {
    // TODO: Implement based on your timer system
    static uint64_t fake_time = 0;
    return fake_time += 100; // Simulate 100ms increments
}
// Initialize drive subsystem
bool drive_init(void) {
    // Clear drive table
    memset(g_drives, 0, sizeof(g_drives));
    g_drive_count = 0;
    last_error = DRIVE_OP_SUCCESS;
    
    // Initialize hardware
    if (!drive_hw_init()) {
        HARDWARE_FAIL("Drive Controller", ERROR_HARDWARE_DISK);
        return false;
    }
    
    // Detect all drives
    g_drive_count = drive_detect_all();
    
    log_error("DRIVE_INIT", "Drive subsystem initialized");
    return true;
}

void drive_shutdown(void) {
    #ifdef DRIVE_ENABLE_CACHE
    drive_cache_shutdown();
    #endif
    
    // Reset all drives
    for (int i = 0; i < g_drive_count; i++) {
        if (g_drives[i].is_present) {
            drive_hw_reset(i);
        }
    }
    
    g_drive_count = 0;
    log_error("DRIVE_SHUTDOWN", "Drive subsystem shutdown");
}

// Detect all drives
int drive_detect_all(void) {
    int detected = 0;
    
    for (uint8_t i = 0; i < MAX_DRIVES; i++) {
        if (drive_detect(i)) {
            detected++;
        }
    }
    
    return detected;
}

// Detect a specific drive
bool drive_detect(uint8_t drive_number) {
    if (drive_number >= MAX_DRIVES) {
        return false;
    }
    
    drive_info_t* drive = &g_drives[drive_number];
    memset(drive, 0, sizeof(drive_info_t));
    drive->drive_number = drive_number;
    
    // Try to detect hardware
    if (!drive_hw_detect(drive_number, drive)) {
        drive->is_present = false;
        drive->status = DRIVE_STATUS_NOT_READY;
        return false;
    }
    
    drive->is_present = true;
    drive->status = drive_hw_get_status(drive_number);
    
    // Calculate total size
    drive->geometry.total_size = drive->geometry.total_sectors * drive->geometry.bytes_per_sector;
    
    return true;
}

// Get drive count
int drive_get_count(void) {
    return g_drive_count;
}

// Get drive information
drive_info_t* drive_get_info(uint8_t drive_number) {
    if (drive_number >= MAX_DRIVES || !g_drives[drive_number].is_present) {
        return NULL;
    }
    return &g_drives[drive_number];
}

// Read multiple sectors
drive_operation_result_t drive_read_sectors(uint8_t drive, uint64_t lba, uint32_t sector_count, void* buffer) {
    if (!drive_validate_parameters(drive, lba, sector_count)) {
        drive_set_last_error(DRIVE_OP_ERROR_INVALID_PARAMS);
        return DRIVE_OP_ERROR_INVALID_PARAMS;
    }
    
    if (!drive_is_ready(drive)) {
        drive_set_last_error(DRIVE_OP_ERROR_NOT_READY);
        return DRIVE_OP_ERROR_NOT_READY;
    }
    
    #ifdef DRIVE_ENABLE_CACHE
    // Try cache first for single sector reads
    if (sector_count == 1 && drive_cache_read(drive, lba, buffer)) {
        return DRIVE_OP_SUCCESS;
    }
    #endif
    
    drive_operation_result_t result = drive_hw_read(drive, lba, sector_count, buffer);
    drive_set_last_error(result);
    
    #ifdef DRIVE_ENABLE_CACHE
    // Cache single sector reads
    if (result == DRIVE_OP_SUCCESS && sector_count == 1) {
        drive_cache_write(drive, lba, buffer);
    }
    #endif
    
    return result;
}

// Write multiple sectors
drive_operation_result_t drive_write_sectors(uint8_t drive, uint64_t lba, uint32_t sector_count, const void* buffer) {
    if (!drive_validate_parameters(drive, lba, sector_count)) {
        drive_set_last_error(DRIVE_OP_ERROR_INVALID_PARAMS);
        return DRIVE_OP_ERROR_INVALID_PARAMS;
    }
    
    if (!drive_is_ready(drive)) {
        drive_set_last_error(DRIVE_OP_ERROR_NOT_READY);
        return DRIVE_OP_ERROR_NOT_READY;
    }
    
    drive_operation_result_t result = drive_hw_write(drive, lba, sector_count, buffer);
    drive_set_last_error(result);
    
    #ifdef DRIVE_ENABLE_CACHE
    // Update cache for single sector writes
    if (result == DRIVE_OP_SUCCESS && sector_count == 1) {
        drive_cache_write(drive, lba, buffer);
    }
    #endif
    
    return result;
}

// Read single sector
drive_operation_result_t drive_read_sector(uint8_t drive, uint64_t lba, void* buffer) {
    return drive_read_sectors(drive, lba, 1, buffer);
}

// Write single sector
drive_operation_result_t drive_write_sector(uint8_t drive, uint64_t lba, const void* buffer) {
    return drive_write_sectors(drive, lba, 1, buffer);
}

// Get drive status
drive_status_t drive_get_status(uint8_t drive_number) {
    if (drive_number >= MAX_DRIVES || !g_drives[drive_number].is_present) {
        return DRIVE_STATUS_UNKNOWN;
    }
    
    g_drives[drive_number].status = drive_hw_get_status(drive_number);
    return g_drives[drive_number].status;
}

// Check if drive is ready
bool drive_is_ready(uint8_t drive_number) {
    drive_status_t status = drive_get_status(drive_number);
    return status == DRIVE_STATUS_READY;
}

// Check if drive is present
bool drive_is_present(uint8_t drive_number) {
    if (drive_number >= MAX_DRIVES) {
        return false;
    }
    return g_drives[drive_number].is_present;
}

// Reset drive
drive_operation_result_t drive_reset(uint8_t drive_number) {
    if (!drive_is_present(drive_number)) {
        return DRIVE_OP_ERROR_INVALID_DRIVE;
    }
    
    drive_operation_result_t result = drive_hw_reset(drive_number);
    drive_set_last_error(result);
    return result;
}

// Flush drive cache
drive_operation_result_t drive_flush_cache(uint8_t drive_number) {
    if (!drive_is_present(drive_number)) {
        return DRIVE_OP_ERROR_INVALID_DRIVE;
    }
    
    #ifdef DRIVE_ENABLE_CACHE
    drive_cache_flush(drive_number);
    #endif
    
    return DRIVE_OP_SUCCESS;
}

// Get sector count
uint64_t drive_get_sector_count(uint8_t drive_number) {
    if (!drive_is_present(drive_number)) {
        return 0;
    }
    return g_drives[drive_number].geometry.total_sectors;
}

// Get sector size
uint32_t drive_get_sector_size(uint8_t drive_number) {
    if (!drive_is_present(drive_number)) {
        return 0;
    }
    return g_drives[drive_number].geometry.bytes_per_sector;
}

// Get drive capacity
uint64_t drive_get_capacity(uint8_t drive_number) {
    if (!drive_is_present(drive_number)) {
        return 0;
    }
    return g_drives[drive_number].geometry.total_size;
}

// CHS to LBA conversion
uint64_t drive_chs_to_lba(uint8_t drive, uint32_t cylinder, uint32_t head, uint32_t sector) {
    if (!drive_is_present(drive)) {
        return 0;
    }
    
    drive_info_t* info = &g_drives[drive];
    return (cylinder * info->geometry.heads + head) * info->geometry.sectors_per_track + (sector - 1);
}

// LBA to CHS conversion (continued)
void drive_lba_to_chs(uint8_t drive, uint64_t lba, uint32_t* cylinder, uint32_t* head, uint32_t* sector) {
    if (!drive_is_present(drive) || !cylinder || !head || !sector) {
        return;
    }
    
    drive_info_t* info = &g_drives[drive];
    
    *sector = (lba % info->geometry.sectors_per_track) + 1;
    *head = (lba / info->geometry.sectors_per_track) % info->geometry.heads;
    *cylinder = lba / (info->geometry.heads * info->geometry.sectors_per_track);
}

// Get drive type string
const char* drive_get_type_string(drive_type_t type) {
    switch (type) {
        case DRIVE_TYPE_FLOPPY:   return "Floppy Disk";
        case DRIVE_TYPE_HDD:      return "Hard Disk";
        case DRIVE_TYPE_CDROM:    return "CD-ROM";
        case DRIVE_TYPE_USB:      return "USB Drive";
        case DRIVE_TYPE_RAMDISK:  return "RAM Disk";
        default:                  return "Unknown";
    }
}

// Get status string
const char* drive_get_status_string(drive_status_t status) {
    switch (status) {
        case DRIVE_STATUS_READY:     return "Ready";
        case DRIVE_STATUS_NOT_READY: return "Not Ready";
        case DRIVE_STATUS_ERROR:     return "Error";
        case DRIVE_STATUS_BUSY:      return "Busy";
        case DRIVE_STATUS_NO_MEDIA:  return "No Media";
        default:                     return "Unknown";
    }
}

// Get operation result string
const char* drive_get_operation_result_string(drive_operation_result_t result) {
    switch (result) {
        case DRIVE_OP_SUCCESS:                return "Success";
        case DRIVE_OP_ERROR_INVALID_DRIVE:    return "Invalid Drive";
        case DRIVE_OP_ERROR_NOT_READY:        return "Drive Not Ready";
        case DRIVE_OP_ERROR_TIMEOUT:          return "Timeout";
        case DRIVE_OP_ERROR_BAD_SECTOR:       return "Bad Sector";
        case DRIVE_OP_ERROR_WRITE_PROTECTED:  return "Write Protected";
        case DRIVE_OP_ERROR_NO_MEDIA:         return "No Media";
        case DRIVE_OP_ERROR_HARDWARE:         return "Hardware Error";
        case DRIVE_OP_ERROR_INVALID_PARAMS:   return "Invalid Parameters";
        default:                              return "Unknown Error";
    }
}

// Print drive information
void drive_print_info(uint8_t drive_number) {
    if (!drive_is_present(drive_number)) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
        print("Drive ");
        print_uint(drive_number);
        print(": Not present\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        return;
    }
    
    drive_info_t* drive = &g_drives[drive_number];
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    print("Drive ");
    print_uint(drive_number);
    print(" Information:\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    
    print("  Type: ");
    print(drive_get_type_string(drive->type));
    print("\n");
    
    print("  Status: ");
    if (drive->status == DRIVE_STATUS_READY) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK));
    } else {
        terminal_setcolor(vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
    }
    print(drive_get_status_string(drive->status));
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    print("\n");
    
    if (strlen(drive->model) > 0) {
        print("  Model: ");
        print(drive->model);
        print("\n");
    }
    
    if (strlen(drive->serial) > 0) {
        print("  Serial: ");
        print(drive->serial);
        print("\n");
    }
    
    print("  Capacity: ");
    print_capacity(drive->geometry.total_size);
    print("\n");
    
    print("  Sectors: ");
    print_uint64(drive->geometry.total_sectors);
    print(" (");
    print_uint(drive->geometry.bytes_per_sector);
    print(" bytes/sector)\n");
    
    print("  Geometry: ");
    print_uint(drive->geometry.cylinders);
    print(" cylinders, ");
    print_uint(drive->geometry.heads);
    print(" heads, ");
    print_uint(drive->geometry.sectors_per_track);
    print(" sectors/track\n");
    
    print("  Features: ");
    if (drive->supports_lba) print("LBA ");
    if (drive->supports_dma) print("DMA ");
    if (drive->is_removable) print("Removable ");
    print("\n");
}

// Print all drives
void drive_print_all_drives(void) {
    terminal_setcolor(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    print("Detected Drives:\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    
    bool found_any = false;
    for (int i = 0; i < MAX_DRIVES; i++) {
        if (g_drives[i].is_present) {
            drive_print_info(i);
            print("\n");
            found_any = true;
        }
    }
    
    if (!found_any) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
        print("No drives detected.\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    }
}

// Validate parameters
bool drive_validate_parameters(uint8_t drive, uint64_t lba, uint32_t sector_count) {
    if (!drive_is_present(drive)) {
        return false;
    }
    
    if (sector_count == 0) {
        return false;
    }
    
    drive_info_t* info = &g_drives[drive];
    if (lba >= info->geometry.total_sectors) {
        return false;
    }
    
    if (lba + sector_count > info->geometry.total_sectors) {
        return false;
    }
    
    return true;
}

// Error handling
void drive_set_last_error(drive_operation_result_t error) {
    last_error = error;
}

drive_operation_result_t drive_get_last_error(void) {
    return last_error;
}

const char* drive_get_last_error_string(void) {
    return drive_get_operation_result_string(last_error);
}

// Hardware interface stubs (implement these based on your hardware)
bool drive_hw_init(void) {
    // TODO: Initialize ATA/SATA/SCSI controllers
    // This is a placeholder implementation
    return true;
}

bool drive_hw_detect(uint8_t drive_number, drive_info_t* info) {
    // TODO: Implement hardware-specific drive detection
    // This is a placeholder that simulates a basic hard drive
    
    if (drive_number >= 4) { // Simulate max 4 drives
        return false;
    }
    
    // Simulate drive 0 as a hard drive
    if (drive_number == 0) {
        info->type = DRIVE_TYPE_HDD;
        info->is_removable = false;
        info->supports_lba = true;
        info->supports_dma = true;
        info->max_transfer_sectors = 256;
        
        // Simulate geometry
        info->geometry.bytes_per_sector = 512;
        info->geometry.sectors_per_track = 63;
        info->geometry.heads = 16;
        info->geometry.cylinders = 1024;
        info->geometry.total_sectors = info->geometry.cylinders * 
                                      info->geometry.heads * 
                                      info->geometry.sectors_per_track;
        
        strcpy(info->model, "Simulated Hard Drive");
        strcpy(info->serial, "SIM123456789");
        strcpy(info->firmware, "1.0");
        
        return true;
    }
    
    return false;
}

drive_operation_result_t drive_hw_read(uint8_t drive, uint64_t lba, uint32_t sectors, void* buffer) {
    // TODO: Implement hardware-specific read operation
    // This is a placeholder that simulates successful reads
    
    if (!drive_is_present(drive)) {
        return DRIVE_OP_ERROR_INVALID_DRIVE;
    }
    
    // Simulate read delay
    for (volatile int i = 0; i < 1000; i++);
    
    // For simulation, fill buffer with pattern
    uint8_t* buf = (uint8_t*)buffer;
    for (uint32_t i = 0; i < sectors * 512; i++) {
        buf[i] = (uint8_t)(lba + i);
    }
    
    return DRIVE_OP_SUCCESS;
}

drive_operation_result_t drive_hw_write(uint8_t drive, uint64_t lba, uint32_t sectors, const void* buffer) {
    // TODO: Implement hardware-specific write operation
    // This is a placeholder that simulates successful writes
    
    if (!drive_is_present(drive)) {
        return DRIVE_OP_ERROR_INVALID_DRIVE;
    }
    
    // Simulate write delay
    for (volatile int i = 0; i < 2000; i++);
    
    return DRIVE_OP_SUCCESS;
}

drive_status_t drive_hw_get_status(uint8_t drive_number) {
    // TODO: Implement hardware-specific status check
    // This is a placeholder that returns ready for present drives
    
    if (!g_drives[drive_number].is_present) {
        return DRIVE_STATUS_NOT_READY;
    }
    
    return DRIVE_STATUS_READY;
}

drive_operation_result_t drive_hw_reset(uint8_t drive_number) {
    // TODO: Implement hardware-specific reset
    // This is a placeholder
    
    if (!drive_is_present(drive_number)) {
        return DRIVE_OP_ERROR_INVALID_DRIVE;
    }
    
    // Simulate reset delay
    for (volatile int i = 0; i < 10000; i++);
    
    return DRIVE_OP_SUCCESS;
}



#ifdef DRIVE_ENABLE_CACHE
// Drive caching implementation
static drive_cache_entry_t* cache_entries = NULL;
static uint32_t cache_size = 0;
static uint32_t cache_count = 0;

bool drive_cache_init(uint32_t size) {
    cache_entries = malloc(size * sizeof(drive_cache_entry_t));
    if (!cache_entries) {
        return false;
    }
    
    cache_size = size;
    cache_count = 0;
    memset(cache_entries, 0, size * sizeof(drive_cache_entry_t));
    
    return true;
}

void drive_cache_shutdown(void) {
    if (cache_entries) {
        drive_cache_flush_all();
        free(cache_entries);
        cache_entries = NULL;
    }
    cache_size = 0;
    cache_count = 0;
}

bool drive_cache_read(uint8_t drive, uint64_t lba, void* buffer) {
    if (!cache_entries) return false;
    
    for (uint32_t i = 0; i < cache_count; i++) {
        if (cache_entries[i].drive == drive && cache_entries[i].lba == lba) {
            memcpy(buffer, cache_entries[i].data, 512);
            cache_entries[i].access_count++;
            return true;
        }
    }
    
    return false;
}
    // Add new entry if space available
    if (cache_count < cache_size) {
        cache_entries[cache_count].drive = drive;
        cache_entries[cache_count].lba = lba;
        cache_entries[cache_count].data = malloc(512);
        if (!cache_entries[cache_count].data) {
            return false;
        }
        memcpy(cache_entries[cache_count].data, buffer, 512);
        cache_entries[cache_count].dirty = true;
        cache_entries[cache_count].access_count = 1;
        cache_count++;
        return true;
    }
    
    // Replace least recently used entry
    uint32_t lru_index = 0;
    uint32_t min_access = cache_entries[0].access_count;
    
    for (uint32_t i = 1; i < cache_count; i++) {
        if (cache_entries[i].access_count < min_access) {
            min_access = cache_entries[i].access_count;
            lru_index = i;
        }
    }
    
    // Flush old entry if dirty
    if (cache_entries[lru_index].dirty) {
        drive_hw_write(cache_entries[lru_index].drive, 
                      cache_entries[lru_index].lba, 
                      1, 
                      cache_entries[lru_index].data);
    }
    
    // Replace with new entry
    cache_entries[lru_index].drive = drive;
    cache_entries[lru_index].lba = lba;
    memcpy(cache_entries[lru_index].data, buffer, 512);
    cache_entries[lru_index].dirty = true;
    cache_entries[lru_index].access_count = 1;
    
    return true;
}

void drive_cache_flush(uint8_t drive) {
    if (!cache_entries) return;
    
    for (uint32_t i = 0; i < cache_count; i++) {
        if (cache_entries[i].drive == drive && cache_entries[i].dirty) {
            drive_hw_write(cache_entries[i].drive, 
                          cache_entries[i].lba, 
                          1, 
                          cache_entries[i].data);
            cache_entries[i].dirty = false;
        }
    }
}

void drive_cache_flush_all(void) {
    if (!cache_entries) return;
    
    for (uint32_t i = 0; i < cache_count; i++) {
        if (cache_entries[i].dirty) {
            drive_hw_write(cache_entries[i].drive, 
                          cache_entries[i].lba, 
                          1, 
                          cache_entries[i].data);
            cache_entries[i].dirty = false;
        }
    }
}
#endif

// Compatibility functions for FAT filesystem
bool disk_read(uint8_t drive, uint32_t lba, uint32_t sectors, void* buffer) {
    drive_operation_result_t result = drive_read_sectors(drive, lba, sectors, buffer);
    return (result == DRIVE_OP_SUCCESS);
}

bool disk_write(uint8_t drive, uint32_t lba, uint32_t sectors, const void* buffer) {
    drive_operation_result_t result = drive_write_sectors(drive, lba, sectors, buffer);
    return (result == DRIVE_OP_SUCCESS);
}

// Advanced drive operations
drive_operation_result_t drive_verify_sectors(uint8_t drive, uint64_t lba, uint32_t sector_count) {
    if (!drive_validate_parameters(drive, lba, sector_count)) {
        return DRIVE_OP_ERROR_INVALID_PARAMS;
    }
    
    if (!drive_is_ready(drive)) {
        return DRIVE_OP_ERROR_NOT_READY;
    }
    
    // Allocate temporary buffer for verification
    uint8_t* temp_buffer = malloc(sector_count * 512);
    if (!temp_buffer) {
        return DRIVE_OP_ERROR_HARDWARE;
    }
    
    // Read sectors to verify they're accessible
    drive_operation_result_t result = drive_hw_read(drive, lba, sector_count, temp_buffer);
    
    free(temp_buffer);
    return result;
}

drive_operation_result_t drive_format_track(uint8_t drive, uint32_t cylinder, uint32_t head) {
    if (!drive_is_present(drive)) {
        return DRIVE_OP_ERROR_INVALID_DRIVE;
    }
    
    drive_info_t* info = &g_drives[drive];
    
    if (cylinder >= info->geometry.cylinders || head >= info->geometry.heads) {
        return DRIVE_OP_ERROR_INVALID_PARAMS;
    }
    
    // Calculate starting LBA for the track
    uint64_t start_lba = drive_chs_to_lba(drive, cylinder, head, 1);
    
    // Allocate buffer filled with zeros
    uint8_t* zero_buffer = malloc(info->geometry.sectors_per_track * 512);
    if (!zero_buffer) {
        return DRIVE_OP_ERROR_HARDWARE;
    }
    
    memset(zero_buffer, 0, info->geometry.sectors_per_track * 512);
    
    // Write zeros to entire track
    drive_operation_result_t result = drive_hw_write(drive, start_lba, 
                                                    info->geometry.sectors_per_track, 
                                                    zero_buffer);
    
    free(zero_buffer);
    return result;
}

// Drive benchmark functions
typedef struct {
    uint64_t read_time_ms;
    uint64_t write_time_ms;
    uint32_t read_speed_kbps;
    uint32_t write_speed_kbps;
    uint32_t errors;
} drive_benchmark_result_t;

drive_operation_result_t drive_benchmark(uint8_t drive, uint32_t test_sectors, drive_benchmark_result_t* result) {
    if (!drive_is_ready(drive) || !result || test_sectors == 0) {
        return DRIVE_OP_ERROR_INVALID_PARAMS;
    }
    
    memset(result, 0, sizeof(drive_benchmark_result_t));
    
    // Allocate test buffer
    uint8_t* test_buffer = malloc(test_sectors * 512);
    if (!test_buffer) {
        return DRIVE_OP_ERROR_HARDWARE;
    }
    
    // Fill buffer with test pattern
    for (uint32_t i = 0; i < test_sectors * 512; i++) {
        test_buffer[i] = (uint8_t)(i & 0xFF);
    }
    
    // Get starting time (you'll need to implement a timer)
    uint64_t start_time = get_system_time_ms();
    
    // Perform read test
    for (uint32_t i = 0; i < 10; i++) {
        if (drive_read_sectors(drive, i * test_sectors, test_sectors, test_buffer) != DRIVE_OP_SUCCESS) {
            result->errors++;
        }
    }
    
    uint64_t read_end_time = get_system_time_ms();
    result->read_time_ms = read_end_time - start_time;
    
    // Perform write test (be careful with this!)
    uint64_t write_start_time = get_system_time_ms();
    
    for (uint32_t i = 0; i < 10; i++) {
        if (drive_write_sectors(drive, i * test_sectors, test_sectors, test_buffer) != DRIVE_OP_SUCCESS) {
            result->errors++;
        }
    }
    
    uint64_t write_end_time = get_system_time_ms();
    result->write_time_ms = write_end_time - write_start_time;
    
    // Calculate speeds (KB/s)
    uint32_t total_kb = (test_sectors * 512 * 10) / 1024;
    
    if (result->read_time_ms > 0) {
        result->read_speed_kbps = (total_kb * 1000) / result->read_time_ms;
    }
    
    if (result->write_time_ms > 0) {
        result->write_speed_kbps = (total_kb * 1000) / result->write_time_ms;
    }
    
    free(test_buffer);
    return DRIVE_OP_SUCCESS;
}

// Drive health monitoring
typedef struct {
    uint32_t power_on_hours;
    uint32_t power_cycle_count;
    uint32_t reallocated_sectors;
    uint32_t pending_sectors;
    uint32_t uncorrectable_errors;
    uint8_t temperature_celsius;
    bool health_ok;
} drive_health_info_t;

drive_operation_result_t drive_get_health_info(uint8_t drive, drive_health_info_t* health) {
    if (!drive_is_present(drive) || !health) {
        return DRIVE_OP_ERROR_INVALID_PARAMS;
    }
    
    // TODO: Implement SMART data reading for real hardware
    // This is a placeholder that returns simulated health data
    
    memset(health, 0, sizeof(drive_health_info_t));
    
    // Simulate health data
    health->power_on_hours = 1000;
    health->power_cycle_count = 100;
    health->reallocated_sectors = 0;
    health->pending_sectors = 0;
    health->uncorrectable_errors = 0;
    health->temperature_celsius = 35;
    health->health_ok = true;
    
    return DRIVE_OP_SUCCESS;
}


// Drive command interface for shell commands
void drive_command_info(int argc, char* argv[]) {
    if (argc < 2) {
        drive_print_all_drives();
        return;
    }
    
    uint8_t drive_num = 0;
    // Simple string to number conversion
    if (argv[1][0] >= '0' && argv[1][0] <= '9') {
        drive_num = argv[1][0] - '0';
    }
    
    drive_print_info(drive_num);
}

void drive_command_test(int argc, char* argv[]) {
    if (argc < 2) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
        print("Usage: drivetest <drive_number>\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        return;
    }
    
    uint8_t drive_num = argv[1][0] - '0';
    
    if (!drive_is_present(drive_num)) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
        print("Drive not present\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        return;
    }
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    print("Testing drive ");
    print_uint(drive_num);
    print("...\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    
    // Test read/write of sector 0
    uint8_t test_buffer[512];
    uint8_t backup_buffer[512];
    
    // Backup original sector
    if (drive_read_sector(drive_num, 0, backup_buffer) == DRIVE_OP_SUCCESS) {
        print("✓ Read test passed\n");
        
        // Test write (restore immediately)
        if (drive_write_sector(drive_num, 0, backup_buffer) == DRIVE_OP_SUCCESS) {
            print("✓ Write test passed\n");
        } else {
            terminal_setcolor(vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
            print("✗ Write test failed\n");
            terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        }
    } else {
        terminal_setcolor(vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
        print("✗ Read test failed\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    }
    
    // Health check
    drive_health_info_t health;
    if (drive_get_health_info(drive_num, &health) == DRIVE_OP_SUCCESS) {
        print("Health: ");
        if (health.health_ok) {
            terminal_setcolor(vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK));
            print("OK");
        } else {
            terminal_setcolor(vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
            print("WARNING");
        }
        terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        print("\n");
    }
}
