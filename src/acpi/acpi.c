#include "acpi.h"
#include "../errors/error.h"
#include "../terminal/terminal.h"
#include <stddef.h> // For size_t and NULL
#include <stdint.h> // For uint32_t, uint8_t, etc.

// Function to print a pointer in hexadecimal format
void print_pointer(void* ptr) {
    print("0x");
    uint32_t address = (uint32_t)(uintptr_t)ptr; // Cast to uintptr_t for safe conversion
    for (int i = 7; i >= 0; i--) {
        uint8_t nibble = (address >> (i * 4)) & 0xF; // Get each nibble
        if (nibble < 10) {
            print((char[]){'0' + nibble, 0}); // Print 0-9
        } else {
            print((char[]){'A' + (nibble - 10), 0}); // Print A-F
        }
    }
}

// Function to scan memory for the RSDP
acpi_rsdp_t* find_rsdp(void) {
    // Scan the memory from 0x000E0000 to 0x000FFFFF (1MB area)
    for (uintptr_t addr = 0x000E0000; addr < 0x000FFFFF; addr += 16) {
        acpi_rsdp_t* rsdp = (acpi_rsdp_t*)addr;

        // Check for the RSDP signature
        if (rsdp->signature[0] == 'R' && rsdp->signature[1] == 'S' &&
            rsdp->signature[2] == 'D' && rsdp->signature[3] == 'P') {
            // Validate checksum
            uint8_t checksum = 0;
            for (size_t i = 0; i < sizeof(acpi_rsdp_t); i++) {
                checksum += ((uint8_t*)rsdp)[i];
            }
            if (checksum == 0) {
                return rsdp; // RSDP found and valid
            }
        }
    }
    return NULL; // RSDP not found
}

// Initialize ACPI
void acpi_init(void) {
    // Locate the RSDP in memory
    acpi_rsdp_t *rsdp = find_rsdp();
    if (rsdp) {
        // Valid RSDP found
        acpi_parse_rsdp(rsdp);
    } else {
        print("\n");
        handle_error("! ACPI: RSDP not found.!\n", "kernel");
    }
}

// Parse the RSDP structure
void acpi_parse_rsdp(acpi_rsdp_t *rsdp) {
    print("ACPI: RSDP found at ");
    print_pointer(rsdp);
    print("\n");
    
    print("OEM ID: ");
    print(rsdp->oem_id);
    print("\n");
    
    print("RSDT Address: ");
    print_pointer((void *)rsdp->rsdt_address);
    print("\n");
    
    // Parse the RSDT
    acpi_rsdt_t *rsdt = (acpi_rsdt_t *)rsdp->rsdt_address;
    acpi_parse_rsdt(rsdt);
}

// Parse the RSDT structure
void acpi_parse_rsdt(acpi_rsdt_t *rsdt) {
    print("ACPI: RSDT found at ");
    print_pointer(rsdt);
    print("\n");
    
    print("OEM Table ID: ");
    print(rsdt->oem_table_id);
    print("\n");
    
    // Further parsing of entries can be done here
}
