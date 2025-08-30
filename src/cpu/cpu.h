#ifndef CPU_H
#define CPU_H

#include <stdint.h>

// CPUID information structure
typedef struct {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
} CPUIDInfo;

// CPU features structure
typedef struct {
    // Basic features (EDX)
    uint8_t fpu : 1;        // Floating Point Unit
    uint8_t vme : 1;        // Virtual Mode Extension
    uint8_t de : 1;         // Debugging Extension
    uint8_t pse : 1;        // Page Size Extension
    uint8_t tsc : 1;        // Time Stamp Counter
    uint8_t msr : 1;        // Model Specific Registers
    uint8_t pae : 1;        // Physical Address Extension
    uint8_t mce : 1;        // Machine Check Exception
    uint8_t cx8 : 1;        // CMPXCHG8B Instruction
    uint8_t apic : 1;       // Advanced PIC
    uint8_t sep : 1;        // SYSENTER/SYSEXIT
    uint8_t mtrr : 1;       // Memory Type Range Registers
    uint8_t pge : 1;        // Page Global Enable
    uint8_t mca : 1;        // Machine Check Architecture
    uint8_t cmov : 1;       // Conditional Move Instructions
    uint8_t pat : 1;        // Page Attribute Table
    uint8_t pse36 : 1;      // 36-bit Page Size Extension
    uint8_t psn : 1;        // Processor Serial Number
        uint8_t clflush : 1;    // CLFLUSH Instruction
    uint8_t dts : 1;        // Debug Trace Store
    uint8_t acpi : 1;       // ACPI Support
    uint8_t mmx : 1;        // MMX Technology
    uint8_t fxsr : 1;       // FXSAVE/FXRSTOR Instructions
    uint8_t sse : 1;        // SSE Instructions
    uint8_t sse2 : 1;       // SSE2 Instructions
    uint8_t ss : 1;         // Self Snoop
    uint8_t htt : 1;        // Hyper-Threading Technology
    uint8_t tm : 1;         // Thermal Monitor
    uint8_t pbe : 1;        // Pending Break Enable

    // Extended features (ECX)
    uint8_t sse3 : 1;       // SSE3 Instructions
    uint8_t pclmulqdq : 1;  // PCLMULQDQ Instruction
    uint8_t dtes64 : 1;     // 64-bit Debug Store
    uint8_t monitor : 1;    // MONITOR/MWAIT Instructions
    uint8_t ds_cpl : 1;     // CPL Qualified Debug Store
    uint8_t vmx : 1;        // Virtual Machine Extensions
    uint8_t smx : 1;        // Safer Mode Extensions
    uint8_t est : 1;        // Enhanced SpeedStep Technology
    uint8_t tm2 : 1;        // Thermal Monitor 2
    uint8_t ssse3 : 1;      // Supplemental SSE3
    uint8_t cnxt_id : 1;    // Context ID
    uint8_t fma : 1;        // Fused Multiply Add
    uint8_t cx16 : 1;       // CMPXCHG16B Instruction
    uint8_t xtpr : 1;       // xTPR Update Control
    uint8_t pdcm : 1;       // Performance/Debug Capability MSR
    uint8_t pcid : 1;       // Process Context Identifiers
    uint8_t dca : 1;        // Direct Cache Access
    uint8_t sse4_1 : 1;     // SSE4.1 Instructions
    uint8_t sse4_2 : 1;     // SSE4.2 Instructions
    uint8_t x2apic : 1;     // x2APIC Support
    uint8_t movbe : 1;      // MOVBE Instruction
    uint8_t popcnt : 1;     // POPCNT Instruction
    uint8_t aes : 1;        // AES Instruction Set
    uint8_t xsave : 1;      // XSAVE/XRSTOR/XSETBV/XGETBV
    uint8_t osxsave : 1;    // XSAVE enabled by OS
    uint8_t avx : 1;        // Advanced Vector Extensions
    uint8_t f16c : 1;       // F16C (half-precision) FP support
    uint8_t rdrand : 1;     // RDRAND Instruction

    // Extended features (0x80000001)
    uint8_t syscall : 1;    // SYSCALL/SYSRET
    uint8_t nx : 1;         // No-Execute Bit
    uint8_t mmxext : 1;     // Extended MMX
    uint8_t fxsr_opt : 1;   // FXSAVE/FXRSTOR optimizations
    uint8_t pdpe1gb : 1;    // 1GB pages
    uint8_t rdtscp : 1;     // RDTSCP instruction
    uint8_t lm : 1;         // Long mode (64-bit)
    uint8_t _3dnowext : 1;  // Extended 3DNow!
    uint8_t _3dnow : 1;     // 3DNow!

    // AMD-specific features
    uint8_t lahf_lm : 1;    // LAHF/SAHF in long mode
    uint8_t cmp_legacy : 1; // Core multi-processing legacy mode
    uint8_t svm : 1;        // Secure Virtual Machine
    uint8_t extapic : 1;    // Extended APIC space
    uint8_t cr8_legacy : 1; // CR8 in 32-bit mode
    uint8_t abm : 1;        // Advanced bit manipulation
    uint8_t sse4a : 1;      // SSE4A
    uint8_t misalignsse : 1;// Misaligned SSE mode
    uint8_t _3dnowprefetch : 1; // 3DNow! prefetch instructions
    uint8_t osvw : 1;       // OS Visible Workaround
    uint8_t ibs : 1;        // Instruction Based Sampling
    uint8_t xop : 1;        // Extended Operations
    uint8_t skinit : 1;     // SKINIT/STGI instructions
    uint8_t wdt : 1;        // Watchdog timer
    uint8_t lwp : 1;        // Light Weight Profiling
    uint8_t fma4 : 1;       // 4-operand FMA instructions
    uint8_t tce : 1;        // Translation Cache Extension
    uint8_t nodeid_msr : 1; // NodeID MSR
    uint8_t tbm : 1;        // Trailing Bit Manipulation
    uint8_t topoext : 1;    // Topology Extensions CPUID leafs
    uint8_t perfctr_core : 1; // Core performance counter extensions
    uint8_t perfctr_nb : 1; // NB performance counter extensions
    uint8_t dbx : 1;        // Data breakpoint extensions
    uint8_t perftsc : 1;    // Performance TSC
    uint8_t pcx_l2i : 1;    // L2I perf counter extensions
} CPUFeatures;

// CPU cache information structure
typedef struct {
    uint32_t l1_data_cache_size;            // KB
    uint32_t l1_data_cache_associativity;
    uint32_t l1_data_cache_line_size;       // bytes
    
    uint32_t l1_instruction_cache_size;     // KB
    uint32_t l1_instruction_cache_associativity;
    uint32_t l1_instruction_cache_line_size; // bytes
    
    uint32_t l2_cache_size;                 // KB
    uint32_t l2_cache_associativity;
    uint32_t l2_cache_line_size;            // bytes
    
    uint32_t l3_cache_size;                 // KB
    uint32_t l3_cache_associativity;
    uint32_t l3_cache_line_size;            // bytes
} CPUCacheInfo;

// CPU topology information
typedef struct {
    uint32_t packages;
    uint32_t physical_cores;
    uint32_t logical_processors;
    uint32_t threads_per_core;
    uint32_t cores_per_package;
} CPUTopology;

// CPU usage statistics
typedef struct {
    uint32_t user_time;     // Percentage
    uint32_t kernel_time;   // Percentage
    uint32_t idle_time;     // Percentage
    uint32_t total_time;    // Total time units
} CPUUsageStats;

// Main CPU information structure
typedef struct {
    char* vendor;
    char* brand_string;
    char* architecture;
    uint32_t family;
    uint32_t model;
    uint32_t stepping;
    uint32_t frequency_mhz;
    uint8_t is_64bit;
    
    CPUFeatures features;
    CPUCacheInfo cache_info;
    CPUTopology topology;
} CPUInfo;

// Function declarations
void get_cpuid(uint32_t function_id, CPUIDInfo *info);
void get_cpuid_extended(uint32_t function_id, uint32_t sub_function, CPUIDInfo *info);

char* get_cpu_vendor(void);
char* get_cpu_brand_string(void);
uint32_t get_cpu_frequency_mhz(void);
uint64_t get_cpu_timestamp(void);
uint32_t get_cpu_temperature(void);

void get_cpu_features(CPUFeatures* features);
void get_cpu_cache_info(CPUCacheInfo* cache_info);
void get_cpu_topology(CPUTopology* topology);
void get_cpu_usage_stats(CPUUsageStats* stats);

void initialize_cpu_info(void);
CPUInfo* get_cpu_info_struct(void);
void get_cpu_info(char** vendor, char** architecture, uint32_t* clock_speed);
void cleanup_cpu_info(void);

// Display functions
void print_cpu_features(void);
void print_cpu_cache_info(void);
void print_cpu_topology(void);
void print_detailed_cpu_info(void);
void print_cpu_usage(void);

// Performance and testing functions
void cpu_performance_test(void);
void cpu_stress_test(uint32_t duration_seconds);

// Monitoring functions
void start_cpu_monitoring(void);
void stop_cpu_monitoring(void);

// Control functions
void set_cpu_frequency(uint32_t frequency_mhz);
void enable_cpu_feature(const char* feature_name);
void disable_cpu_feature(const char* feature_name);

#endif // CPU_H
