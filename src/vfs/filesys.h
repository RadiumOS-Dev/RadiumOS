#ifndef FILESYS_H
#define FILESYS_H

#include <stddef.h>
#include <stdint.h>

#define MAX_FILENAME_LENGTH 256
#define MAX_PATH_LENGTH 512
#define MAX_FILES_PER_DIR 128
#define MAX_DIRECTORIES 64
#define FILE_BUFFER_SIZE 4096

// File types
typedef enum {
    FILE_TYPE_REGULAR = 0,
    FILE_TYPE_DIRECTORY = 1
} file_type_t;

// File entry structure
typedef struct {
    char name[MAX_FILENAME_LENGTH];
    file_type_t type;
    size_t size;
    char* content;
    uint32_t created_time;
    uint32_t modified_time;
    uint32_t parent_dir_id;
    uint32_t file_id;
} file_entry_t;

// Directory structure
typedef struct {
    char name[MAX_FILENAME_LENGTH];
    uint32_t dir_id;
    uint32_t parent_dir_id;
    file_entry_t* files[MAX_FILES_PER_DIR];
    size_t file_count;
    uint32_t created_time;
} directory_t;

// Filesystem structure
typedef struct {
    directory_t* directories[MAX_DIRECTORIES];
    size_t directory_count;
    uint32_t current_dir_id;
    char current_path[MAX_PATH_LENGTH];
    uint32_t next_file_id;
    uint32_t next_dir_id;
} filesystem_t;

// Function declarations
int filesys_init(void);
int filesys_create_file(const char* filename, const char* content);
int filesys_create_directory(const char* dirname);
int filesys_delete_file(const char* filename);
int filesys_delete_directory(const char* dirname);
int filesys_change_directory(const char* path);
int filesys_list_directory(void);
int filesys_read_file(const char* filename, char* buffer, size_t buffer_size);
int filesys_write_file(const char* filename, const char* content);
int filesys_file_exists(const char* filename);
int filesys_directory_exists(const char* dirname);
char* filesys_get_current_path(void);
directory_t* filesys_get_current_directory(void);
file_entry_t* filesys_find_file(const char* filename);
directory_t* filesys_find_directory(const char* dirname);
void filesys_print_tree(uint32_t dir_id, int depth);
int filesys_copy_file(const char* src, const char* dest);
int filesys_move_file(const char* src, const char* dest);
int filesys_get_file_info(const char* filename);
void filesys_cleanup(void);

// Command wrapper functions
void filesys_ls_command(int argc, char* argv[]);
void filesys_cd_command(int argc, char* argv[]);
void filesys_pwd_command(int argc, char* argv[]);
void filesys_mkdir_command(int argc, char* argv[]);
void filesys_rmdir_command(int argc, char* argv[]);
void filesys_rm_command(int argc, char* argv[]);
void filesys_cat_command(int argc, char* argv[]);
void filesys_cp_command(int argc, char* argv[]);
void filesys_mv_command(int argc, char* argv[]);
void filesys_tree_command(int argc, char* argv[]);
void filesys_stat_command(int argc, char* argv[]);


// Path utilities
char* filesys_normalize_path(const char* path);
char* filesys_get_parent_path(const char* path);
char* filesys_get_basename(const char* path);
int filesys_is_absolute_path(const char* path);

#endif // FILESYS_H
