#include "utility.h"
#define MEMORY_POOL_SIZE 1024
static char memory_pool[MEMORY_POOL_SIZE];
static size_t allocated_size = 0;

typedef struct Block {
    size_t size;
    struct Block* next;
} Block;

static Block* free_list = NULL;

char* strstr(const char* h, const char* n) {
    if (!*n) return (char*)h;
    for (; *h; h++) {
        const char* hh = h, *nn = n;
        while (*hh && *nn && (*hh == *nn)) { hh++; nn++; }
        if (!*nn) return (char*)h;
    }
    return NULL;
}

int int_to_str(char *mem, int max_len, int number) {
    
    int pos = 0;
    char temp[max_len];
    while((number > 0) && (pos < max_len)) {
        short digit = number % 10;
        number /= 10;
        temp[pos++] = (char)(digit + 48);
    }

    for(int i = (pos - 1); i > -1; --i) {//reverse it
        mem[(pos - i) - 1] = temp[i];
    }

    return pos;
}

int parse_int(const char* str) {
    if (!str) {
        return 0; // Handle null pointer
    }
    
    int result = 0;
    int i = 0;
    bool negative = false;
    
    // Skip leading whitespace
    while (str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r') {
        i++;
    }
    
    // Check for sign
    if (str[i] == '-') {
        negative = true;
        i++;
    } else if (str[i] == '+') {
        i++;
    }
    
    // Parse digits
    while (str[i] >= '0' && str[i] <= '9') {
        // Check for overflow (simple check)
        if (result > (2147483647 - (str[i] - '0')) / 10) {
            return negative ? -2147483648 : 2147483647; // Return max/min int on overflow
        }
        
        result = result * 10 + (str[i] - '0');
        i++;
    }
    
    return negative ? -result : result;
}


void* malloc(size_t size) {
    if (!size) return NULL;
    
    // Align size to pointer boundary (typically 8 bytes on 32-bit systems)
    size = (size + 7) & ~7;
    
    Block* current = free_list;
    Block* prev = NULL;
    
    // Search for a suitable free block
    while (current) {
        if (current->size >= size) {
            // Found a suitable block
            if (current->size >= size + sizeof(Block) + 8) {
                // Split the block if there's enough space for another block
                Block* new_block = (Block*)((char*)current + sizeof(Block) + size);
                new_block->size = current->size - size - sizeof(Block);
                new_block->next = current->next;
                
                current->size = size;
                current->next = NULL; // Mark as allocated
                
                // Update free list
                if (prev) {
                    prev->next = new_block;
                } else {
                    free_list = new_block;
                }
            } else {
                // Use the entire block
                if (prev) {
                    prev->next = current->next;
                } else {
                    free_list = current->next;
                }
                current->next = NULL; // Mark as allocated
            }
            
            // Clear the allocated memory
            memset((char*)current + sizeof(Block), 0, current->size);
            return (void*)((char*)current + sizeof(Block));
        }
        prev = current;
        current = current->next;
    }
    
    // No suitable free block found, allocate from the end of the pool
    size_t total_used = 0;
    
    // Calculate how much memory is actually used
    char* pool_end = memory_pool;
    Block* scan = (Block*)memory_pool;
    
    while ((char*)scan < memory_pool + MEMORY_POOL_SIZE) {
        if ((char*)scan + sizeof(Block) + scan->size > memory_pool + MEMORY_POOL_SIZE) {
            break; // Prevent overflow
        }
        
        pool_end = (char*)scan + sizeof(Block) + scan->size;
        
        // Find next block
        Block* next_block = (Block*)pool_end;
        if ((char*)next_block >= memory_pool + MEMORY_POOL_SIZE) {
            break;
        }
        
        // Check if this looks like a valid block
        if (next_block->size == 0 || next_block->size > MEMORY_POOL_SIZE) {
            break;
        }
        
        scan = next_block;
    }
    
    total_used = pool_end - memory_pool;
    
    // Check if we have enough space
    if (total_used + sizeof(Block) + size > MEMORY_POOL_SIZE) {
        return NULL; // Out of memory
    }
    
    // Allocate new block at the end
    Block* new_block = (Block*)pool_end;
    new_block->size = size;
    new_block->next = NULL; // Mark as allocated
    
    // Clear the allocated memory
    memset((char*)new_block + sizeof(Block), 0, size);
    
    return (void*)((char*)new_block + sizeof(Block));
}


void free(void* ptr) {
    if (!ptr) return;
    Block* block = (Block*)((char*)ptr - sizeof(Block));
    block->next = free_list;
    free_list = block;
}

char* strchr(const char* str, int c) {
    while (*str) if (*str++ == (char)c) return (char*)(str - 1);
    return NULL;
}

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

void reverse(char str[], int length) {
    for (int start = 0, end = length - 1; start < end; start++, end--) {
        char temp = str[start]; str[start] = str[end]; str[end] = temp;
    }
}

void* memset(void* ptr, int value, size_t num) {
    unsigned char* p = ptr;
    while (num--) *p++ = (unsigned char)value;
    return ptr;
}

int memcmp(const void* ptr1, const void* ptr2, size_t num) {
    const uint8_t* p1 = (const uint8_t*)ptr1;
    const uint8_t* p2 = (const uint8_t*)ptr2;
    for (size_t i = 0; i < num; i++) {
        if (p1[i] < p2[i]) return -1;
        if (p1[i] > p2[i]) return 1;
    }
    return 0;
}

void itoa(int num, char* str, int base) {
    int i = 0, isNegative = 0;
    if (num == 0) { str[i++] = '0'; str[i] = '\0'; return; }
    if (num < 0 && base == 10) { isNegative = 1; num = -num; }
    while (num) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'A' : rem + '0';
        num /= base;
    }
    if (isNegative) str[i++] = '-';
    str[i] = '\0';
    reverse(str, i);
}

void srand(uint32_t new_seed) { seed = new_seed; }

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n-- && *s1 && (*s1 == *s2)) { s1++; s2++; }
    return n ? *(unsigned char*)s1 - *(unsigned char*)s2 : 0;
}

size_t strcspn(const char *s, const char *reject) {
    size_t count = 0;
    while (s[count]) {
        const char *r = reject;
        while (*r) if (s[count] == *r++) return count;
        count++;
    }
    return count;
}

int returnRandomInteger() {
    const uint32_t a = 1664525, c = 1013904223, m = 4294967296;
    seed = (a * seed + c) % m;
    return (int)(seed % 100) + 1;
}

int strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) { str1++; str2++; }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i]; i++) dest[i] = src[i];
    for (; i < n; i++) dest[i] = '\0';
    return dest;
}

int string_to_int(const char* str) {
    int num = 0;
    while (*str >= '0' && *str <= '9') num = num * 10 + (*str++ - '0');
    return num;
}

char* strtok(char* str, const char* delimiters) {
    static char* strtok_ptr;
    if (str) strtok_ptr = str;
    if (!strtok_ptr) return NULL;
    while (*strtok_ptr && strchr(delimiters, *strtok_ptr)) strtok_ptr++;
    if (!*strtok_ptr) { strtok_ptr = NULL; return NULL; }
    char* token_start = strtok_ptr;
    while (*strtok_ptr && !strchr(delimiters, *strtok_ptr)) strtok_ptr++;
    if (*strtok_ptr) *strtok_ptr++ = '\0';
    else strtok_ptr = NULL;
    return token_start;
}

uint16_t calculate_checksum(uint16_t *buf, size_t length) {
    uint32_t sum = 0;
    for (size_t i = 0; i < length / 2; i++) sum += buf[i];
    if (length % 2) sum += ((uint8_t *)buf)[length - 1];
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)(~sum);
}

int atoi(const char *str) {
    int result = 0, sign = 1;
    while (*str == ' ') str++;
    if (*str == '-') { sign = -1; str++; }
    else if (*str == '+') str++;
    while (*str >= '0' && *str <= '9') {
        int digit = *str - '0';
        if (result > INT32_MAX / 10 || (result == INT32_MAX / 10 && digit > 7)) return (sign == 1) ? INT32_MAX : INT32_MIN;
        result = result * 10 + digit; str++;
    }
    return result * sign;
}

int int_to_stringg(int value, char* buffer, int buffer_size) {
    int index = 0, is_negative = 0;
    if (value < 0) { is_negative = 1; value = -value; }
    do { if (index < buffer_size - 1) buffer[index++] = (value % 10) + '0'; } while (value /= 10);
    if (is_negative && index < buffer_size - 1) buffer[index++] = '-';
    reverse(buffer, index);
    buffer[index] = '\0';
    return index;
}

void int_to_string(int value, char* buffer) {
    if (value == 0) { buffer[0] = '0'; buffer[1] = '\0'; return; }
    int is_negative = 0;
    if (value < 0) { is_negative = 1; value = -value; }
    int index = 0;
    while (value) { buffer[index++] = (value % 10) + '0'; value /= 10; }
    if (is_negative) buffer[index++] = '-';
    buffer[index] = '\0';
    reverse(buffer, index);
}

void* kmalloc(size_t size) {
    if (!size) return NULL;
    size = (size + sizeof(Block) - 1) & ~(sizeof(Block) - 1);
    Block* current = free_list, *prev = NULL;
    while (current) {
        if (current->size >= size) {
            if (current->size > size + sizeof(Block)) {
                Block* new_block = (Block*)((char*)current + sizeof(Block) + size);
                new_block->size = current->size - size - sizeof(Block);
                new_block->next = current->next;
                current->size = size;
                current->next = new_block;
            } else {
                if (prev) prev->next = current->next;
                else free_list = current->next;
            }
            allocated_size += current->size;
            return (void*)((char*)current + sizeof(Block));
        }
        prev = current;
        current = current->next;
    }
    if (allocated_size + size + sizeof(Block) > MEMORY_POOL_SIZE) return NULL;
    Block* new_block = (Block*)(memory_pool + allocated_size);
    new_block->size = size;
    allocated_size += size + sizeof(Block);
    return (void*)((char*)new_block + sizeof(Block));
}

char* strcat(char* dest, const char* src) {
    char* ptr = dest;
    while (*ptr) ptr++;
    while (*src) *ptr++ = *src++;
    *ptr = '\0';
    return dest;
}

char* strcpy(char* dest, const char* src) {
    char* original_dest = dest;
    while (*src) *dest++ = *src++;
    *dest = '\0';
    return original_dest;
}

inline uint16_t htons(uint16_t hostshort) {
    return (hostshort << 8) | (hostshort >> 8);
}

inline uint16_t ntohs(uint16_t netshort) {
    return (netshort << 8) | (netshort >> 8);
}

inline uint32_t htonl(uint32_t hostlong) {
    return ((hostlong & 0x000000FF) << 24) | ((hostlong & 0x0000FF00) << 8) | ((hostlong & 0x00FF0000) >> 8) | ((hostlong & 0xFF000000) >> 24);
}

void* memcpy(void* dest, const void* src, size_t n) {
    char* d = (char*)dest; const char* s = (const char*)src;
    for (size_t i = 0; i < n; i++) d[i] = s[i];
    return dest;
}

char* strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char* copy = (char*)malloc(len + 1);
    if (!copy) return NULL;
    strcpy(copy, s);
    return copy;
}

bool has_keyword(const char* str, const char* keyword) {
    return strstr(str, keyword) != NULL;
}

char** split(const char* str, const char* delimiter, size_t* count) {
    char* temp_str = strdup(str);
    char* token; char** result = NULL; *count = 0;
    token = strtok(temp_str, delimiter);
    while (token) {
        result = realloc(result, sizeof(char*) * (*count + 1));
        result[*count] = strdup(token);
        (*count)++;
        token = strtok(NULL, delimiter);
    }
    free(temp_str);
    return result;
}

bool ends_with(const char* str, const char* suffix) {
    size_t str_len = strlen(str), suffix_len = strlen(suffix);
    return suffix_len <= str_len && strcmp(str + str_len - suffix_len, suffix) == 0;
}

bool starts_with(const char* str, const char* prefix) {
    size_t prefix_len = strlen(prefix);
    return strncmp(str, prefix, prefix_len) == 0;
}

bool isdigit(char c) {
    return c >= '0' && c <= '9';
}

bool isspace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

// Utility functions implementation
char toupper(char c) {
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 'A';
    }
    return c;
}

char tolower(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 'a';
    }
    return c;
}


void* realloc(void* ptr, size_t new_size) {
    if (!new_size) { free(ptr); return NULL; }
    if (!ptr) return malloc(new_size);
    size_t current_size = *((size_t*)((char*)ptr - sizeof(size_t)));
    void* new_ptr = malloc(new_size);
    if (!new_ptr) return NULL;
    size_t copy_size = (new_size < current_size) ? new_size : current_size;
    memcpy(new_ptr, ptr, copy_size);
    free(ptr);
    return new_ptr;
}

int copy_string(char *dest, const char *src) {
    int i = 0;
    while (src[i]) { dest[i] = src[i]; i++; }
    dest[i] = '\0';
    return i;
}

char* strrchr(const char* str, int c) {
    char* last = NULL;
    while (*str) {
        if (*str == c) {
            last = (char*)str;
        }
        str++;
    }
    return last;
}

uint64_t __udivdi3(uint64_t dividend, uint64_t divisor) {
    if (divisor == 0) {
        // handle division by zero if needed
        return 0;
    }
    uint64_t quotient = 0;
    uint64_t rem = 0;
    for (int i = 63; i >= 0; i--) {
        rem <<= 1;
        rem |= (dividend >> i) & 1;
        if (rem >= divisor) {
            rem -= divisor;
            quotient |= ((uint64_t)1 << i);
        }
    }
    return quotient;
}

uint64_t __divdi3(uint64_t dividend, uint64_t divisor) {
    if (divisor == 0) {
        // handle division by zero if needed
        return 0;
    }
    uint64_t quotient = 0;
    uint64_t rem = 0;
    for (int i = 63; i >= 0; i--) {
        rem <<= 1;
        rem |= (dividend >> i) & 1;
        if (rem >= divisor) {
            rem -= divisor;
            quotient |= ((uint64_t)1 << i);
        }
    }
    return quotient;
}
uint64_t __moddi3(uint64_t dividend, uint64_t divisor) {
    if (divisor == 0) {
        // handle division by zero if needed
        return 0;
    }
    uint64_t rem = 0;
    for (int i = 63; i >= 0; i--) {
        rem <<= 1;
        rem |= (dividend >> i) & 1;
        if (rem >= divisor) {
            rem -= divisor;
        }
    }
    return rem;
}

uint64_t __umoddi3(uint64_t dividend, uint64_t divisor) {
    if (divisor == 0) {
        // handle division by zero if needed
        return 0;
    }
    uint64_t rem = 0;
    for (int i = 63; i >= 0; i--) {
        rem <<= 1;
        rem |= (dividend >> i) & 1;
        if (rem >= divisor) {
            rem -= divisor;
        }
    }
    return rem;
}

// sizeof function implementations using pointer arithmetic
size_t sizeof_char(void) {
    return (size_t)((char*)1 - (char*)0);
}

size_t sizeof_short(void) {
    return (size_t)((short*)1 - (short*)0);
}

size_t sizeof_int(void) {
    return (size_t)((int*)1 - (int*)0);
}

size_t sizeof_long(void) {
    return (size_t)((long*)1 - (long*)0);
}

size_t sizeof_long_long(void) {
    return (size_t)((long long*)1 - (long long*)0);
}

size_t sizeof_float(void) {
    return (size_t)((float*)1 - (float*)0);
}

size_t sizeof_double(void) {
    return (size_t)((double*)1 - (double*)0);
}

size_t sizeof_pointer(void) {
    return (size_t)((void**)1 - (void**)0);
}

size_t sizeof_size_t(void) {
    return (size_t)((size_t*)1 - (size_t*)0);
}

size_t sizeof_uint8(void) {
    return (size_t)((uint8_t*)1 - (uint8_t*)0);
}

size_t sizeof_uint16(void) {
    return (size_t)((uint16_t*)1 - (uint16_t*)0);
}

size_t sizeof_uint32(void) {
    return (size_t)((uint32_t*)1 - (uint32_t*)0);
}

size_t sizeof_uint64(void) {
    return (size_t)((uint64_t*)1 - (uint64_t*)0);
}

// Generic sizeof function that takes a type ID
size_t sizeof_type(int type_id) {
    switch (type_id) {
        case SIZEOF_TYPE_CHAR:
            return sizeof_char();
        case SIZEOF_TYPE_SHORT:
            return sizeof_short();
        case SIZEOF_TYPE_INT:
            return sizeof_int();
        case SIZEOF_TYPE_LONG:
            return sizeof_long();
        case SIZEOF_TYPE_LONG_LONG:
            return sizeof_long_long();
        case SIZEOF_TYPE_FLOAT:
            return sizeof_float();
        case SIZEOF_TYPE_DOUBLE:
            return sizeof_double();
        case SIZEOF_TYPE_POINTER:
            return sizeof_pointer();
        case SIZEOF_TYPE_SIZE_T:
            return sizeof_size_t();
        case SIZEOF_TYPE_UINT8:
            return sizeof_uint8();
        case SIZEOF_TYPE_UINT16:
            return sizeof_uint16();
        case SIZEOF_TYPE_UINT32:
            return sizeof_uint32();
        case SIZEOF_TYPE_UINT64:
            return sizeof_uint64();
        default:
            return 0;
    }
}

// Helper function to get sizeof for structures
size_t sizeof_struct(size_t member_count, ...) {
    // This would require variadic arguments support
    // For now, return 0 as placeholder
    return 0;
}

// Array sizeof function
size_t sizeof_array(size_t element_size, size_t count) {
    return element_size * count;
}

// Function to calculate structure size with padding
size_t sizeof_struct_with_padding(size_t* member_sizes, size_t member_count) {
    if (!member_sizes || member_count == 0) {
        return 0;
    }
    
    size_t total_size = 0;
    size_t max_alignment = 1;
    
    // Find maximum alignment requirement
    for (size_t i = 0; i < member_count; i++) {
        if (member_sizes[i] > max_alignment) {
            max_alignment = member_sizes[i];
        }
    }
    
    // Calculate size with padding
    for (size_t i = 0; i < member_count; i++) {
        // Align current position
        size_t alignment = member_sizes[i];
        if (alignment > sizeof_pointer()) {
            alignment = sizeof_pointer();
        }
        
        if (total_size % alignment != 0) {
            total_size += alignment - (total_size % alignment);
        }
        
        total_size += member_sizes[i];
    }
    
    // Final padding to align entire structure
    if (total_size % max_alignment != 0) {
        total_size += max_alignment - (total_size % max_alignment);
    }
    
    return total_size;
}


void warn(const char* message, const char* file) {
    if (!message) return;
    
    // Set color to yellow/brown for warning messages
    terminal_setcolor(VGA_COLOR_BROWN);
    print("[WARN");
    
    if (file) {
        print(":");
        print(file);
    }
    
    print("] ");
    terminal_setcolor(VGA_COLOR_WHITE);
    print(message);
    print("\n");
    
    // Reset to default color
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
}
/**
 * @brief Display a success/completion message with file context
 * 
 * @param message The success message to display
 * @param file The source file name where the completion occurred
 */
void done(const char* message, const char* file) {
    if (!message) return;
    
    // Set color to green for success messages
    terminal_setcolor(VGA_COLOR_GREEN);
    print("[DONE");
    
    if (file) {
        print(":");
        print(file);
    }
    
    print("] ");
    terminal_setcolor(VGA_COLOR_WHITE);
    print(message);
    print("\n");
    
    // Reset to default color
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
}

void info(const char* message, const char* file) {
    if (!message) return;
    
    // Set color to cyan for info messages
    terminal_setcolor(VGA_COLOR_CYAN);
    print("[INFO");
    
    if (file) {
        print(":");
        print(file);
    }
    
    print("] ");
    terminal_setcolor(VGA_COLOR_WHITE);
    print(message);
    print("\n");
    
    // Reset to default color
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
}

void error(const char* message, const char* file) {
    if (!message) return;
    
    // Set color to red for error messages
    terminal_setcolor(VGA_COLOR_RED);
    print("[ERROR");
    
    if (file) {
        print(":");
        print(file);
    }
    
    print("] ");
    terminal_setcolor(VGA_COLOR_WHITE);
    print(message);
    print("\n");
    
    // Reset to default color
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
}

// Add this function to debug memory issues
void debug_memory_status() {
    info("Checking memory status...", __FILE__);
    
    // Try allocating small amounts to see what works
    void* test1 = malloc(1024);
    if (test1) {
        info("1KB allocation: SUCCESS", __FILE__);
        free(test1);
    } else {
        warn("1KB allocation: FAILED", __FILE__);
    }
    
    void* test2 = malloc(4096);
    if (test2) {
        info("4KB allocation: SUCCESS", __FILE__);
        free(test2);
    } else {
        warn("4KB allocation: FAILED", __FILE__);
    }
    
    void* test3 = malloc(8192);
    if (test3) {
        info("8KB allocation: SUCCESS", __FILE__);
        free(test3);
    } else {
        warn("8KB allocation: FAILED", __FILE__);
    }
}
