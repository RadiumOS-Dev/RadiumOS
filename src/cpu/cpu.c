#include "cpu.h"
#include "../utility/utility.h"
#include "../terminal/terminal.h"

// CPU feature flags for CPUID function 1 (EDX register)
#define CPUID_FEAT_EDX_FPU      (1 << 0)   // Floating Point Unit
#define CPUID_FEAT_EDX_VME      (1 << 1)   // Virtual Mode Extension
#define CPUID_FEAT_EDX_DE       (1 << 2)   // Debugging Extension
#define CPUID_FEAT_EDX_PSE      (1 << 3)   // Page Size Extension
#define CPUID_FEAT_EDX_TSC      (1 << 4)   // Time Stamp Counter
#define CPUID_FEAT_EDX_MSR      (1 << 5)   // Model Specific Registers
#define CPUID_FEAT_EDX_PAE      (1 << 6)   // Physical Address Extension
#define CPUID_FEAT_EDX_MCE      (1 << 7)   // Machine Check Exception
#define CPUID_FEAT_EDX_CX8      (1 << 8)   // CMPXCHG8B Instruction
#define CPUID_FEAT_EDX_APIC     (1 << 9)   // Advanced PIC
#define CPUID_FEAT_EDX_SEP      (1 << 11)  // SYSENTER/SYSEXIT
#define CPUID_FEAT_EDX_MTRR     (1 << 12)  // Memory Type Range Registers
#define CPUID_FEAT_EDX_PGE      (1 << 13)  // Page Global Enable
#define CPUID_FEAT_EDX_MCA      (1 << 14)  // Machine Check Architecture
#define CPUID_FEAT_EDX_CMOV     (1 << 15)  // Conditional Move Instructions
#define CPUID_FEAT_EDX_PAT      (1 << 16)  // Page Attribute Table
#define CPUID_FEAT_EDX_PSE36    (1 << 17)  // 36-bit Page Size Extension
#define CPUID_FEAT_EDX_PSN      (1 << 18)  // Processor Serial Number
#define CPUID_FEAT_EDX_CLF      (1 << 19)  // CLFLUSH Instruction
#define CPUID_FEAT_EDX_DTES     (1 << 21)  // Debug Trace Store
#define CPUID_FEAT_EDX_ACPI     (1 << 22)  // ACPI Support
#define CPUID_FEAT_EDX_MMX      (1 << 23)  // MMX Technology
#define CPUID_FEAT_EDX_FXSR     (1 << 24)  // FXSAVE/FXRSTOR Instructions
#define CPUID_FEAT_EDX_SSE      (1 << 25)  // SSE Instructions
#define CPUID_FEAT_EDX_SSE2     (1 << 26)  // SSE2 Instructions
#define CPUID_FEAT_EDX_SS       (1 << 27)  // Self Snoop
#define CPUID_FEAT_EDX_HTT      (1 << 28)  // Hyper-Threading Technology
#define CPUID_FEAT_EDX_TM1      (1 << 29)  // Thermal Monitor
#define CPUID_FEAT_EDX_IA64     (1 << 30)  // IA64 Processor
#define CPUID_FEAT_EDX_PBE      (1 << 31)  // Pending Break Enable

// CPU feature flags for CPUID function 1 (ECX register)
#define CPUID_FEAT_ECX_SSE3     (1 << 0)   // SSE3 Instructions
#define CPUID_FEAT_ECX_PCLMUL   (1 << 1)   // PCLMULQDQ Instruction
#define CPUID_FEAT_ECX_DTES64   (1 << 2)   // 64-bit Debug Store
#define CPUID_FEAT_ECX_MONITOR  (1 << 3)   // MONITOR/MWAIT Instructions
#define CPUID_FEAT_ECX_DS_CPL   (1 << 4)   // CPL Qualified Debug Store
#define CPUID_FEAT_ECX_VMX      (1 << 5)   // Virtual Machine Extensions
#define CPUID_FEAT_ECX_SMX      (1 << 6)   // Safer Mode Extensions
#define CPUID_FEAT_ECX_EST      (1 << 7)   // Enhanced SpeedStep Technology
#define CPUID_FEAT_ECX_TM2      (1 << 8)   // Thermal Monitor 2
#define CPUID_FEAT_ECX_SSSE3    (1 << 9)   // Supplemental SSE3
#define CPUID_FEAT_ECX_CID      (1 << 10)  // Context ID
#define CPUID_FEAT_ECX_FMA      (1 << 12)  // Fused Multiply Add
#define CPUID_FEAT_ECX_CX16     (1 << 13)  // CMPXCHG16B Instruction
#define CPUID_FEAT_ECX_ETPRD    (1 << 14)  // xTPR Update Control
#define CPUID_FEAT_ECX_PDCM     (1 << 15)  // Performance/Debug Capability MSR
#define CPUID_FEAT_ECX_PCIDE    (1 << 17)  // Process Context Identifiers
#define CPUID_FEAT_ECX_DCA      (1 << 18)  // Direct Cache Access
#define CPUID_FEAT_ECX_SSE4_1   (1 << 19)  // SSE4.1 Instructions
#define CPUID_FEAT_ECX_SSE4_2   (1 << 20)  // SSE4.2 Instructions
#define CPUID_FEAT_ECX_x2APIC   (1 << 21)  // x2APIC Support
#define CPUID_FEAT_ECX_MOVBE    (1 << 22)  // MOVBE Instruction
#define CPUID_FEAT_ECX_POPCNT   (1 << 23)  // POPCNT Instruction
#define CPUID_FEAT_ECX_AES      (1 << 25)  // AES Instruction Set
#define CPUID_FEAT_ECX_XSAVE    (1 << 26)  // XSAVE/XRSTOR/XSETBV/XGETBV
#define CPUID_FEAT_ECX_OSXSAVE  (1 << 27)  // XSAVE enabled by OS
#define CPUID_FEAT_ECX_AVX      (1 << 28)  // Advanced Vector Extensions
#define CPUID_FEAT_ECX_F16C     (1 << 29)  // F16C (half-precision) FP support
#define CPUID_FEAT_ECX_RDRAND   (1 << 30)  // RDRAND Instruction

// Global CPU information structure
static CPUInfo g_cpu_info = {0};
static int g_cpu_info_initialized = 0;

void get_cpuid(uint32_t function_id, CPUIDInfo *info) {
    __asm__ volatile (
        "cpuid"
        : "=a" (info->eax), "=b" (info->ebx), "=c" (info->ecx), "=d" (info->edx)
        : "a" (function_id)
    );
}

void get_cpuid_extended(uint32_t function_id, uint32_t sub_function, CPUIDInfo *info) {
    __asm__ volatile (
        "cpuid"
        : "=a" (info->eax), "=b" (info->ebx), "=c" (info->ecx), "=d" (info->edx)
        : "a" (function_id), "c" (sub_function)
    );
}

char* get_cpu_vendor() {
    CPUIDInfo info;
    get_cpuid(0, &info);
    
    char* vendor = (char*)malloc(13 * sizeof(char));
    if (vendor == NULL) {
        return NULL;
    }

    *((uint32_t*)&vendor[0]) = info.ebx;
    *((uint32_t*)&vendor[4]) = info.edx;
    *((uint32_t*)&vendor[8]) = info.ecx;
    vendor[12] = '\0';

    return vendor;
}

char* get_cpu_brand_string() {
    CPUIDInfo info;
    char* brand = (char*)malloc(49 * sizeof(char));
    if (brand == NULL) {
        return NULL;
    }

    // Check if extended functions are available
    get_cpuid(0x80000000, &info);
    if (info.eax < 0x80000004) {
        free(brand);
        return NULL;
    }

    // Get brand string from extended CPUID functions
    get_cpuid(0x80000002, &info);
    *((uint32_t*)&brand[0]) = info.eax;
    *((uint32_t*)&brand[4]) = info.ebx;
    *((uint32_t*)&brand[8]) = info.ecx;
    *((uint32_t*)&brand[12]) = info.edx;

    get_cpuid(0x80000003, &info);
    *((uint32_t*)&brand[16]) = info.eax;
    *((uint32_t*)&brand[20]) = info.ebx;
    *((uint32_t*)&brand[24]) = info.ecx;
    *((uint32_t*)&brand[28]) = info.edx;

    get_cpuid(0x80000004, &info);
    *((uint32_t*)&brand[32]) = info.eax;
    *((uint32_t*)&brand[36]) = info.ebx;
    *((uint32_t*)&brand[40]) = info.ecx;
    *((uint32_t*)&brand[44]) = info.edx;

    brand[48] = '\0';
    return brand;
}

void get_cpu_features(CPUFeatures* features) {
    CPUIDInfo info;
    
    // Initialize all features to false
    memset(features, 0, sizeof(CPUFeatures));
    
    // Get basic features
    get_cpuid(1, &info);
    
    // Parse EDX features
    features->fpu = (info.edx & CPUID_FEAT_EDX_FPU) != 0;
    features->vme = (info.edx & CPUID_FEAT_EDX_VME) != 0;
    features->de = (info.edx & CPUID_FEAT_EDX_DE) != 0;
    features->pse = (info.edx & CPUID_FEAT_EDX_PSE) != 0;
    features->tsc = (info.edx & CPUID_FEAT_EDX_TSC) != 0;
    features->msr = (info.edx & CPUID_FEAT_EDX_MSR) != 0;
    features->pae = (info.edx & CPUID_FEAT_EDX_PAE) != 0;
    features->mce = (info.edx & CPUID_FEAT_EDX_MCE) != 0;
    features->cx8 = (info.edx & CPUID_FEAT_EDX_CX8) != 0;
    features->apic = (info.edx & CPUID_FEAT_EDX_APIC) != 0;
    features->sep = (info.edx & CPUID_FEAT_EDX_SEP) != 0;
    features->mtrr = (info.edx & CPUID_FEAT_EDX_MTRR) != 0;
    features->pge = (info.edx & CPUID_FEAT_EDX_PGE) != 0;
    features->mca = (info.edx & CPUID_FEAT_EDX_MCA) != 0;
    features->cmov = (info.edx & CPUID_FEAT_EDX_CMOV) != 0;
    features->pat = (info.edx & CPUID_FEAT_EDX_PAT) != 0;
    features->pse36 = (info.edx & CPUID_FEAT_EDX_PSE36) != 0;
    features->psn = (info.edx & CPUID_FEAT_EDX_PSN) != 0;
    features->clflush = (info.edx & CPUID_FEAT_EDX_CLF) != 0;
    features->dts = (info.edx & CPUID_FEAT_EDX_DTES) != 0;
    features->acpi = (info.edx & CPUID_FEAT_EDX_ACPI) != 0;
    features->mmx = (info.edx & CPUID_FEAT_EDX_MMX) != 0;
    features->fxsr = (info.edx & CPUID_FEAT_EDX_FXSR) != 0;
    features->sse = (info.edx & CPUID_FEAT_EDX_SSE) != 0;
    features->sse2 = (info.edx & CPUID_FEAT_EDX_SSE2) != 0;
    features->ss = (info.edx & CPUID_FEAT_EDX_SS) != 0;
    features->htt = (info.edx & CPUID_FEAT_EDX_HTT) != 0;
    features->tm = (info.edx & CPUID_FEAT_EDX_TM1) != 0;
    features->pbe = (info.edx & CPUID_FEAT_EDX_PBE) != 0;

    // Parse ECX features
    features->sse3 = (info.ecx & CPUID_FEAT_ECX_SSE3) != 0;
    features->pclmulqdq = (info.ecx & CPUID_FEAT_ECX_PCLMUL) != 0;
    features->dtes64 = (info.ecx & CPUID_FEAT_ECX_DTES64) != 0;
    features->monitor = (info.ecx & CPUID_FEAT_ECX_MONITOR) != 0;
    features->ds_cpl = (info.ecx & CPUID_FEAT_ECX_DS_CPL) != 0;
    features->vmx = (info.ecx & CPUID_FEAT_ECX_VMX) != 0;
    features->smx = (info.ecx & CPUID_FEAT_ECX_SMX) != 0;
    features->est = (info.ecx & CPUID_FEAT_ECX_EST) != 0;
    features->tm2 = (info.ecx & CPUID_FEAT_ECX_TM2) != 0;
    features->ssse3 = (info.ecx & CPUID_FEAT_ECX_SSSE3) != 0;
    features->cnxt_id = (info.ecx & CPUID_FEAT_ECX_CID) != 0;
    features->fma = (info.ecx & CPUID_FEAT_ECX_FMA) != 0;
        features->cx16 = (info.ecx & CPUID_FEAT_ECX_CX16) != 0;
    features->xtpr = (info.ecx & CPUID_FEAT_ECX_ETPRD) != 0;
    features->pdcm = (info.ecx & CPUID_FEAT_ECX_PDCM) != 0;
    features->pcid = (info.ecx & CPUID_FEAT_ECX_PCIDE) != 0;
    features->dca = (info.ecx & CPUID_FEAT_ECX_DCA) != 0;
    features->sse4_1 = (info.ecx & CPUID_FEAT_ECX_SSE4_1) != 0;
    features->sse4_2 = (info.ecx & CPUID_FEAT_ECX_SSE4_2) != 0;
    features->x2apic = (info.ecx & CPUID_FEAT_ECX_x2APIC) != 0;
    features->movbe = (info.ecx & CPUID_FEAT_ECX_MOVBE) != 0;
    features->popcnt = (info.ecx & CPUID_FEAT_ECX_POPCNT) != 0;
    features->aes = (info.ecx & CPUID_FEAT_ECX_AES) != 0;
    features->xsave = (info.ecx & CPUID_FEAT_ECX_XSAVE) != 0;
    features->osxsave = (info.ecx & CPUID_FEAT_ECX_OSXSAVE) != 0;
    features->avx = (info.ecx & CPUID_FEAT_ECX_AVX) != 0;
    features->f16c = (info.ecx & CPUID_FEAT_ECX_F16C) != 0;
    features->rdrand = (info.ecx & CPUID_FEAT_ECX_RDRAND) != 0;

    // Check for extended features
    get_cpuid(0x80000001, &info);
    features->syscall = (info.edx & (1 << 11)) != 0;
    features->nx = (info.edx & (1 << 20)) != 0;
    features->mmxext = (info.edx & (1 << 22)) != 0;
    features->fxsr_opt = (info.edx & (1 << 25)) != 0;
    features->pdpe1gb = (info.edx & (1 << 26)) != 0;
    features->rdtscp = (info.edx & (1 << 27)) != 0;
    features->lm = (info.edx & (1 << 29)) != 0; // Long mode (64-bit)
    features->_3dnowext = (info.edx & (1 << 30)) != 0;
    features->_3dnow = (info.edx & (1 << 31)) != 0;

    features->lahf_lm = (info.ecx & (1 << 0)) != 0;
    features->cmp_legacy = (info.ecx & (1 << 1)) != 0;
    features->svm = (info.ecx & (1 << 2)) != 0;
    features->extapic = (info.ecx & (1 << 3)) != 0;
    features->cr8_legacy = (info.ecx & (1 << 4)) != 0;
    features->abm = (info.ecx & (1 << 5)) != 0;
    features->sse4a = (info.ecx & (1 << 6)) != 0;
    features->misalignsse = (info.ecx & (1 << 7)) != 0;
    features->_3dnowprefetch = (info.ecx & (1 << 8)) != 0;
    features->osvw = (info.ecx & (1 << 9)) != 0;
    features->ibs = (info.ecx & (1 << 10)) != 0;
    features->xop = (info.ecx & (1 << 11)) != 0;
    features->skinit = (info.ecx & (1 << 12)) != 0;
    features->wdt = (info.ecx & (1 << 13)) != 0;
    features->lwp = (info.ecx & (1 << 15)) != 0;
    features->fma4 = (info.ecx & (1 << 16)) != 0;
    features->tce = (info.ecx & (1 << 17)) != 0;
    features->nodeid_msr = (info.ecx & (1 << 19)) != 0;
    features->tbm = (info.ecx & (1 << 21)) != 0;
    features->topoext = (info.ecx & (1 << 22)) != 0;
    features->perfctr_core = (info.ecx & (1 << 23)) != 0;
    features->perfctr_nb = (info.ecx & (1 << 24)) != 0;
    features->dbx = (info.ecx & (1 << 26)) != 0;
    features->perftsc = (info.ecx & (1 << 27)) != 0;
    features->pcx_l2i = (info.ecx & (1 << 28)) != 0;
}

void get_cpu_cache_info(CPUCacheInfo* cache_info) {
    CPUIDInfo info;
    
    // Initialize cache info
    memset(cache_info, 0, sizeof(CPUCacheInfo));
    
    // Get cache information using function 0x80000005 and 0x80000006
    get_cpuid(0x80000000, &info);
    if (info.eax >= 0x80000005) {
        // L1 cache info
        get_cpuid(0x80000005, &info);
        cache_info->l1_data_cache_size = (info.ecx >> 24) & 0xFF; // KB
        cache_info->l1_data_cache_associativity = (info.ecx >> 16) & 0xFF;
        cache_info->l1_data_cache_line_size = info.ecx & 0xFF;
        
        cache_info->l1_instruction_cache_size = (info.edx >> 24) & 0xFF; // KB
        cache_info->l1_instruction_cache_associativity = (info.edx >> 16) & 0xFF;
        cache_info->l1_instruction_cache_line_size = info.edx & 0xFF;
    }
    
    if (info.eax >= 0x80000006) {
        // L2 and L3 cache info
        get_cpuid(0x80000006, &info);
        cache_info->l2_cache_size = (info.ecx >> 16) & 0xFFFF; // KB
        cache_info->l2_cache_associativity = (info.ecx >> 12) & 0xF;
        cache_info->l2_cache_line_size = info.ecx & 0xFF;
        
        cache_info->l3_cache_size = ((info.edx >> 18) & 0x3FFF) * 512; // KB
        cache_info->l3_cache_associativity = (info.edx >> 12) & 0xF;
        cache_info->l3_cache_line_size = info.edx & 0xFF;
    }
}

uint64_t get_cpu_timestamp() {
    uint32_t low, high;
    __asm__ volatile ("rdtsc" : "=a" (low), "=d" (high));
    return ((uint64_t)high << 32) | low;
}

uint32_t get_cpu_frequency_mhz() {
    CPUIDInfo info;
    
    // Try to get frequency from CPUID function 0x16 (newer processors)
    get_cpuid(0x16, &info);
    if (info.eax != 0) {
        return info.eax; // Base frequency in MHz
    }
    
    // Fallback: estimate using TSC (less accurate)
    uint64_t start_tsc = get_cpu_timestamp();
    
    // Simple delay loop (not very accurate)
    for (volatile int i = 0; i < 1000000; i++);
    
    uint64_t end_tsc = get_cpu_timestamp();
    uint64_t cycles = end_tsc - start_tsc;
    
    // Very rough estimation - this would need proper timing
    return (uint32_t)(cycles / 1000); // Rough MHz estimate
}

void get_cpu_topology(CPUTopology* topology) {
    CPUIDInfo info;
    
    memset(topology, 0, sizeof(CPUTopology));
    
    // Get basic processor info
    get_cpuid(1, &info);
    topology->logical_processors = (info.ebx >> 16) & 0xFF;
    
    // Check if topology enumeration is supported
    get_cpuid(0, &info);
    if (info.eax >= 0xB) {
        // Use topology enumeration leaf
        get_cpuid_extended(0xB, 0, &info);
        topology->threads_per_core = info.ebx & 0xFFFF;
        
        get_cpuid_extended(0xB, 1, &info);
        topology->cores_per_package = info.ebx & 0xFFFF;
        
        if (topology->threads_per_core > 0) {
            topology->physical_cores = topology->cores_per_package / topology->threads_per_core;
        } else {
            topology->physical_cores = topology->cores_per_package;
            topology->threads_per_core = 1;
        }
    } else {
        // Fallback method
        topology->physical_cores = topology->logical_processors;
        topology->threads_per_core = 1;
        topology->cores_per_package = topology->logical_processors;
    }
    
    topology->packages = 1; // Assume single package for now
}

void initialize_cpu_info() {
    if (g_cpu_info_initialized) {
        return;
    }
    
    // Get basic CPU information
    g_cpu_info.vendor = get_cpu_vendor();
    g_cpu_info.brand_string = get_cpu_brand_string();
    g_cpu_info.frequency_mhz = get_cpu_frequency_mhz();
    
    // Get CPU features
    get_cpu_features(&g_cpu_info.features);
    
    // Get cache information
    get_cpu_cache_info(&g_cpu_info.cache_info);
    
    // Get topology information
    get_cpu_topology(&g_cpu_info.topology);
    
    // Determine architecture
    CPUIDInfo info;
    get_cpuid(1, &info);
    uint32_t family = ((info.eax >> 8) & 0xF) + ((info.eax >> 20) & 0xFF);
    uint32_t model = ((info.eax >> 4) & 0xF) | ((info.eax >> 12) & 0xF0);
    uint32_t stepping = info.eax & 0xF;
    
    g_cpu_info.family = family;
    g_cpu_info.model = model;
    g_cpu_info.stepping = stepping;
    
    // Determine if 64-bit capable
    if (g_cpu_info.features.lm) {
        g_cpu_info.architecture = "x86_64";
        g_cpu_info.is_64bit = 1;
    } else {
        g_cpu_info.architecture = "x86";
        g_cpu_info.is_64bit = 0;
    }
    
    g_cpu_info_initialized = 1;
}

CPUInfo* get_cpu_info_struct() {
    if (!g_cpu_info_initialized) {
        initialize_cpu_info();
    }
    return &g_cpu_info;
}

void get_cpu_info(char** vendor, char** architecture, uint32_t* clock_speed) {
    if (!g_cpu_info_initialized) {
        initialize_cpu_info();
    }
    
    *vendor = g_cpu_info.vendor;
    *architecture = g_cpu_info.architecture;
    *clock_speed = g_cpu_info.frequency_mhz;
}

void print_cpu_features() {
    if (!g_cpu_info_initialized) {
        initialize_cpu_info();
    }
    
    CPUFeatures* features = &g_cpu_info.features;
    
    print("CPU Features:\n");
    print("=============\n");
    
    // Basic features
    if (features->fpu) print("  FPU: Floating Point Unit\n");
    if (features->vme) print("  VME: Virtual Mode Extension\n");
    if (features->de) print("  DE: Debugging Extension\n");
    if (features->pse) print("  PSE: Page Size Extension\n");
    if (features->tsc) print("  TSC: Time Stamp Counter\n");
    if (features->msr) print("  MSR: Model Specific Registers\n");
    if (features->pae) print("  PAE: Physical Address Extension\n");
    if (features->mce) print("  MCE: Machine Check Exception\n");
    if (features->cx8) print("  CX8: CMPXCHG8B Instruction\n");
    if (features->apic) print("  APIC: Advanced PIC\n");
    if (features->sep) print("  SEP: SYSENTER/SYSEXIT\n");
    if (features->mtrr) print("  MTRR: Memory Type Range Registers\n");
    if (features->pge) print("  PGE: Page Global Enable\n");
    if (features->mca) print("  MCA: Machine Check Architecture\n");
    if (features->cmov) print("  CMOV: Conditional Move Instructions\n");
    if (features->pat) print("  PAT: Page Attribute Table\n");
    if (features->pse36) print("  PSE36: 36-bit Page Size Extension\n");
    if (features->clflush) print("  CLFLUSH: Cache Line Flush\n");
    if (features->mmx) print("  MMX: MMX Technology\n");
    if (features->fxsr) print("  FXSR: FXSAVE/FXRSTOR Instructions\n");
    if (features->sse) print("  SSE: Streaming SIMD Extensions\n");
    if (features->sse2) print("  SSE2: Streaming SIMD Extensions 2\n");
    if (features->sse3) print("  SSE3: Streaming SIMD Extensions 3\n");
    if (features->ssse3) print("  SSSE3: Supplemental SSE3\n");
    if (features->sse4_1) print("  SSE4.1: Streaming SIMD Extensions 4.1\n");
    if (features->sse4_2) print("  SSE4.2: Streaming SIMD Extensions 4.2\n");
    if (features->avx) print("  AVX: Advanced Vector Extensions\n");
    if (features->aes) print("  AES: AES Instruction Set\n");
    if (features->rdrand) print("  RDRAND: Random Number Generator\n");
    if (features->htt) print("  HTT: Hyper-Threading Technology\n");
    if (features->vmx) print("  VMX: Virtual Machine Extensions\n");
    if (features->smx) print("  SMX: Safer Mode Extensions\n");
        if (features->lm) print("  LM: Long Mode (64-bit)\n");
    if (features->nx) print("  NX: No-Execute Bit\n");
    if (features->syscall) print("  SYSCALL: Fast System Call\n");
    if (features->svm) print("  SVM: Secure Virtual Machine\n");
    if (features->fma) print("  FMA: Fused Multiply-Add\n");
    if (features->f16c) print("  F16C: Half-precision Float Conversion\n");
    if (features->popcnt) print("  POPCNT: Population Count\n");
    if (features->movbe) print("  MOVBE: Move Data After Swapping Bytes\n");
    if (features->xsave) print("  XSAVE: Extended State Save/Restore\n");
    if (features->osxsave) print("  OSXSAVE: XSAVE Enabled by OS\n");
    if (features->monitor) print("  MONITOR: MONITOR/MWAIT Instructions\n");
    if (features->x2apic) print("  x2APIC: Extended xAPIC Support\n");
    if (features->pclmulqdq) print("  PCLMULQDQ: Carry-less Multiplication\n");
    if (features->est) print("  EST: Enhanced SpeedStep Technology\n");
    if (features->tm) print("  TM: Thermal Monitor\n");
    if (features->tm2) print("  TM2: Thermal Monitor 2\n");
    if (features->dca) print("  DCA: Direct Cache Access\n");
    if (features->sse4a) print("  SSE4A: SSE4A Instructions\n");
    if (features->_3dnow) print("  3DNow!: 3DNow! Instructions\n");
    if (features->_3dnowext) print("  3DNow!+: Extended 3DNow! Instructions\n");
    if (features->abm) print("  ABM: Advanced Bit Manipulation\n");
    if (features->xop) print("  XOP: Extended Operations\n");
    if (features->fma4) print("  FMA4: 4-operand Fused Multiply-Add\n");
    if (features->tbm) print("  TBM: Trailing Bit Manipulation\n");
}

void print_cpu_cache_info() {
    if (!g_cpu_info_initialized) {
        initialize_cpu_info();
    }
    
    CPUCacheInfo* cache = &g_cpu_info.cache_info;
    
    print("CPU Cache Information:\n");
    print("======================\n");
    
    if (cache->l1_data_cache_size > 0) {
        char buffer[64];
        print("  L1 Data Cache: ");
        itoa(cache->l1_data_cache_size, buffer, 10);
        print(buffer);
        print(" KB, ");
        itoa(cache->l1_data_cache_associativity, buffer, 10);
        print(buffer);
        print("-way, ");
        itoa(cache->l1_data_cache_line_size, buffer, 10);
        print(buffer);
        print(" byte line size\n");
    }
    
    if (cache->l1_instruction_cache_size > 0) {
        char buffer[64];
        print("  L1 Instruction Cache: ");
        itoa(cache->l1_instruction_cache_size, buffer, 10);
        print(buffer);
        print(" KB, ");
        itoa(cache->l1_instruction_cache_associativity, buffer, 10);
        print(buffer);
        print("-way, ");
        itoa(cache->l1_instruction_cache_line_size, buffer, 10);
        print(buffer);
        print(" byte line size\n");
    }
    
    if (cache->l2_cache_size > 0) {
        char buffer[64];
        print("  L2 Cache: ");
        itoa(cache->l2_cache_size, buffer, 10);
        print(buffer);
        print(" KB, ");
        itoa(cache->l2_cache_associativity, buffer, 10);
        print(buffer);
        print("-way, ");
        itoa(cache->l2_cache_line_size, buffer, 10);
        print(buffer);
        print(" byte line size\n");
    }
    
    if (cache->l3_cache_size > 0) {
        char buffer[64];
        print("  L3 Cache: ");
        itoa(cache->l3_cache_size, buffer, 10);
        print(buffer);
        print(" KB, ");
        itoa(cache->l3_cache_associativity, buffer, 10);
        print(buffer);
        print("-way, ");
        itoa(cache->l3_cache_line_size, buffer, 10);
        print(buffer);
        print(" byte line size\n");
    }
}

void print_cpu_topology() {
    if (!g_cpu_info_initialized) {
        initialize_cpu_info();
    }
    
    CPUTopology* topology = &g_cpu_info.topology;
    
    print("CPU Topology:\n");
    print("=============\n");
    
    char buffer[64];
    print("  Packages: ");
    itoa(topology->packages, buffer, 10);
    print(buffer);
    print("\n");
    
    print("  Physical Cores: ");
    itoa(topology->physical_cores, buffer, 10);
    print(buffer);
    print("\n");
    
    print("  Logical Processors: ");
    itoa(topology->logical_processors, buffer, 10);
    print(buffer);
    print("\n");
    
    print("  Threads per Core: ");
    itoa(topology->threads_per_core, buffer, 10);
    print(buffer);
    print("\n");
    
    print("  Cores per Package: ");
    itoa(topology->cores_per_package, buffer, 10);
    print(buffer);
    print("\n");
}

void print_detailed_cpu_info() {
    if (!g_cpu_info_initialized) {
        initialize_cpu_info();
    }
    
    char buffer[64];
    
    print("Detailed CPU Information:\n");
    print("=========================\n");
    
    print("  Vendor: ");
    if (g_cpu_info.vendor) {
        print(g_cpu_info.vendor);
    } else {
        print("Unknown");
    }
    print("\n");
    
    print("  Brand: ");
    if (g_cpu_info.brand_string) {
        print(g_cpu_info.brand_string);
    } else {
        print("Unknown");
    }
    print("\n");
    
    print("  Architecture: ");
    print(g_cpu_info.architecture);
    print("\n");
    
    print("  Family: ");
    itoa(g_cpu_info.family, buffer, 10);
    print(buffer);
    print("\n");
    
    print("  Model: ");
    itoa(g_cpu_info.model, buffer, 10);
    print(buffer);
    print("\n");
    
    print("  Stepping: ");
    itoa(g_cpu_info.stepping, buffer, 10);
    print(buffer);
    print("\n");
    
    print("  Frequency: ");
    itoa(g_cpu_info.frequency_mhz, buffer, 10);
    print(buffer);
    print(" MHz\n");
    
    print("  64-bit Capable: ");
    print(g_cpu_info.is_64bit ? "Yes" : "No");
    print("\n\n");
    
    print_cpu_topology();
    print("\n");
    print_cpu_cache_info();
    print("\n");
    print_cpu_features();
}

uint32_t get_cpu_temperature() {
    // This is a simplified implementation
    // Real temperature reading would require MSR access and vendor-specific methods
    uint32_t temp = 0;
    
    // Try to read temperature from MSR (Model Specific Register)
    // This is just a placeholder - actual implementation would need MSR support
    // and would be vendor-specific (Intel vs AMD)
    
    return temp; // Returns 0 for now - placeholder
}

void cpu_performance_test() {
    print("Running CPU Performance Test...\n");
    
    uint64_t start_time = get_cpu_timestamp();
    
    // Simple arithmetic test
    volatile uint32_t result = 0;
    for (volatile uint32_t i = 0; i < 1000000; i++) {
        result += i * i;
        result -= i / 2;
        result ^= i;
    }
    
    uint64_t end_time = get_cpu_timestamp();
    uint64_t cycles = end_time - start_time;
    
    char buffer[64];
    print("  Arithmetic Test Cycles: ");
    // Convert 64-bit to string (simplified)
    itoa((uint32_t)(cycles & 0xFFFFFFFF), buffer, 10);
    print(buffer);
    print("\n");
    
    // Memory access test
    start_time = get_cpu_timestamp();
    
    volatile uint32_t* test_array = (volatile uint32_t*)malloc(1024 * sizeof(uint32_t));
    if (test_array) {
        for (int i = 0; i < 1024; i++) {
            test_array[i] = i;
        }
        
        for (int i = 0; i < 1000; i++) {
            for (int j = 0; j < 1024; j++) {
                result += test_array[j];
            }
        }
        
        free((void*)test_array);
    }
    
    end_time = get_cpu_timestamp();
    cycles = end_time - start_time;
    
    print("  Memory Access Test Cycles: ");
    itoa((uint32_t)(cycles & 0xFFFFFFFF), buffer, 10);
    print(buffer);
    print("\n");
    
    print("Performance test completed.\n");
}

void cpu_stress_test(uint32_t duration_seconds) {
    print("Running CPU Stress Test for ");
    char buffer[32];
    itoa(duration_seconds, buffer, 10);
    print(buffer);
    print(" seconds...\n");
    
    uint64_t start_time = get_cpu_timestamp();
    uint64_t target_cycles = (uint64_t)duration_seconds * g_cpu_info.frequency_mhz * 1000000;
    
    volatile uint32_t dummy = 0;
    uint64_t iterations = 0;
    
    while ((get_cpu_timestamp() - start_time) < target_cycles) {
        // Intensive calculations
        for (int i = 0; i < 1000; i++) {
            dummy += i * i * i;
            dummy ^= (dummy << 1);
            dummy -= (dummy >> 2);
        }
        iterations++;
        
        // Check for keyboard interrupt to stop early

    }
    
    print("Stress test completed. Iterations: ");
    itoa((uint32_t)(iterations & 0xFFFFFFFF), buffer, 10);
    print(buffer);
    print("\n");
}

void cleanup_cpu_info() {
    if (g_cpu_info_initialized) {
        if (g_cpu_info.vendor) {
            free(g_cpu_info.vendor);
            g_cpu_info.vendor = NULL;
        }
        if (g_cpu_info.brand_string) {
            free(g_cpu_info.brand_string);
            g_cpu_info.brand_string = NULL;
        }
        g_cpu_info_initialized = 0;
    }
}

// CPU monitoring functions
void start_cpu_monitoring() {
    // Initialize CPU monitoring
    // This would set up periodic sampling of CPU metrics
    print("CPU monitoring started.\n");
}

void stop_cpu_monitoring() {
    // Stop CPU monitoring
    print("CPU monitoring stopped.\n");
}

void get_cpu_usage_stats(CPUUsageStats* stats) {
    // This would calculate CPU usage statistics
    // For now, just initialize to zero
    memset(stats, 0, sizeof(CPUUsageStats));
    
    // Placeholder values
    stats->user_time = 25;
    stats->kernel_time = 10;
    stats->idle_time = 65;
    stats->total_time = 100;
}

void print_cpu_usage() {
    CPUUsageStats stats;
    get_cpu_usage_stats(&stats);
    
    char buffer[32];
    print("CPU Usage:\n");
    print("  User: ");
    itoa(stats.user_time, buffer, 10);
    print(buffer);
    print("%\n");
    
    print("  Kernel: ");
    itoa(stats.kernel_time, buffer, 10);
    print(buffer);
    print("%\n");
    
    print("  Idle: ");
    itoa(stats.idle_time, buffer, 10);
    print(buffer);
    print("%\n");
}

// Advanced CPU control functions
void set_cpu_frequency(uint32_t frequency_mhz) {
    // This would set CPU frequency using ACPI or vendor-specific methods
    // Placeholder implementation
    char buffer[32];
    print("Setting CPU frequency to ");
    itoa(frequency_mhz, buffer, 10);
    print(buffer);
    print(" MHz (not implemented)\n");
}

void enable_cpu_feature(const char* feature_name) {
    // Enable specific CPU features if supported
    print("Enabling CPU feature: ");
    print(feature_name);
    print(" (not implemented)\n");
}

void disable_cpu_feature(const char* feature_name) {
    // Disable specific CPU features if supported
    print("Disabling CPU feature: ");
    print(feature_name);
    print(" (not implemented)\n");
}
