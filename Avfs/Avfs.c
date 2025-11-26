// Avfs.c - Simple RAM filesystem for OSDev

#include <stdint.h>
#include "../terminal/terminal.h"
#include "../utility/utility.h"
#include "Avfs.h"
#define AVFS_BLOCK_SIZE 512
#define AVFS_NUM_BLOCKS 1024
#define AVFS_MAX_FILES  64
#define AVFS_FILENAME_MAX 32

typedef struct {
    char name[AVFS_FILENAME_MAX];
    uint32_t size;
    uint32_t start_block;
    uint8_t used;
} avfs_file_entry_t;

typedef struct {
    uint8_t data[AVFS_BLOCK_SIZE * AVFS_NUM_BLOCKS];
    avfs_file_entry_t files[AVFS_MAX_FILES];
    uint8_t block_bitmap[AVFS_NUM_BLOCKS]; // 0 = free, 1 = used
} avfs_t;

static avfs_t avfs;

void avfs_init() {
    memset(&avfs, 0, sizeof(avfs));
}

static int avfs_find_free_blocks(int count) {
    int consecutive = 0;
    for (int i = 0; i < AVFS_NUM_BLOCKS; i++) {
        if (avfs.block_bitmap[i] == 0) {
            consecutive++;
            if (consecutive == count) {
                return i - count + 1;
            }
        } else {
            consecutive = 0;
        }
    }
    return -1; // no space
}

static int avfs_find_file(const char* name) {
    for (int i = 0; i < AVFS_MAX_FILES; i++) {
        if (avfs.files[i].used && strcmp(avfs.files[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// Returns the size of the file with given name, or -1 if not found
int avfs_get_filesize(const char* name) {
    int file_index = avfs_find_file(name);
    if (file_index == -1) {
        return -1; // file not found
    }
    return avfs.files[file_index].size;
}


int avfs_create_file(const char* name, uint32_t size) {
    if (avfs_find_file(name) != -1) {
        return -1; // file exists
    }
    int blocks_needed = (size + AVFS_BLOCK_SIZE - 1) / AVFS_BLOCK_SIZE;
    int start_block = avfs_find_free_blocks(blocks_needed);
    if (start_block == -1) {
        return -2; // no space
    }
    // Find free file entry
    int file_index = -1;
    for (int i = 0; i < AVFS_MAX_FILES; i++) {
        if (!avfs.files[i].used) {
            file_index = i;
            break;
        }
    }
    if (file_index == -1) {
        return -3; // no file entries free
    }
    // Mark blocks used
    for (int i = start_block; i < start_block + blocks_needed; i++) {
        avfs.block_bitmap[i] = 1;
    }
    // Setup file entry
    avfs_file_entry_t* f = &avfs.files[file_index];
    strncpy(f->name, name, AVFS_FILENAME_MAX - 1);
    f->name[AVFS_FILENAME_MAX - 1] = 0;
    f->size = size;
    f->start_block = start_block;
    f->used = 1;
    return 0; // success
}

#define MAX_CHECK_BUFFER_SIZE 4096  // Increased from 1024 for larger files; adjust based on your stack size
bool insideFile(const char* name, const char* search_str) {
    if (!name || !search_str) {
        return false;  // Invalid arguments
    }
    uint32_t search_len = strlen(search_str);
    if (search_len == 0) {
        return false;  // Empty search string
    }
    int file_size = avfs_get_filesize(name);
    if (file_size == -1) {
        return false;  // File not found
    }
    if ((uint32_t)file_size < search_len) {
        return false;  // File too small to contain search
    }
    if (file_size > MAX_CHECK_BUFFER_SIZE) {
        return false;  // File too large for buffer (safety; see alternative below for chunked search)
    }
    char buffer[MAX_CHECK_BUFFER_SIZE + 1];  // +1 for null terminator
    int read_result = avfs_read_file(name, buffer, file_size, 0);
    if (read_result != 0) {
        return false;  // Read failed
    }
    buffer[file_size] = '\0';  // Null-terminate the buffer (assumes no embedded nulls in text file)
    // strstr searches the *entire* null-terminated buffer as one string,
    // dynamically scanning byte-by-byte across all content, including newlines (\n),
    // spaces, or any other characters. It finds the substring anywhere, even spanning lines.
    return (strstr(buffer, search_str) != NULL);
}

// Reads file content into user-provided buffer
// Returns 0 on success, -1 on error
int avfs_get_content(const char* name, char* buffer, uint32_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return -1;
    }
    
    int file_size = avfs_get_filesize(name);
    if (file_size == -1) {
        return -1; // file not found
    }
    
    if ((uint32_t)file_size >= buffer_size) {
        return -1; // buffer too small
    }
    
    int read_result = avfs_read_file(name, buffer, file_size, 0);
    if (read_result != 0) {
        return -1; // read failed
    }
    
    buffer[file_size] = '\0'; // null-terminate
    return 0;
}

bool avfs_file_exists(const char* name) {
    if (avfs_get_filesize(name) > 0) {
        return true;
    }
    // Optional: Scan your file table for name
    return false;  // Implement full scan if needed
}

int avfs_write_file(const char* name, const void* buffer, uint32_t size, uint32_t offset) {
    int file_index = avfs_find_file(name);
    if (file_index == -1) return -1; // file not found
    avfs_file_entry_t* f = &avfs.files[file_index];
    if (offset + size > f->size) return -2; // out of bounds

    uint32_t start_addr = f->start_block * AVFS_BLOCK_SIZE + offset;
    memcpy(&avfs.data[start_addr], buffer, size);
    return 0;
}

int avfs_read_file(const char* name, void* buffer, uint32_t size, uint32_t offset) {
    int file_index = avfs_find_file(name);
    if (file_index == -1) return -1; // file not found
    avfs_file_entry_t* f = &avfs.files[file_index];
    if (offset + size > f->size) return -2; // out of bounds

    uint32_t start_addr = f->start_block * AVFS_BLOCK_SIZE + offset;
    memcpy(buffer, &avfs.data[start_addr], size);
    return 0;
}

void avfs_list_files() {
    printr("Avfs files:\n");
    for (int i = 0; i < AVFS_MAX_FILES; i++) {
        if (avfs.files[i].used) {
            printr(" - %s ", avfs.files[i].name);
            printr("(size: ");
            print_capacity(avfs.files[i].size);
            printr("\n");
        }
    }
}


int avfs_remove_file(const char* name) {
    int file_index = avfs_find_file(name);
    if (file_index == -1) {
        return -1; // file not found
    }

    avfs_file_entry_t* f = &avfs.files[file_index];
    int blocks_used = (f->size + AVFS_BLOCK_SIZE - 1) / AVFS_BLOCK_SIZE;

    // Free the blocks
    for (int i = f->start_block; i < f->start_block + blocks_used; i++) {
        avfs.block_bitmap[i] = 0;
    }

    // Clear the file entry
    memset(f, 0, sizeof(avfs_file_entry_t));

    return 0; // success
}

// Avfs.c - Add this function

// Appends data to an existing file
// Returns 0 on success, negative on error
int avfs_append_file(const char* name, const void* buffer, uint32_t size) {
    int file_index = avfs_find_file(name);
    if (file_index == -1) {
        return -1; // file not found
    }
    
    avfs_file_entry_t* f = &avfs.files[file_index];
    uint32_t old_size = f->size;
    uint32_t new_size = old_size + size;
    
    // Calculate blocks needed for old and new sizes
    int old_blocks = (old_size + AVFS_BLOCK_SIZE - 1) / AVFS_BLOCK_SIZE;
    int new_blocks = (new_size + AVFS_BLOCK_SIZE - 1) / AVFS_BLOCK_SIZE;
    
    // Check if we need more blocks
    if (new_blocks > old_blocks) {
        int additional_blocks = new_blocks - old_blocks;
        
        // Check if blocks after current file are free
        bool can_extend = true;
        for (int i = f->start_block + old_blocks; i < f->start_block + new_blocks; i++) {
            if (i >= AVFS_NUM_BLOCKS || avfs.block_bitmap[i] != 0) {
                can_extend = false;
                break;
            }
        }
        
        if (can_extend) {
            // Mark additional blocks as used
            for (int i = f->start_block + old_blocks; i < f->start_block + new_blocks; i++) {
                avfs.block_bitmap[i] = 1;
            }
        } else {
            // Need to relocate file to new location
            int new_start = avfs_find_free_blocks(new_blocks);
            if (new_start == -1) {
                return -2; // no space
            }
            
            // Copy old data to new location
            uint32_t old_addr = f->start_block * AVFS_BLOCK_SIZE;
            uint32_t new_addr = new_start * AVFS_BLOCK_SIZE;
            memcpy(&avfs.data[new_addr], &avfs.data[old_addr], old_size);
            
            // Free old blocks
            for (int i = f->start_block; i < f->start_block + old_blocks; i++) {
                avfs.block_bitmap[i] = 0;
            }
            
            // Mark new blocks as used
            for (int i = new_start; i < new_start + new_blocks; i++) {
                avfs.block_bitmap[i] = 1;
            }
            
            // Update file entry
            f->start_block = new_start;
        }
    }
    
    // Append the new data
    uint32_t append_addr = f->start_block * AVFS_BLOCK_SIZE + old_size;
    memcpy(&avfs.data[append_addr], buffer, size);
    
    // Update file size
    f->size = new_size;
    
    return 0; // success
}