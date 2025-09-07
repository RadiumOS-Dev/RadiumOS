#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>

// Main error handling function
void handle_error(const char* message, const char* errorType);

// Specific error type handlers
void handle_specific_error(const char* errorCode, const char* description);
void kernel_panic(const char* reason, const char* errorCode);
void hardware_fault(const char* component, const char* errorCode);
void memory_error(const char* operation, const char* errorCode);
void task_error(const char* taskName, const char* errorCode);
void system_error(const char* operation, const char* errorCode);
void meltdown_screen(char* message, char* file, int line, int error_code, int cr2, int int_no);
// Error recovery and logging
bool attempt_recovery(const char* errorType);
void log_error(const char* errorCode, const char* message);

#define ERROR_HARDWARE_UNKNOWN      "%x023_"
#define ERROR_HARDWARE_CPU          "%x023_41104"
#define ERROR_HARDWARE_CPU_GET      "%x023_09135"
#define ERROR_HARDWARE_GPU          "%x023_77201"
#define ERROR_HARDWARE_RAM          "%x023_88302"
#define ERROR_HARDWARE_DISK         "%x023_99403"
#define ERROR_HARDWARE_NET          "%x023_55504"

#define ERROR_KERNEL_UNKNOWN        "%x901_"
#define ERROR_KERNEL_MEMORY         "%x901_mpw41"
#define ERROR_KERNEL_STACK          "%x901_kst22"
#define ERROR_KERNEL_IRQ            "%x901_irq88"
#define ERROR_KERNEL_SCHED          "%x901_sched9"
#define ERROR_KERNEL_FS             "%x901_fs404"

#define ERROR_SYSTEM_UNKNOWN        "%x512_"
#define ERROR_SYSTEM_PERM           "%x512_perm1"
#define ERROR_SYSTEM_FILE           "%x512_file2"
#define ERROR_SYSTEM_PROC           "%x512_proc3"
#define ERROR_SYSTEM_SOCK           "%x512_sock4"

#define ERROR_MEMORY_UNKNOWN        "%x256_"
#define ERROR_MEMORY_ALLOC          "%x256_alloc1"
#define ERROR_MEMORY_SEGV           "%x256_segv11"
#define ERROR_MEMORY_LEAK           "%x256_leak22"
#define ERROR_MEMORY_BOUND          "%x256_bound3"

#define ERROR_TASK_UNKNOWN          "%x128_"
#define ERROR_TASK_DEAD             "%x128_dead1"
#define ERROR_TASK_TIME             "%x128_time2"
#define ERROR_TASK_SYNC             "%x128_sync3"
#define ERROR_TASK_PRIOR            "%x128_prior4"

// Operation type definitions
#define OP_NOP                      "Nop"  // No Operation
#define OP_DOP                      "Dop"  // Do Operation
#define OP_TOP                      "Top"  // Task Operation
#define OP_SOP                      "Sop"  // System Operation
#define OP_IOP                      "Iop"  // Interrupt Operation
#define OP_MOP                      "Mop"  // Memory Operation

// Error severity levels
typedef enum {
    ERROR_SEVERITY_INFO,
    ERROR_SEVERITY_WARNING,
    ERROR_SEVERITY_ERROR,
    ERROR_SEVERITY_CRITICAL,
    ERROR_SEVERITY_FATAL
} error_severity_t;

// Error structure for detailed error reporting
typedef struct {
    const char* code;
    const char* message;
    const char* operation;
    error_severity_t severity;
    bool recoverable;
} error_info_t;

// Advanced error handling functions
void report_error(const error_info_t* error);
void set_error_handler(void (*handler)(const error_info_t*));
void clear_error_log(void);
int get_error_count(void);

// Convenience macros for common error scenarios
#define PANIC(reason) kernel_panic(reason, ERROR_KERNEL_UNKNOWN)
#define HARDWARE_FAIL(component, code) hardware_fault(component, code)
#define MEMORY_FAIL(op, code) memory_error(op, code)
#define TASK_FAIL(task, code) task_error(task, code)
#define SYSTEM_FAIL(op, code) system_error(op, code)

#endif // ERROR_H
