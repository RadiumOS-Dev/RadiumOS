// Avfs.h - Header for Avfs RAM filesystem

#ifndef AVFS_H
#define AVFS_H

#include <stdint.h>
#include <stdbool.h>
#define AVFS_BLOCK_SIZE 512
#define AVFS_NUM_BLOCKS 1024
#define AVFS_MAX_FILES  64
#define AVFS_FILENAME_MAX 32

// Initialize the RAM filesystem
void avfs_init(void);

// Create a file with given name and size (in bytes)
// Returns 0 on success, negative on error:
//  -1 file exists
//  -2 no space
//  -3 no free file entries
int avfs_create_file(const char* name, uint32_t size);
int avfs_get_filesize(const char* name);
// Write data to a file at given offset
// Returns 0 on success, negative on error:
//  -1 file not found
//  -2 out of bounds
int avfs_write_file(const char* name, const void* buffer, uint32_t size, uint32_t offset);
bool avfs_file_exists(const char* name);
bool insideFile(const char* name, const char* search_str);
int avfs_get_content(const char* name, char* buffer, uint32_t buffer_size);
// Read data from a file at given offset
// Returns 0 on success, negative on error:
//  -1 file not found
//  -2 out of bounds
int avfs_read_file(const char* name, void* buffer, uint32_t size, uint32_t offset);
int avfs_remove_file(const char* name);
// List all files (prints to console or kernel log)
void avfs_list_files(void);
int avfs_append_file(const char* name, const void* buffer, uint32_t size);
#endif // AVFS_H
