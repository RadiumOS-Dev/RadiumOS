#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MEMORY_POOL_SIZE 1024000

// Memory map constants
#define MEMORY_MAP_MAX_ENTRIES 32
#define MEMORY_TYPE_AVAILABLE 1
#define MEMORY_TYPE_RESERVED 2
#define MEMORY_TYPE_ACPI_RECLAIMABLE 3
#define MEMORY_TYPE_ACPI_NVS 4
#define MEMORY_TYPE_BAD 5

typedef struct Block {
    size_t size;
    struct Block* next;
} Block;

// Updated MemoryInfo structure for physical memory tracking
typedef struct {
    uint64_t total_memory;      // Total physical RAM
    uint64_t used_memory;       // Used physical RAM
    uint32_t pool_total;        // Memory pool size
    uint32_t pool_used;         // Memory pool used
    uint32_t total_pages;       // Total pages available
    uint32_t allocated_pages;   // Pages currently allocated
} MemoryInfo;

// Memory map entry structure
typedef struct {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t acpi_extended_attributes;
} __attribute__((packed)) MemoryMapEntry;

// Memory map structure
typedef struct {
    uint32_t entry_count;
    MemoryMapEntry entries[MEMORY_MAP_MAX_ENTRIES];
} MemoryMap;

// Global memory variables
extern uint8_t memory_pool[MEMORY_POOL_SIZE];
extern size_t used_memory;
extern size_t total_memory;

// Core memory management functions
void memory_init(void);
void* memory_alloc(size_t size);
void memory_free(void* ptr);
MemoryInfo get_memory_info(void);

// Physical memory detection and initialization
void init_physical_memory(void);
int detect_memory_e820(MemoryMap* memory_map);
uint32_t detect_memory_simple(void);

// Physical page management functions
uint32_t allocate_physical_page(void);
void free_physical_page(uint32_t physical_addr);
uint32_t allocate_contiguous_pages(uint32_t num_pages);
void free_contiguous_pages(uint32_t physical_addr, uint32_t num_pages);

// Page bitmap management
void set_page_allocated(uint32_t page_number);
void set_page_free(uint32_t page_number);
bool is_page_allocated(uint32_t page_number);

// Memory mapping functions
void* map_physical_memory(uint32_t phys_addr, uint32_t size);
void unmap_memory(void* virt_addr, uint32_t size);

// Additional memory mapping utilities
uint32_t virtual_to_physical(void* virt_addr);
bool is_mapped(void* virt_addr);
int map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);
void unmap_page(uint32_t virtual_addr);
void enable_paging(void);

// Memory information and statistics
void print_memory_info(void);
void get_memory_stats(uint32_t* total_kb, uint32_t* used_kb, uint32_t* free_kb);

// Page flags for map_page function
#define PAGE_PRESENT        0x1
#define PAGE_WRITABLE       0x2
#define PAGE_USER           0x4
#define PAGE_WRITETHROUGH   0x8
#define PAGE_CACHE_DISABLE  0x10

// Page size constants
#define PAGE_SIZE 4096
#define PAGE_ALIGN(addr) (((addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
#define PAGE_DIRECTORY_INDEX(addr) (((addr) >> 22) & 0x3FF)
#define PAGE_TABLE_INDEX(addr) (((addr) >> 12) & 0x3FF)

// Memory utility macros
#define BYTES_TO_KB(bytes) ((bytes) / 1024)
#define BYTES_TO_MB(bytes) ((bytes) / (1024 * 1024))
#define KB_TO_BYTES(kb) ((kb) * 1024)
#define MB_TO_BYTES(mb) ((mb) * 1024 * 1024)

// Memory alignment macros
#define ALIGN_UP(addr, align) (((addr) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(addr, align) ((addr) & ~((align) - 1))
#define IS_ALIGNED(addr, align) (((addr) & ((align) - 1)) == 0)

// Physical memory address ranges (common x86 layout)
#define MEMORY_HOLE_START   0xA0000     // VGA memory hole start
#define MEMORY_HOLE_END     0x100000    // End of memory hole (1MB)
#define KERNEL_VIRTUAL_BASE 0xC0000000  // 3GB virtual address space start
#define USER_VIRTUAL_BASE   0x08048000  // Typical user space start

// Error codes for memory operations
#define MEMORY_SUCCESS          0
#define MEMORY_ERROR_NULL_PTR   -1
#define MEMORY_ERROR_NO_MEMORY  -2
#define MEMORY_ERROR_INVALID    -3
#define MEMORY_ERROR_NOT_MAPPED -4
#define MEMORY_ERROR_ALREADY_MAPPED -5

#endif // MEMORY_H