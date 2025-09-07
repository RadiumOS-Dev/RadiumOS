// acpi.h
#ifndef ACPI_H
#define ACPI_H

#include <stdint.h>

#pragma pack(push, 1) // Ensure structures are packed

// RSDP structure
typedef struct {
    char signature[8];          // "RSD PTR " (8 bytes)
    uint8_t checksum;           // Checksum of the structure
    char oem_id[6];            // OEM ID (6 bytes)
    uint8_t revision;           // Revision of the structure
    uint32_t rsdt_address;      // Address of the RSDT
    uint32_t length;            // Length of the structure
    uint64_t xsdt_address;      // Address of the XSDT (if applicable)
    uint8_t extended_checksum;  // Extended checksum
    uint8_t reserved[3];        // Reserved bytes
} acpi_rsdp_t;

// RSDT structure
typedef struct {
    char signature[4];          // "RSDT"
    uint32_t length;            // Length of the table
    uint32_t revision;          // Revision of the table
    uint32_t checksum;          // Checksum of the table
    char oem_id[6];             // OEM ID
    char oem_table_id[8];       // OEM Table ID
    uint32_t oem_revision;       // OEM Revision
    uint32_t creator_id;        // Creator ID
    uint32_t creator_revision;  // Creator Revision
    // Table entries follow
} acpi_rsdt_t;

#pragma pack(pop) // Restore previous packing

// Function declarations
void acpi_init(void);
void acpi_parse_rsdp(acpi_rsdp_t *rsdp);
void acpi_parse_rsdt(acpi_rsdt_t *rsdt);
acpi_rsdp_t* find_rsdp(void);
void print_pointer(void* ptr);

#endif // ACPI_H
