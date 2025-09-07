#include "../terminal/terminal.h"
#include "../timers/timer.h"
#include "../memory/memory.h"
#include "../utility/utility.h"

// Memory allocation tracking
#define MAX_ALLOCATIONS 1000

static void* allocations[MAX_ALLOCATIONS];
static uint32_t allocation_sizes[MAX_ALLOCATIONS];
static uint32_t allocation_ids[MAX_ALLOCATIONS];
static int allocation_count = 0;
static uint32_t next_alloc_id = 1;

// Helper function to parse size with units (K, M, G)
uint32_t parse_size(const char* str) {
    if (!str) return 0;
    
    int base_size = parse_int(str);
    if (base_size <= 0) return 0;
    
    // Find the unit
    int i = 0;
    while (str[i] && (str[i] < 'A' || str[i] > 'Z') && (str[i] < 'a' || str[i] > 'z')) {
        i++;
    }
    
    if (str[i] == 'K' || str[i] == 'k') {
        return base_size * 1024;
    } else if (str[i] == 'M' || str[i] == 'm') {
        return base_size * 1024 * 1024;
    } else if (str[i] == 'G' || str[i] == 'g') {
        return base_size * 1024 * 1024 * 1024;
    } else {
        return base_size; // Assume bytes
    }
}

// Helper function to add allocation to tracking
void add_allocation(void* ptr, uint32_t size) {
    if (allocation_count < MAX_ALLOCATIONS && ptr) {
        allocations[allocation_count] = ptr;
        allocation_sizes[allocation_count] = size;
        allocation_ids[allocation_count] = next_alloc_id++;
        allocation_count++;
    }
}

// Helper function to remove allocation from tracking
void remove_allocation(void* ptr) {
    for (int i = 0; i < allocation_count; i++) {
        if (allocations[i] == ptr) {
            // Shift remaining allocations
            for (int j = i; j < allocation_count - 1; j++) {
                allocations[j] = allocations[j + 1];
                allocation_sizes[j] = allocation_sizes[j + 1];
                allocation_ids[j] = allocation_ids[j + 1];
            }
            allocation_count--;
            break;
        }
    }
}

// Print memory size in human readable format
void print_memory_size(uint32_t bytes) {
    if (bytes >= 1024 * 1024 * 1024) {
        print_integer(bytes / (1024 * 1024 * 1024));
        print(".");
        print_integer((bytes % (1024 * 1024 * 1024)) / (1024 * 1024 * 100));
        print("GB");
    } else if (bytes >= 1024 * 1024) {
        print_integer(bytes / (1024 * 1024));
        print(".");
        print_integer((bytes % (1024 * 1024)) / (1024 * 100));
        print("MB");
    } else if (bytes >= 1024) {
        print_integer(bytes / 1024);
        print(".");
        print_integer((bytes % 1024) / 100);
        print("KB");
    } else {
        print_integer(bytes);
        print("B");
    }
}

// Free all allocations
void mempop_free() {
    if (allocation_count == 0) {
        print("No allocations to free.\n");
        return;
    }
    
    print("Freeing ");
    print_integer(allocation_count);
    print(" allocations...\n");
    
    uint32_t total_freed = 0;
    for (int i = 0; i < allocation_count; i++) {
        if (allocations[i]) {
            total_freed += allocation_sizes[i];
            free(allocations[i]);
            allocations[i] = NULL;
        }
    }
    
    print("Freed ");
    print_memory_size(total_freed);
    print(" of memory.\n");
    
    allocation_count = 0;
    next_alloc_id = 1;
}

// List all allocations
void mempop_list() {
    if (allocation_count == 0) {
        print("No active allocations.\n");
        return;
    }
    
    print("Active Memory Allocations:\n");
    print("==========================\n");
    
    uint32_t total_size = 0;
    for (int i = 0; i < allocation_count; i++) {
        print("[");
        print_integer(allocation_ids[i]);
        print("] ");
        print_memory_size(allocation_sizes[i]);
        print(" at 0x");
        print_hex((uint32_t)allocations[i]);
        print("\n");
        total_size += allocation_sizes[i];
    }
    
    print("==========================\n");
    print("Total: ");
    print_integer(allocation_count);
    print(" allocations, ");
    print_memory_size(total_size);
    print("\n");
}

// Allocate memory
void mempop_allocate(const char* size_str, int count) {
    uint32_t size = parse_size(size_str);
    
    if (size == 0) {
        print("Invalid size format. Use: <number>[K|M|G]\n");
        print("Examples: 1024, 4K, 1M, 2G\n");
        return;
    }
    
    if (count <= 0 || count > 100) {
        print("Invalid count. Must be between 1 and 100.\n");
        return;
    }
    
    print("Allocating ");
    print_memory_size(size);
    if (count > 1) {
        print(" x ");
        print_integer(count);
        print(" times");
    }
    print("...\n");
    
    int successful = 0;
    uint32_t total_allocated = 0;
    
    for (int i = 0; i < count; i++) {
        void* ptr = malloc(size);
        if (ptr) {
            add_allocation(ptr, size);
            successful++;
            total_allocated += size;
            
            print("  [");
            print_integer(allocation_ids[allocation_count - 1]);
            print("] ");
            print_memory_size(size);
            print(" allocated at 0x");
            print_hex((uint32_t)ptr);
            print("\n");
        } else {
            print("\n\n\ninvalid memory size !\n\n\n");
            break;
        }
    }
    
    print("Summary: ");
    print_integer(successful);
    print("/");
    print_integer(count);
    print(" allocations successful, ");
    print_memory_size(total_allocated);
    print(" total allocated.\n");
}

// Parse multiplication format (e.g., "10K * 5")
int parse_multiplication(char* argv[], int argc, int start_index, char** size_str) {
    *size_str = argv[start_index];
    
    // Check if next argument is "*"
    if (start_index + 1 < argc && strcmp(argv[start_index + 1], "*") == 0) {
        if (start_index + 2 < argc) {
            return parse_int(argv[start_index + 2]);
        }
    }
    
    return 1; // Default count
}

void mempop_command(int argc, char* argv[]) {
    //used_memory = 100000;
    if (argc < 2) {
        terminal_clear_inFunction();
        print("\n=== MEMPOP - Memory Engineering Management Platform ===\n");
        print("Memory allocation and management toolkit\n\n");
        print("Commands:\n");
        print("  mempop free           - Free all allocated memory\n");
        print("  mempop list           - List all active allocations\n");
        print("  mempop <size>         - Allocate memory\n");
        print("  mempop <size> * <num> - Allocate memory multiple times\n\n");
        print("Size formats:\n");
        print("  1024    - 1024 bytes\n");
        print("  1K      - 1 kilobyte (1024 bytes)\n");
        print("  1M      - 1 megabyte (1024 KB)\n");
        print("  1G      - 1 gigabyte (1024 MB)\n\n");
        print("Examples:\n");
        print("  mempop 1K         - Allocate 1 kilobyte\n");
        print("  mempop 1M         - Allocate 1 megabyte\n");
        print("  mempop 10K * 10   - Allocate 10KB, 10 times\n");
        print("  mempop 512 * 5    - Allocate 512 bytes, 5 times\n\n");
        
        // Show current memory status
        print("Current Status:\n");
        print("  Active allocations: ");
        print_integer(allocation_count);
        print("/");
        print_integer(MAX_ALLOCATIONS);
        print("\n");
        
        if (allocation_count > 0) {
            uint32_t total = 0;
            for (int i = 0; i < allocation_count; i++) {
                total += allocation_sizes[i];
            }
            print("  Total allocated: ");
            print_memory_size(total);
            print("\n");
        }
        
        return;
    }
    
    char* command = argv[1];
    
    if (strcmp(command, "free") == 0) {
        mempop_free();
    }
    else if (strcmp(command, "list") == 0) {
        mempop_list();
    }
    else {
        // Try to parse as size allocation
        char* size_str;
        int count = parse_multiplication(argv, argc, 1, &size_str);
        
        if (count > 0) {
            mempop_allocate(size_str, count);
        } else {
            print("Invalid command or format.\n");
            print("Use 'mempop' without arguments for help.\n");
        }
    }
}
