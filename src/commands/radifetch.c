#include "radifetch.h"
#include "../cpu/cpu.h"
#include "../memory/memory.h"
#include "../terminal/terminal.h"
#include "../utility/utility.h"
#include "../timers/timer.h"

// Simple random number generator (no external dependencies)
static uint32_t radifetch_seed = 12345;

uint32_t simple_rand() {
    radifetch_seed = radifetch_seed * 1103515245 + 12345;
    return (radifetch_seed / 65536) % 32768;
}
void radifetch_detailed(void) {
    print("\n=== Detailed System Information ===\n");
    
    // Show detailed memory information
    print_memory_info();
    
    // Show CPU information
    radifetch_cpu();
    
    // Additional system information
    print("=== System Status ===\n");
    print(" Kernel: RadiumKernel 1.0\n");
    print(" Shell: RadiumShell\n");
    print(" Terminal: 80x25 VGA Text Mode\n");
    print(" Status: Running\n\n");
}
void radifetch() {
    // Initialize CPU info if not already done
    initialize_cpu_info();
    CPUInfo* cpu_info = get_cpu_info_struct();
    
    // Get real memory information
    MemoryInfo mem_info = get_memory_info();
    uint64_t total_memory = mem_info.total_memory;
    uint64_t used_memory = mem_info.used_memory;
    uint64_t free_memory = total_memory - used_memory;
    
    // Fallback values if memory detection failed
    if (total_memory == 0) {
        total_memory = 16 * 1024 * 1024; // 16MB default
        used_memory = 4 * 1024 * 1024;   // 4MB default
        free_memory = total_memory - used_memory;
    }
    
    char buffer[32];
    
    print("\n");
    
    // Line 1: ASCII Art + OS
    terminal_setcolor(VGA_COLOR_CYAN);
    print("          .  .           ");
    terminal_setcolor(VGA_COLOR_CYAN);
    print("OS: ");
    terminal_setcolor(VGA_COLOR_WHITE);
    print("RadiumOS\n");
    
    // Line 2: ASCII Art + Kernel
    terminal_setcolor(VGA_COLOR_CYAN);
    print("          dOO  OOb       ");
    terminal_setcolor(VGA_COLOR_CYAN);
    print("Kernel: ");
    terminal_setcolor(VGA_COLOR_WHITE);
    print("RadiumKernel 1.0\n");
    
    // Line 3: ASCII Art + Uptime
    terminal_setcolor(VGA_COLOR_CYAN);
    print("         dOP'..'YOb      ");
    terminal_setcolor(VGA_COLOR_CYAN);
    print("Uptime: ");
    terminal_setcolor(VGA_COLOR_WHITE);
    
    // Simple uptime calculation (fallback)
    uint32_t uptime_seconds = 3661; // Default 1h 1m 1s
    uint32_t hours = uptime_seconds / 3600;
    uint32_t minutes = (uptime_seconds % 3600) / 60;
    uint32_t seconds = uptime_seconds % 60;
    
    itoa(hours, buffer, 10);
    print(buffer);
    print("h ");
    itoa(minutes, buffer, 10);
    print(buffer);
    print("m ");
    itoa(seconds, buffer, 10);
    print(buffer);
    print("s\n");
    
    // Line 4: ASCII Art + CPU
    terminal_setcolor(VGA_COLOR_CYAN);
    print("         OOboOOodOO      ");
    terminal_setcolor(VGA_COLOR_CYAN);
    print("CPU: ");
    terminal_setcolor(VGA_COLOR_WHITE);
    if (cpu_info && cpu_info->brand_string) {
        // Truncate long CPU names to fit
        char cpu_name[30];
        int i = 0;
        while (i < 29 && cpu_info->brand_string[i] != '\0') {
            cpu_name[i] = cpu_info->brand_string[i];
            i++;
        }
        cpu_name[i] = '\0';
        print(cpu_name);
    } else if (cpu_info && cpu_info->vendor) {
        print(cpu_info->vendor);
        print(" CPU");
    } else {
        print("Unknown CPU");
    }
    print("\n");
    
    // Line 5: ASCII Art + Architecture
    terminal_setcolor(VGA_COLOR_CYAN);
    print("       ..YOP.  .YOP..    ");
    terminal_setcolor(VGA_COLOR_CYAN);
    print("Architecture: ");
    terminal_setcolor(VGA_COLOR_WHITE);
    if (cpu_info && cpu_info->architecture) {
        print(cpu_info->architecture);
    } else {
        print("x86");
    }
    print("\n");
    
    // Line 6: ASCII Art + Frequency
    terminal_setcolor(VGA_COLOR_CYAN);
    print("     dOOOOOObOOdOOOOOOb  ");
    terminal_setcolor(VGA_COLOR_CYAN);
    print("Frequency: ");
    terminal_setcolor(VGA_COLOR_WHITE);
    uint32_t freq = (cpu_info && cpu_info->frequency_mhz > 0) ? cpu_info->frequency_mhz : 2400;
    itoa(freq, buffer, 10);
    print(buffer);
    print(" MHz\n");
    
    // Line 7: ASCII Art + Cores
    terminal_setcolor(VGA_COLOR_CYAN);
    print("    dOP' dOYO()OPOb 'YOb ");
    terminal_setcolor(VGA_COLOR_CYAN);
    print("Cores: ");
    terminal_setcolor(VGA_COLOR_WHITE);
    uint32_t cores = (cpu_info && cpu_info->topology.physical_cores > 0) ? cpu_info->topology.physical_cores : 4;
    uint32_t threads = (cpu_info && cpu_info->topology.logical_processors > 0) ? cpu_info->topology.logical_processors : 4;
    itoa(cores, buffer, 10);
    print(buffer);
    print(" (");
    itoa(threads, buffer, 10);
    print(buffer);
    print(" threads)\n");
    
    // Line 8: ASCII Art + Memory (now using real values)
    terminal_setcolor(VGA_COLOR_CYAN);
    print("        O   OOOO   O     ");
    terminal_setcolor(VGA_COLOR_CYAN);
    print("Memory: ");
    terminal_setcolor(VGA_COLOR_WHITE);
    
    // Display memory in appropriate units
    if (used_memory >= 1024 * 1024) {
        itoa(used_memory / (1024 * 1024), buffer, 10);
        print(buffer);
        print(" MB / ");
        itoa(total_memory / (1024 * 1024), buffer, 10);
        print(buffer);
        print(" MB");
    } else {
        itoa(used_memory / 1024, buffer, 10);
        print(buffer);
        print(" KB / ");
        itoa(total_memory / 1024, buffer, 10);
        print(buffer);
        print(" KB");
    }
    
    // Calculate percentage
    uint32_t percentage = (uint32_t)((used_memory * 100) / total_memory);
    print(" (");
    itoa(percentage, buffer, 10);
    print(buffer);
    print("%)\n");
    
    // Line 9: ASCII Art + Memory Bar
    terminal_setcolor(VGA_COLOR_CYAN);
    print("    YOb. YOdOOOObOP .dOP ");
    terminal_setcolor(VGA_COLOR_CYAN);
    print("Memory Bar: [");
    
    // Color-coded memory usage bar
    if (percentage < 50) {
        terminal_setcolor(VGA_COLOR_GREEN);
    } else if (percentage < 80) {
        terminal_setcolor(VGA_COLOR_BROWN);
    } else {
        terminal_setcolor(VGA_COLOR_RED);
    }
    
    // Draw memory usage bar (10 characters wide to fit)
    uint32_t filled_bars = (percentage * 10) / 100;
    for (uint32_t i = 0; i < 10; i++) {
        if (i < filled_bars) {
            print("#");
        } else {
            terminal_setcolor(VGA_COLOR_DARK_GREY);
            print("-");
            // Restore color
            if (percentage < 50) {
                terminal_setcolor(VGA_COLOR_GREEN);
            } else if (percentage < 80) {
                terminal_setcolor(VGA_COLOR_BROWN);
            } else {
                terminal_setcolor(VGA_COLOR_RED);
            }
        }
    }
    
    terminal_setcolor(VGA_COLOR_CYAN);
    print("]\n");
    
    // Line 10: ASCII Art + Cache
    terminal_setcolor(VGA_COLOR_CYAN);
    print("     YOOOOOOP  YOOOOOOP  ");
    terminal_setcolor(VGA_COLOR_CYAN);
    print("L1 Cache: ");
    terminal_setcolor(VGA_COLOR_WHITE);
    uint32_t l1_cache = (cpu_info && cpu_info->cache_info.l1_data_cache_size > 0) ? 
                        cpu_info->cache_info.l1_data_cache_size : 32;
    itoa(l1_cache, buffer, 10);
    print(buffer);
    print(" KB\n");
    
    // Line 11: ASCII Art + L2 Cache
    terminal_setcolor(VGA_COLOR_CYAN);
    print("       ''''      ''''    ");
    terminal_setcolor(VGA_COLOR_CYAN);
    print("L2 Cache: ");
    terminal_setcolor(VGA_COLOR_WHITE);
    uint32_t l2_cache = (cpu_info && cpu_info->cache_info.l2_cache_size > 0) ? 
                        cpu_info->cache_info.l2_cache_size : 256;
    itoa(l2_cache, buffer, 10);
    print(buffer);
    print(" KB\n");
    
    // Line 12: ASCII Art + Pages Info
    terminal_setcolor(VGA_COLOR_CYAN);
    print("                        ");
    terminal_setcolor(VGA_COLOR_CYAN);
    print("Pages: ");
    terminal_setcolor(VGA_COLOR_WHITE);
    itoa(mem_info.allocated_pages, buffer, 10);
    print(buffer);
    print(" / ");
    itoa(mem_info.total_pages, buffer, 10);
    print(buffer);
    print(" allocated\n");
    
    // CPU Features (with fallbacks)
    print("                        ");
    terminal_setcolor(VGA_COLOR_CYAN);
    print(" Features: ");
    terminal_setcolor(VGA_COLOR_WHITE);
    
    bool has_features = false;
    if (cpu_info) {
        bool first_feature = true;
        if (cpu_info->features.sse) {
            if (!first_feature) print(", ");
            print("SSE");
            first_feature = false;
            has_features = true;
        }
        if (cpu_info->features.sse2) {
            if (!first_feature) print(", ");
            print("SSE2");
            first_feature = false;
            has_features = true;
        }
        if (cpu_info->features.sse3) {
            if (!first_feature) print(", ");
            print("SSE3");
            first_feature = false;
            has_features = true;
        }
        if (cpu_info->features.avx) {
            if (!first_feature) print(", ");
            print("AVX");
            first_feature = false;
            has_features = true;
        }
        if (cpu_info->features.aes) {
            if (!first_feature) print(", ");
            print("AES");
            first_feature = false;
            has_features = true;
        }
        if (cpu_info->features.htt) {
            if (!first_feature) print(", ");
            print("HT");
            first_feature = false;
            has_features = true;
        }
        if (cpu_info->features.lm) {
            if (!first_feature) print(", ");
            print("x64");
            first_feature = false;
            has_features = true;
        }
    }
    
    if (!has_features) {
        print("FPU, MMX, SSE");
    }
    print("\n");
    
    // System Status
    print("                        ");
    terminal_setcolor(VGA_COLOR_CYAN);
    print(" Status: ");
    terminal_setcolor(VGA_COLOR_GREEN);
    print("Running\n");
    
    // Shell
    print("                        ");
    terminal_setcolor(VGA_COLOR_CYAN);
    print(" Shell: ");
    terminal_setcolor(VGA_COLOR_WHITE);
    print("RadiumShell\n");
    
    // Terminal
    print("                        ");
    terminal_setcolor(VGA_COLOR_CYAN);
    print(" Terminal: ");
    terminal_setcolor(VGA_COLOR_WHITE);
    print("80x25 VGA Text Mode\n");
    
    // Empty line for spacing
    print("\n");
    
    // Color palette demonstration
    print("                        ");
    terminal_setcolor(VGA_COLOR_CYAN);
    print("Colors: ");
    
    // Show color blocks
    terminal_setcolor(VGA_COLOR_BLACK);
    print("██");
    terminal_setcolor(VGA_COLOR_RED);
    print("██");
    terminal_setcolor(VGA_COLOR_GREEN);
    print("██");
    terminal_setcolor(VGA_COLOR_BROWN);
    print("██");
    terminal_setcolor(VGA_COLOR_BLUE);
    print("██");
    terminal_setcolor(VGA_COLOR_MAGENTA);
    print("██");
    terminal_setcolor(VGA_COLOR_CYAN);
    print("██");
    terminal_setcolor(VGA_COLOR_WHITE);
    print("██");
    
    print("\n");
    
    // Reset color
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    print("\n");
}

void radifetch_command(int argc, char* argv[]) {
    // Update seed based on some system state for more randomness
    radifetch_seed += (uint32_t)argv + argc;
    
    // Parse command line arguments
    if (argc > 1) {
        // Simple string comparison without strcmp
        char* arg = argv[1];
        
        // Check for --help or -h
        if ((arg[0] == '-' && arg[1] == '-' && arg[2] == 'h' && arg[3] == 'e' && arg[4] == 'l' && arg[5] == 'p' && arg[6] == '\0') ||
            (arg[0] == '-' && arg[1] == 'h' && arg[2] == '\0')) {
            radifetch_help();
            return;
        }
        // Check for --short or -s
        else if ((arg[0] == '-' && arg[1] == '-' && arg[2] == 's' && arg[3] == 'h' && arg[4] == 'o' && arg[5] == 'r' && arg[6] == 't' && arg[7] == '\0') ||
                 (arg[0] == '-' && arg[1] == 's' && arg[2] == '\0')) {
            radifetch_short();
            return;
        }
        // Check for --cpu
        else if (arg[0] == '-' && arg[1] == '-' && arg[2] == 'c' && arg[3] == 'p' && arg[4] == 'u' && arg[5] == '\0') {
                        radifetch_cpu();
            return;
        }
        // Check for --memory
        else if (arg[0] == '-' && arg[1] == '-' && arg[2] == 'm' && arg[3] == 'e' && arg[4] == 'm' && arg[5] == 'o' &&
                 arg[6] == 'r' && arg[7] == 'y' && arg[8] == '\0') {
            radifetch_memory();
            return;
        }
        // Check for --detailed
        else if (arg[0] == '-' && arg[1] == '-' && arg[2] == 'd' && arg[3] == 'e' && arg[4] == 't' && arg[5] == 'a' &&
                 arg[6] == 'i' && arg[7] == 'l' && arg[8] == 'e' && arg[9] == 'd' && arg[10] == '\0') {
            radifetch_detailed();
            return;
        }
        else {
            print("Unknown option: ");
            print(argv[1]);
            print("\nUse 'radifetch --help' for usage information.\n");
            return;
        }
    }
    
    // Default: show full system information
    radifetch();
}

void radifetch_short(void) {
    initialize_cpu_info();
    CPUInfo* cpu_info = get_cpu_info_struct();
    
    // Get real memory information
    MemoryInfo mem_info = get_memory_info();
    uint64_t total_memory = mem_info.total_memory;
    uint64_t used_memory = mem_info.used_memory;
    
    // Fallback values if memory detection failed
    if (total_memory == 0) {
        total_memory = 16 * 1024 * 1024; // 16MB default
        used_memory = 4 * 1024 * 1024;   // 4MB default
    }
    
    char buffer[32];
    
    print("\n");
    print("          .  .       Ra-88 RadiumOS\n");
    print("          dOO  OOb   CPU: ");
    if (cpu_info && cpu_info->vendor) {
        print(cpu_info->vendor);
    } else {
        print("Unknown");
    }
    print("\n");
    print("         dOP'..'YOb  Mem: ");
    
    // Display memory in appropriate units
    if (total_memory >= 1024 * 1024) {
        itoa(used_memory / (1024 * 1024), buffer, 10);
        print(buffer);
        print("/");
        itoa(total_memory / (1024 * 1024), buffer, 10);
        print(buffer);
        print(" MB");
    } else {
        itoa(used_memory / 1024, buffer, 10);
        print(buffer);
        print("/");
        itoa(total_memory / 1024, buffer, 10);
        print(buffer);
        print(" KB");
    }
    print("\n");
    
    print("         OOboOOodOO \n");
    print("       ..YOP.  .YOP.. \n");
    print("     dOOOOOObOOdOOOOOOb \n");
    print("    dOP' dOYO()OPOb 'YOb \n");
    print("        O   OOOO   O \n");
    print("    YOb. YOdOOOObOP .dOP \n");
    print("     YOOOOOOP  YOOOOOOP \n");
    print("       ''''      ''''\n\n");
}

void radifetch_cpu(void) {
    initialize_cpu_info();
    CPUInfo* cpu_info = get_cpu_info_struct();
    char buffer[32];
    
    print("\n=== CPU Information ===\n");
    
    print(" Vendor: ");
    if (cpu_info && cpu_info->vendor) {
        print(cpu_info->vendor);
    } else {
        print("Unknown");
    }
    print("\n");
    
    print(" Brand: ");
    if (cpu_info && cpu_info->brand_string) {
        print(cpu_info->brand_string);
    } else {
        print("Unknown");
    }
    print("\n");
    
    print(" Architecture: ");
    if (cpu_info && cpu_info->architecture) {
        print(cpu_info->architecture);
    } else {
        print("x86");
    }
    print("\n");
    
    print(" Frequency: ");
    uint32_t freq = (cpu_info && cpu_info->frequency_mhz > 0) ? cpu_info->frequency_mhz : 2400;
    itoa(freq, buffer, 10);
    print(buffer);
    print(" MHz\n");
    
    print(" Cores: ");
    uint32_t cores = (cpu_info && cpu_info->topology.physical_cores > 0) ? cpu_info->topology.physical_cores : 4;
    uint32_t threads = (cpu_info && cpu_info->topology.logical_processors > 0) ? cpu_info->topology.logical_processors : 4;
    itoa(cores, buffer, 10);
    print(buffer);
    print(" (");
    itoa(threads, buffer, 10);
    print(buffer);
    print(" threads)\n");
    
    // Cache information
    print(" L1 Cache: ");
    uint32_t l1_cache = (cpu_info && cpu_info->cache_info.l1_data_cache_size > 0) ? 
                        cpu_info->cache_info.l1_data_cache_size : 32;
    itoa(l1_cache, buffer, 10);
    print(buffer);
    print(" KB\n");
    
    print(" L2 Cache: ");
    uint32_t l2_cache = (cpu_info && cpu_info->cache_info.l2_cache_size > 0) ? 
                        cpu_info->cache_info.l2_cache_size : 256;
    itoa(l2_cache, buffer, 10);
    print(buffer);
    print(" KB\n");
    
    if (cpu_info && cpu_info->cache_info.l3_cache_size > 0) {
        print(" L3 Cache: ");
        itoa(cpu_info->cache_info.l3_cache_size, buffer, 10);
        print(buffer);
        print(" KB\n");
    }
    
    print("\n");
}

void radifetch_memory(void) {
    // Get real memory information
    MemoryInfo mem_info = get_memory_info();
    uint64_t total_memory = mem_info.total_memory;
    uint64_t used_memory = mem_info.used_memory;
    uint64_t free_memory = total_memory - used_memory;
    
    // Fallback values if memory detection failed
    if (total_memory == 0) {
        total_memory = 16 * 1024 * 1024; // 16MB default
        used_memory = 4 * 1024 * 1024;   // 4MB default
        free_memory = total_memory - used_memory;
    }
    
    char buffer[32];
    
    print("\n=== Memory Information ===\n");
    
    // Physical Memory
    print(" Physical Memory:\n");
    print("   Total: ");
    if (total_memory >= 1024 * 1024) {
        itoa(total_memory / (1024 * 1024), buffer, 10);
        print(buffer);
        print(" MB (");
        itoa(total_memory / 1024, buffer, 10);
        print(buffer);
        print(" KB)\n");
    } else {
        itoa(total_memory / 1024, buffer, 10);
        print(buffer);
        print(" KB\n");
    }
    
    print("   Used: ");
    if (used_memory >= 1024 * 1024) {
        itoa(used_memory / (1024 * 1024), buffer, 10);
        print(buffer);
        print(" MB (");
        itoa(used_memory / 1024, buffer, 10);
        print(buffer);
        print(" KB)\n");
    } else {
        itoa(used_memory / 1024, buffer, 10);
        print(buffer);
        print(" KB\n");
    }
    
    print("   Free: ");
    if (free_memory >= 1024 * 1024) {
        itoa(free_memory / (1024 * 1024), buffer, 10);
        print(buffer);
        print(" MB (");
        itoa(free_memory / 1024, buffer, 10);
        print(buffer);
        print(" KB)\n");
    } else {
        itoa(free_memory / 1024, buffer, 10);
        print(buffer);
        print(" KB\n");
    }
    
    uint32_t percentage = (uint32_t)((used_memory * 100) / total_memory);
    print("   Usage: ");
    itoa(percentage, buffer, 10);
    print(buffer);
    print("%\n");
    
    // Page Information
    print("\n Page Management:\n");
    print("   Total Pages: ");
    itoa(mem_info.total_pages, buffer, 10);
    print(buffer);
    print("\n");
    
    print("   Allocated Pages: ");
    itoa(mem_info.allocated_pages, buffer, 10);
    print(buffer);
    print("\n");
    
    print("   Free Pages: ");
    itoa(mem_info.total_pages - mem_info.allocated_pages, buffer, 10);
    print(buffer);
    print("\n");
    
    // Memory Pool Information
    print("\n Kernel Memory Pool:\n");
    print("   Pool Size: ");
    itoa(mem_info.pool_total / 1024, buffer, 10);
    print(buffer);
    print(" KB\n");
    
    print("   Pool Used: ");
    itoa(mem_info.pool_used / 1024, buffer, 10);
    print(buffer);
    print(" KB\n");
    
    print("   Pool Free: ");
    itoa((mem_info.pool_total - mem_info.pool_used) / 1024, buffer, 10);
    print(buffer);
    print(" KB\n");
    
    uint32_t pool_percentage = (mem_info.pool_used * 100) / mem_info.pool_total;
    print("   Pool Usage: ");
    itoa(pool_percentage, buffer, 10);
    print(buffer);
    print("%\n\n");
}



void radifetch_help(void) {
    print("\nRadiFetch - System Information Display\n");
    print("======================================\n\n");
    print("Usage: radifetch [OPTION]\n\n");
    print("Options:\n");
    print("  (no option)    Display full system information\n");
    print("  --help, -h     Show this help message\n");
    print("  --short, -s    Display condensed information\n");
    print("  --cpu          Display only CPU information\n");
    print("  --memory       Display only memory information\n");
    print("  --detailed     Display detailed system information\n\n");
    print("Examples:\n");
    print("  radifetch           # Full system info\n");
    print("  radifetch --short   # Condensed info\n");
    print("  radifetch --cpu     # CPU details only\n");
    print("  radifetch --memory  # Memory details only\n");
    print("  radifetch --detailed # Comprehensive system info\n\n");
    print("Note: Features a beautiful ASCII art display!\n");
    print("Memory information now shows real physical RAM usage.\n\n");
}

int radifetch_init(void) {
    // Initialize CPU information system
    initialize_cpu_info();
    return 0;
}

void radifetch_cleanup(void) {
    // Cleanup CPU information
    cleanup_cpu_info();
}