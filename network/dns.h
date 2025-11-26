// dns.h - DNS Client Header
#ifndef DNS_H
#define DNS_H

#include <stdint.h>
#include <stdbool.h>

// DNS Header Structure
typedef struct {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;  // Question count
    uint16_t ancount;  // Answer count
    uint16_t nscount;  // Authority count
    uint16_t arcount;  // Additional count
} __attribute__((packed)) dns_header_t;

// DNS Question Structure
typedef struct {
    // Name is variable length, handled separately
    uint16_t qtype;
    uint16_t qclass;
} __attribute__((packed)) dns_question_t;

// DNS Answer Structure
typedef struct {
    uint16_t name;      // Pointer to name
    uint16_t type;
    uint16_t class;
    uint32_t ttl;
    uint16_t rdlength;
    uint32_t rdata;     // IPv4 address
} __attribute__((packed)) dns_answer_t;

// DNS Configuration
#define DNS_PORT 53
#define DNS_QUERY_TIMEOUT_MS 5000

// DNS Query Types
#define DNS_TYPE_A      1   // IPv4 address
#define DNS_TYPE_AAAA   28  // IPv6 address
#define DNS_TYPE_CNAME  5   // Canonical name

// DNS Classes
#define DNS_CLASS_IN    1   // Internet

// Function Prototypes
bool dns_resolve(const char* hostname, uint32_t* ip_out);
void dns_set_server(uint32_t dns_ip);
uint32_t dns_get_server(void);

#endif // DNS_H
