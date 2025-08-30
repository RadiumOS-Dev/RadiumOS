#include "memory.h"
#include "../terminal/terminal.h"
#include "../errors/error.h"

// Memory pool for kernel allocations
uint8_t memory_pool[MEMORY_POOL_SIZE]; 
size_t used_memory = 0; 
size_t total_memory = 10000;
static Block* free_list = NULL;

// Page directory and page table structures for memory mapping
static uint32_t *page_directory = NULL;
static uint32_t next_virtual_addr = 0xC0000000; // Start virtual addresses at 3GB

// Page size constants
#define PAGE_SIZE 4096
#define PAGE_ALIGN(addr) (((addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
#define PAGE_DIRECTORY_INDEX(addr) (((addr) >> 22) & 0x3FF)
#define PAGE_TABLE_INDEX(addr) (((addr) >> 12) & 0x3FF)

// Physical memory detection and tracking
#define MEMORY_MAP_MAX_ENTRIES 32
#define MEMORY_TYPE_AVAILABLE 1
#define MEMORY_TYPE_RESERVED 2
#define MEMORY_TYPE_ACPI_RECLAIMABLE 3
#define MEMORY_TYPE_ACPI_NVS 4
#define MEMORY_TYPE_BAD 5



// Global variables for physical memory tracking
static MemoryMap system_memory_map;
static uint64_t total_physical_memory = 0;
static uint64_t available_physical_memory = 0;
static uint64_t used_physical_memory = 0;

// Bitmap for tracking allocated pages
static uint8_t* page_bitmap = NULL;
static uint32_t total_pages = 0;
static uint32_t allocated_pages = 0;

// Function to detect memory using BIOS E820 (simplified for kernel mode)
int detect_memory_e820(MemoryMap* memory_map) {
    // Since we're in protected mode, we'll simulate or use bootloader-provided info
    // This is a placeholder - in real implementation, you'd get this from bootloader
    memory_map->entry_count = 3; // Example entries
    
    // Example memory map (adjust based on your system)
    memory_map->entries[0].base_addr = 0x0;
    memory_map->entries[0].length = 0x9FC00; // ~640KB
    memory_map->entries[0].type = MEMORY_TYPE_AVAILABLE;
    
    memory_map->entries[1].base_addr = 0x9FC00;
    memory_map->entries[1].length = 0x400; // BIOS area
    memory_map->entries[1].type = MEMORY_TYPE_RESERVED;
    
    memory_map->entries[2].base_addr = 0x100000; // 1MB
    memory_map->entries[2].length = 0x7F00000; // 127MB (example)
    memory_map->entries[2].type = MEMORY_TYPE_AVAILABLE;
    
    return memory_map->entry_count;
}

// Alternative: Simple memory detection
uint32_t detect_memory_simple(void) {
    // For now, return a reasonable default
    // In real implementation, you might read from CMOS or use other methods
    return 128 * 1024 * 1024; // 128MB default
}

// Set a page as allocated in the bitmap
void set_page_allocated(uint32_t page_number) {
    if (page_number >= total_pages || !page_bitmap) return;
    
    uint32_t byte_index = page_number / 8;
    uint32_t bit_index = page_number % 8;
    
    if (!(page_bitmap[byte_index] & (1 << bit_index))) {
        page_bitmap[byte_index] |= (1 << bit_index);
        allocated_pages++;
        used_physical_memory += PAGE_SIZE;
    }
}

// Set a page as free in the bitmap
void set_page_free(uint32_t page_number) {
    if (page_number >= total_pages || !page_bitmap) return;
    
    uint32_t byte_index = page_number / 8;
    uint32_t bit_index = page_number % 8;
    
    if (page_bitmap[byte_index] & (1 << bit_index)) {
        page_bitmap[byte_index] &= ~(1 << bit_index);
        allocated_pages--;
        used_physical_memory -= PAGE_SIZE;
    }
}

// Check if a page is allocated
bool is_page_allocated(uint32_t page_number) {
    if (page_number >= total_pages || !page_bitmap) return true;
    
    uint32_t byte_index = page_number / 8;
    uint32_t bit_index = page_number % 8;
    
    return (page_bitmap[byte_index] & (1 << bit_index)) != 0;
}

// Find and allocate a free physical page
uint32_t allocate_physical_page(void) {
    if (!page_bitmap) return 0;
    
    for (uint32_t page = 0; page < total_pages; page++) {
        if (!is_page_allocated(page)) {
            set_page_allocated(page);
            return page * PAGE_SIZE; // Return physical address
        }
    }
    
    return 0; // No free pages
}

// Free a physical page
void free_physical_page(uint32_t physical_addr) {
    uint32_t page_number = physical_addr / PAGE_SIZE;
    set_page_free(page_number);
}

// Initialize physical memory detection and tracking
void init_physical_memory(void) {
    print("Detecting physical memory...\n");
    
    // Try E820 first (more accurate)
    int entries = detect_memory_e820(&system_memory_map);
    
    if (entries > 0) {
        print("Memory map detected using E820:\n");
        
        total_physical_memory = 0;
        available_physical_memory = 0;
        
        for (int i = 0; i < entries; i++) {
            MemoryMapEntry* entry = &system_memory_map.entries[i];
            
            print("  Region ");
            print_decimal(i);
            print(": Base=0x");
            print_hex((uint32_t)entry->base_addr);
            print(", Length=0x");
            print_hex((uint32_t)entry->length);
            print(", Type=");
            print_decimal(entry->type);
            print("\n");
            
            total_physical_memory += entry->length;
            
            if (entry->type == MEMORY_TYPE_AVAILABLE) {
                available_physical_memory += entry->length;
            }
        }
    } else {
        // Fallback to simple detection
        print("Using simple memory detection...\n");
        total_physical_memory = detect_memory_simple();
        available_physical_memory = total_physical_memory - (1024 * 1024); // Reserve 1MB for system
    }
    
    print("Total physical memory: ");
    print_capacity(total_physical_memory);
    print(" bytes (");
    print_decimal(total_physical_memory / (1024 * 1024));
    print(" MB)\n");
    
    print("Available physical memory: ");
    print_capacity(available_physical_memory);
    print(" bytes (");
    print_decimal(available_physical_memory / (1024 * 1024));
    print(" MB)\n");
    
    // Initialize page bitmap for tracking allocated pages
    total_pages = available_physical_memory / PAGE_SIZE;
    uint32_t bitmap_size = (total_pages + 7) / 8; // Round up to nearest byte
    
    // We'll allocate the bitmap from our memory pool initially
    page_bitmap = (uint8_t*)memory_alloc(bitmap_size);
    if (page_bitmap) {
        // Clear bitmap (all pages initially free)
        for (uint32_t i = 0; i < bitmap_size; i++) {
            page_bitmap[i] = 0;
        }
        print("Page bitmap initialized for ");
        print_decimal(total_pages);
        print(" pages\n");
    } else {
        print("Warning: Could not allocate page bitmap\n");
    }
    
    allocated_pages = 0;
    used_physical_memory = 0;
}

void memory_init(void) {
    static int initialized = 0; 
    if(initialized) {
        handle_error("Memory already initialized.\n", "kernel");
        return;
    } 
    initialized = 1;
    
    print("Initializing memory pool...\n"); 
    used_memory = 0; 
    free_list = (Block*)memory_pool; 
    free_list->size = MEMORY_POOL_SIZE; 
    free_list->next = NULL;
    
    print("Memory pool initialized with size: "); 
    print_capacity(MEMORY_POOL_SIZE); 
    print(" bytes.\n");
    
    // Initialize physical memory detection
    init_physical_memory();
    
    // Initialize page directory for memory mapping
    page_directory = (uint32_t*)memory_alloc(PAGE_SIZE);
    if (page_directory) {
        // Clear page directory
        for (int i = 0; i < 1024; i++) {
            page_directory[i] = 0;
        }
        print("Page directory initialized\n");
    }
}

void* memory_alloc(size_t size) {
    Block *c = free_list, *p = NULL;
    size = (size + sizeof(Block) - 1) & ~(sizeof(Block) - 1);
    print("Requesting allocation of size: ");
    print_capacity(size);
    print("\n");

    while (c) {
        if (c->size >= size) {
            if (c->size > size + sizeof(Block)) {
                Block *n = (Block*)((uint8_t*)c + size + sizeof(Block));
                n->size = c->size - size - sizeof(Block);
                n->next = c->next;
                c->size = size;
                c->next = n;
            } else {
                if (p) {
                    p->next = c->next;
                } else {
                    free_list = c->next;
                }
            }
            used_memory += c->size;
            print("Allocated memory block of size: ");
            print_decimal(c->size);
            print("\n");
            return (void*)((uint8_t*)c + sizeof(Block));
        }
        p = c;
        c = c->next;
    }

    return NULL;
}

void memory_free(void* ptr) {
    if(!ptr) return; 
    Block* b = (Block*)((uint8_t*)ptr - sizeof(Block)); 
    used_memory -= b->size; 
    b->next = free_list; 
    free_list = b;
}

MemoryInfo get_memory_info(void) {
    MemoryInfo info; 
    
    // Return physical memory information
    info.total_memory = total_physical_memory; 
    info.used_memory = used_physical_memory;
    info.pool_total = MEMORY_POOL_SIZE;
    info.pool_used = used_memory;
    info.total_pages = total_pages;
    info.allocated_pages = allocated_pages;
    
    return info;
}

// Function to display detailed memory information
void print_memory_info(void) {
    print("=== Physical Memory Information ===\n");
    print("Total RAM: ");
        print_capacity(total_physical_memory);
    print(" bytes (");
    print_decimal(total_physical_memory / (1024 * 1024));
    print(" MB)\n");
    
    print("Available RAM: ");
    print_capacity(available_physical_memory);
    print(" bytes (");
    print_decimal(available_physical_memory / (1024 * 1024));
    print(" MB)\n");
    
    print("Used RAM: ");
    print_capacity(used_physical_memory);
    print(" bytes (");
    print_decimal(used_physical_memory / (1024 * 1024));
    print(" MB)\n");
    
    print("Free RAM: ");
    print_capacity(available_physical_memory - used_physical_memory);
    print(" bytes (");
    print_decimal((available_physical_memory - used_physical_memory) / (1024 * 1024));
    print(" MB)\n");
    
    print("Total Pages: ");
    print_decimal(total_pages);
    print("\n");
    
    print("Allocated Pages: ");
    print_decimal(allocated_pages);
    print("\n");
    
    print("Free Pages: ");
    print_decimal(total_pages - allocated_pages);
    print("\n");
    
    print("\n=== Memory Pool Information ===\n");
    print("Pool Size: ");
    print_capacity(MEMORY_POOL_SIZE);
    print(" bytes\n");
    
    print("Pool Used: ");
    print_capacity(used_memory);
    print(" bytes\n");
    
    print("Pool Free: ");
    print_capacity(MEMORY_POOL_SIZE - used_memory);
    print(" bytes\n");
    
    print("\n=== Memory Map ===\n");
    for (int i = 0; i < system_memory_map.entry_count; i++) {
        MemoryMapEntry* entry = &system_memory_map.entries[i];
        print("Region ");
        print_decimal(i);
        print(": 0x");
        print_hex((uint32_t)entry->base_addr);
        print(" - 0x");
        print_hex((uint32_t)(entry->base_addr + entry->length - 1));
        print(" (");
        print_capacity(entry->length);
        print(" bytes) - ");
        
        switch(entry->type) {
            case MEMORY_TYPE_AVAILABLE:
                print("Available");
                break;
            case MEMORY_TYPE_RESERVED:
                print("Reserved");
                break;
            case MEMORY_TYPE_ACPI_RECLAIMABLE:
                print("ACPI Reclaimable");
                break;
            case MEMORY_TYPE_ACPI_NVS:
                print("ACPI NVS");
                break;
            case MEMORY_TYPE_BAD:
                print("Bad Memory");
                break;
            default:
                print("Unknown");
                break;
        }
        print("\n");
    }
}

// Helper function to create or get a page table
static uint32_t* get_page_table(uint32_t virtual_addr) {
    if (!page_directory) {
        return NULL;
    }
    
    uint32_t pd_index = PAGE_DIRECTORY_INDEX(virtual_addr);
    
    // Check if page table exists
    if (!(page_directory[pd_index] & 0x1)) {
        // Create new page table
        uint32_t *page_table = (uint32_t*)memory_alloc(PAGE_SIZE);
        if (!page_table) {
            return NULL;
        }
        
        // Clear page table
        for (int i = 0; i < 1024; i++) {
            page_table[i] = 0;
        }
        
        // Add page table to directory (present, writable, user accessible)
        page_directory[pd_index] = ((uint32_t)page_table) | 0x7;
    }
    
    return (uint32_t*)(page_directory[pd_index] & 0xFFFFF000);
}

// Map physical memory to virtual address space
void* map_physical_memory(uint32_t phys_addr, uint32_t size) {
    if (!page_directory) {
        print("Error: Page directory not initialized\n");
        return NULL;
    }
    
    // Align addresses and size to page boundaries
    uint32_t phys_aligned = phys_addr & 0xFFFFF000;
    uint32_t offset = phys_addr - phys_aligned;
    uint32_t size_aligned = PAGE_ALIGN(size + offset);
    uint32_t virtual_addr = next_virtual_addr;
    
    print("Mapping physical memory: 0x");
    print_hex(phys_addr);
    print(" (size: ");
    print_uint(size);
    print(" bytes) to virtual: 0x");
    print_hex(virtual_addr + offset);
    print("\n");
    
    // Map each page
    uint32_t pages_needed = size_aligned / PAGE_SIZE;
    for (uint32_t i = 0; i < pages_needed; i++) {
        uint32_t current_virt = virtual_addr + (i * PAGE_SIZE);
        uint32_t current_phys = phys_aligned + (i * PAGE_SIZE);
        
        // Get page table for this virtual address
        uint32_t *page_table = get_page_table(current_virt);
        if (!page_table) {
            print("Error: Failed to get page table for virtual address 0x");
            print_hex(current_virt);
            print("\n");
            return NULL;
        }
        
        // Set page table entry (present, writable, not user accessible for kernel mappings)
        uint32_t pt_index = PAGE_TABLE_INDEX(current_virt);
        page_table[pt_index] = current_phys | 0x3; // Present + Writable
        
        // Mark physical page as allocated
        uint32_t page_number = current_phys / PAGE_SIZE;
        set_page_allocated(page_number);
    }
    
    // Update next available virtual address
    next_virtual_addr += size_aligned;
    
    // Return virtual address with original offset
    return (void*)(virtual_addr + offset);
}

// Unmap virtual memory
void unmap_memory(void* virt_addr, uint32_t size) {
    if (!page_directory || !virt_addr) {
        return;
    }
    
    uint32_t virtual_addr = (uint32_t)virt_addr;
    uint32_t virt_aligned = virtual_addr & 0xFFFFF000;
    uint32_t size_aligned = PAGE_ALIGN(size + (virtual_addr - virt_aligned));
    
    print("Unmapping virtual memory: 0x");
    print_hex(virtual_addr);
    print(" (size: ");
    print_uint(size);
    print(" bytes)\n");
    
    // Unmap each page
    uint32_t pages_to_unmap = size_aligned / PAGE_SIZE;
    for (uint32_t i = 0; i < pages_to_unmap; i++) {
        uint32_t current_virt = virt_aligned + (i * PAGE_SIZE);
        uint32_t pd_index = PAGE_DIRECTORY_INDEX(current_virt);
        
        // Check if page table exists
        if (page_directory[pd_index] & 0x1) {
            uint32_t *page_table = (uint32_t*)(page_directory[pd_index] & 0xFFFFF000);
            uint32_t pt_index = PAGE_TABLE_INDEX(current_virt);
            
            // Get physical address before clearing
            uint32_t phys_addr = page_table[pt_index] & 0xFFFFF000;
            
            // Clear page table entry
            page_table[pt_index] = 0;
            
            // Free physical page
            if (phys_addr) {
                free_physical_page(phys_addr);
            }
            
            // Invalidate TLB entry
            asm volatile("invlpg (%0)" : : "r"(current_virt) : "memory");
        }
    }
}

// Get physical address from virtual address
uint32_t virtual_to_physical(void* virt_addr) {
    if (!page_directory || !virt_addr) {
        return 0;
    }
    
    uint32_t virtual_addr = (uint32_t)virt_addr;
    uint32_t pd_index = PAGE_DIRECTORY_INDEX(virtual_addr);
    uint32_t pt_index = PAGE_TABLE_INDEX(virtual_addr);
    uint32_t offset = virtual_addr & 0xFFF;
    
    // Check if page table exists
    if (!(page_directory[pd_index] & 0x1)) {
        return 0; // Page not mapped
    }
    
    uint32_t *page_table = (uint32_t*)(page_directory[pd_index] & 0xFFFFF000);
    
    // Check if page is mapped
    if (!(page_table[pt_index] & 0x1)) {
        return 0; // Page not mapped
    }
    
    return (page_table[pt_index] & 0xFFFFF000) | offset;
}

// Check if virtual address is mapped
bool is_mapped(void* virt_addr) {
    return virtual_to_physical(virt_addr) != 0;
}

// Map a single page
int map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    if (!page_directory) {
        return -1;
    }
    
    uint32_t *page_table = get_page_table(virtual_addr);
    if (!page_table) {
        return -1;
    }
    
    uint32_t pt_index = PAGE_TABLE_INDEX(virtual_addr);
    page_table[pt_index] = (physical_addr & 0xFFFFF000) | (flags & 0xFFF);
    
    // Mark physical page as allocated
    uint32_t page_number = (physical_addr & 0xFFFFF000) / PAGE_SIZE;
    set_page_allocated(page_number);
    
    // Invalidate TLB entry
    asm volatile("invlpg (%0)" : : "r"(virtual_addr) : "memory");
    
    return 0;
}

// Unmap a single page
void unmap_page(uint32_t virtual_addr) {
    if (!page_directory) {
        return;
    }
    
    uint32_t pd_index = PAGE_DIRECTORY_INDEX(virtual_addr);
    
    if (page_directory[pd_index] & 0x1) {
        uint32_t *page_table = (uint32_t*)(page_directory[pd_index] & 0xFFFFF000);
        uint32_t pt_index = PAGE_TABLE_INDEX(virtual_addr);
        
        // Get physical address before clearing
        uint32_t phys_addr = page_table[pt_index] & 0xFFFFF000;
        
        page_table[pt_index] = 0;
        
        // Free physical page
        if (phys_addr) {
            free_physical_page(phys_addr);
        }
        
        // Invalidate TLB entry
        asm volatile("invlpg (%0)" : : "r"(virtual_addr) : "memory");
    }
}

// Enable paging (call this after setting up page directory)
void enable_paging(void) {
    if (!page_directory) {
        print("Error: Cannot enable paging - page directory not initialized\n");
        return;
    }
    
    // Load page directory into CR3
    asm volatile("mov %0, %%cr3" : : "r"(page_directory));
    
    // Enable paging by setting bit 31 in CR0
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
    
    print("Paging enabled\n");
}

// Additional utility functions for physical memory management

// Allocate contiguous physical pages
uint32_t allocate_contiguous_pages(uint32_t num_pages) {
    if (!page_bitmap || num_pages == 0) return 0;
    
    // Find contiguous free pages
    for (uint32_t start_page = 0; start_page <= total_pages - num_pages; start_page++) {
        bool found_contiguous = true;
        
        // Check if we have enough contiguous pages
        for (uint32_t i = 0; i < num_pages; i++) {
            if (is_page_allocated(start_page + i)) {
                found_contiguous = false;
                break;
            }
        }
        
        if (found_contiguous) {
            // Allocate all pages
            for (uint32_t i = 0; i < num_pages; i++) {
                set_page_allocated(start_page + i);
            }
            return start_page * PAGE_SIZE; // Return physical address
        }
    }
    
    return 0; // No contiguous block found
}

// Free contiguous physical pages
void free_contiguous_pages(uint32_t physical_addr, uint32_t num_pages) {
    uint32_t start_page = physical_addr / PAGE_SIZE;
    
    for (uint32_t i = 0; i < num_pages; i++) {
        set_page_free(start_page + i);
    }
}

// Get memory statistics
void get_memory_stats(uint32_t* total_kb, uint32_t* used_kb, uint32_t* free_kb) {
    if (total_kb) *total_kb = total_physical_memory / 1024;
    if (used_kb) *used_kb = used_physical_memory / 1024;
    if (free_kb) *free_kb = (available_physical_memory - used_physical_memory) / 1024;
}