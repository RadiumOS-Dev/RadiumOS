#ifndef UTILITY_H
#define UTILITY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h> // Include for boolean type
#include "../terminal/terminal.h"
static char* strtok_ptr = NULL;
static uint32_t seed = 123456789; // Initial seed value

// Function prototypes
extern size_t strlen(const char* str);
void srand(uint32_t new_seed);
int returnRandomInteger();
extern void reverse(char str[], int length);
extern void* memset(void* ptr, int value, size_t num);
extern void itoa(int num, char* str, int base);
extern int strcmp(const char* str1, const char* str2);
extern void* memcpy(void* dest, const void* src, size_t n);
extern char* strncpy(char* dest, const char* src, size_t n);
char* strtok(char* str, const char* delimiters);
char* strchr(const char* str, int character);
void int_to_string(int value, char* buffer);
int int_to_stringg(int value, char* buffer, int buffer_size);
int string_to_int(const char* str);
size_t strcspn(const char *s, const char *reject);
int strncmp(const char *s1, const char *s2, size_t n);
char* strdup(const char* s);
void* memcpy(void* dest, const void* src, size_t n);
char* strstr(const char* haystack, const char* needle);
bool starts_with(const char* str, const char* prefix);
bool ends_with(const char* str, const char* suffix);
char** split(const char* str, const char* delimiter, size_t* count);
bool has_keyword(const char* str, const char* keyword);
int memcmp(const void* ptr1, const void* ptr2, size_t num);
char* strrchr(const char* str, int c);
int int_to_str(char *mem, int max_len, int number);
int parse_int(const char* str);
bool isdigit(char c);
bool isspace(char c);
char toupper(char c);
char tolower(char c);
uint64_t __divdi3(uint64_t dividend, uint64_t divisor);
uint64_t __udivdi3(uint64_t dividend, uint64_t divisor);
uint64_t __moddi3(uint64_t dividend, uint64_t divisor);
void* realloc(void* ptr, size_t new_size);
int copy_string(char *dest, const char *src);
extern inline uint16_t htons(uint16_t hostshort);
extern inline uint16_t ntohs(uint16_t netshort);
extern inline uint32_t htonl(uint32_t hostlong);
// Memory management prototypes
void* malloc(size_t size);
void free(void* ptr);
void* kmalloc(size_t size);
char* strcat(char* dest, const char* src);
char* strcpy(char* dest, const char* src);
uint16_t calculate_checksum(uint16_t *buf, size_t length);
int atoi(const char *str);

uint64_t __umoddi3(uint64_t dividend, uint64_t divisor);
// sizeof function declarations
size_t sizeof_char(void);
size_t sizeof_short(void);
size_t sizeof_int(void);
size_t sizeof_long(void);
size_t sizeof_long_long(void);
size_t sizeof_float(void);
size_t sizeof_double(void);
size_t sizeof_pointer(void);
size_t sizeof_size_t(void);
size_t sizeof_uint8(void);
size_t sizeof_uint16(void);
size_t sizeof_uint32(void);
size_t sizeof_uint64(void);

// Generic sizeof function
size_t sizeof_type(int type_id);

// Type IDs for sizeof_type function
#define SIZEOF_TYPE_CHAR        0
#define SIZEOF_TYPE_SHORT       1
#define SIZEOF_TYPE_INT         2
#define SIZEOF_TYPE_LONG        3
#define SIZEOF_TYPE_LONG_LONG   4
#define SIZEOF_TYPE_FLOAT       5
#define SIZEOF_TYPE_DOUBLE      6
#define SIZEOF_TYPE_POINTER     7
#define SIZEOF_TYPE_SIZE_T      8
#define SIZEOF_TYPE_UINT8       9
#define SIZEOF_TYPE_UINT16      10
#define SIZEOF_TYPE_UINT32      11
#define SIZEOF_TYPE_UINT64      12

void done(const char* message, const char* file);
void warn(const char* message, const char* file);
void info(const char* message, const char* file);
void error(const char* message, const char* file);
void debug_memory_status();
int abs(int x);
#endif // UTILITY_H
