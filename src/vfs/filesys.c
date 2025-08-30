#include "filesys.h"
#include "../terminal/terminal.h"
#include "../utility/utility.h"

static filesystem_t* fs = NULL;

// Update current path string
void filesys_update_current_path(void) {
    if (!fs) return;
    
    if (fs->current_dir_id == 0) {
        strcpy(fs->current_path, "/");
        return;
    }
    
    // Build path by traversing up to root
    char temp_path[MAX_PATH_LENGTH] = "";
    uint32_t dir_id = fs->current_dir_id;
    
    while (dir_id != 0 && dir_id < fs->directory_count) {
        directory_t* dir = fs->directories[dir_id];
        if (!dir) break;
        
        char new_temp[MAX_PATH_LENGTH];
        strcpy(new_temp, "/");
        strcat(new_temp, dir->name);
        strcat(new_temp, temp_path);
        strcpy(temp_path, new_temp);
        
        dir_id = dir->parent_dir_id;
    }
    
    if (strlen(temp_path) == 0) {
        strcpy(fs->current_path, "/");
    } else {
        strcpy(fs->current_path, temp_path);
    }
}

// Initialize the filesystem
int filesys_init(void) {
    fs = (filesystem_t*)malloc(sizeof(filesystem_t));
    if (!fs) {
        return -1;
    }
    
    // Initialize filesystem structure
    memset(fs, 0, sizeof(filesystem_t));
    fs->next_file_id = 1;
    fs->next_dir_id = 1;
    
    // Create root directory
    directory_t* root = (directory_t*)malloc(sizeof(directory_t));

    
    strcpy(root->name, "/");
    root->dir_id = 0;
    root->parent_dir_id = 0;
    root->file_count = 0;
    root->created_time = 0; 
    
    fs->directories[0] = root;
    fs->directory_count = 1;
    fs->current_dir_id = 0;
    strcpy(fs->current_path, "/");
    
    return 0;
}

// Get current directory
directory_t* filesys_get_current_directory(void) {
    if (!fs || fs->current_dir_id >= fs->directory_count) {
        return NULL;
    }
    return fs->directories[fs->current_dir_id];
}

// Find file in current directory
file_entry_t* filesys_find_file(const char* filename) {
    directory_t* current_dir = filesys_get_current_directory();
    if (!current_dir) {
        return NULL;
    }
    
    for (size_t i = 0; i < current_dir->file_count; i++) {
        if (current_dir->files[i] && strcmp(current_dir->files[i]->name, filename) == 0) {
            return current_dir->files[i];
        }
    }
    return NULL;
}

// Find directory by name in current directory
directory_t* filesys_find_directory(const char* dirname) {
    directory_t* current_dir = filesys_get_current_directory();
    if (!current_dir) {
        return NULL;
    }
    
    // Handle special cases
    if (strcmp(dirname, ".") == 0) {
        return current_dir;
    }
    
    if (strcmp(dirname, "..") == 0) {
        if (current_dir->parent_dir_id < fs->directory_count) {
            return fs->directories[current_dir->parent_dir_id];
        }
        return current_dir; // Root directory
    }
    
    // Search for directory
    for (size_t i = 0; i < fs->directory_count; i++) {
        if (fs->directories[i] && 
            fs->directories[i]->parent_dir_id == fs->current_dir_id &&
            strcmp(fs->directories[i]->name, dirname) == 0) {
            return fs->directories[i];
        }
    }
    return NULL;
}

// Create a new file
int filesys_create_file(const char* filename, const char* content) {
    if (!fs || !filename) {
        return -1;
    }
    
    directory_t* current_dir = filesys_get_current_directory();
    if (!current_dir || current_dir->file_count >= MAX_FILES_PER_DIR) {
        return -1;
    }
    
    // Check if file already exists
    if (filesys_find_file(filename)) {
        return -2; // File already exists
    }
    
    // Create new file entry
    file_entry_t* new_file = (file_entry_t*)malloc(sizeof(file_entry_t));
    if (!new_file) {
        return -1;
    }
    
    strcpy(new_file->name, filename);
    new_file->type = FILE_TYPE_REGULAR;
    new_file->file_id = fs->next_file_id++;
    new_file->parent_dir_id = fs->current_dir_id;
    new_file->created_time = 0; // Implement proper time
    new_file->modified_time = 0;
    
    // Allocate and copy content
    if (content) {
        new_file->size = strlen(content);
        new_file->content = (char*)malloc(new_file->size + 1);
        if (!new_file->content) {
            free(new_file);
            return -1;
        }
        strcpy(new_file->content, content);
    } else {
        new_file->size = 0;
        new_file->content = NULL;
    }
    
    // Add file to current directory
    current_dir->files[current_dir->file_count] = new_file;
    current_dir->file_count++;
    
    return 0;
}

// Create a new directory
int filesys_create_directory(const char* dirname) {
    if (!fs || !dirname || fs->directory_count >= MAX_DIRECTORIES) {
        return -1;
    }
    
    // Check if directory already exists
    if (filesys_find_directory(dirname)) {
        return -2; // Directory already exists
    }
    
    // Create new directory
    directory_t* new_dir = (directory_t*)malloc(sizeof(directory_t));
    if (!new_dir) {
        return -1;
    }
    
    strcpy(new_dir->name, dirname);
    new_dir->dir_id = fs->next_dir_id++;
    new_dir->parent_dir_id = fs->current_dir_id;
    new_dir->file_count = 0;
    new_dir->created_time = 0; // Implement proper time
    
    fs->directories[fs->directory_count] = new_dir;
    fs->directory_count++;
    
    return 0;
}

// Change directory
int filesys_change_directory(const char* path) {
    if (!fs || !path) {
        return -1;
    }
    
    directory_t* target_dir = NULL;
    
    // Handle absolute path
    if (path[0] == '/') {
        target_dir = fs->directories[0]; // Root directory
        fs->current_dir_id = 0;
        strcpy(fs->current_path, "/");
        
        // If path is just "/", we're done
        if (strlen(path) == 1) {
            return 0;
        }
        
        // Parse the rest of the path
        char* path_copy = strdup(path + 1); // Skip the leading '/'
        char* token = strtok(path_copy, "/");
        
        while (token) {
            directory_t* next_dir = filesys_find_directory(token);
            if (!next_dir) {
                free(path_copy);
                return -1; // Directory not found
            }
            fs->current_dir_id = next_dir->dir_id;
            token = strtok(NULL, "/");
        }
        
        free(path_copy);
        
        // Update current path
        strcpy(fs->current_path, path);
        return 0;
    }
    
    // Handle relative path
    char* path_copy = strdup(path);
    char* token = strtok(path_copy, "/");
    
    while (token) {
        directory_t* next_dir = filesys_find_directory(token);
        if (!next_dir) {
            free(path_copy);
            return -1; // Directory not found
        }
        fs->current_dir_id = next_dir->dir_id;
        token = strtok(NULL, "/");
    }
    
    free(path_copy);
    
    // Update current path
    filesys_update_current_path();
    return 0;
}



// List directory contents
int filesys_list_directory(void) {
    directory_t* current_dir = filesys_get_current_directory();
    if (!current_dir) {
        return -1;
    }
    
    print("\nDirectory listing for: ");
    print(fs->current_path);
    print("\n\n");
    
    // Show directories first
    for (size_t i = 0; i < fs->directory_count; i++) {
        if (fs->directories[i] && fs->directories[i]->parent_dir_id == fs->current_dir_id) {
            print("[DIR]  ");
            print(fs->directories[i]->name);
            print("/\n");
        }
    }
    
    // Show files
    for (size_t i = 0; i < current_dir->file_count; i++) {
        if (current_dir->files[i]) {
            print("[FILE] ");
            print(current_dir->files[i]->name);
            print(" (");
            char size_str[32];
            itoa(current_dir->files[i]->size, size_str, 10);
            print(size_str);
            print(" bytes)\n");
        }
    }
    
    print("\n");
    return 0;
}

// Read file content
int filesys_read_file(const char* filename, char* buffer, size_t buffer_size) {
    file_entry_t* file = filesys_find_file(filename);
    if (!file || !buffer) {
        return -1;
    }
    
    if (file->size >= buffer_size) {
        return -2; // Buffer too small
    }
    
    if (file->content) {
        strcpy(buffer, file->content);
        return file->size;
    }
    
    buffer[0] = '\0';
    return 0;
}

// Write file content
int filesys_write_file(const char* filename, const char* content) {
    file_entry_t* file = filesys_find_file(filename);
    
    if (file) {
        // File exists, update content
        if (file->content) {
            free(file->content);
        }
        
        if (content) {
            file->size = strlen(content);
            file->content = (char*)malloc(file->size + 1);
            if (!file->content) {
                return -1;
            }
            strcpy(file->content, content);
        } else {
            file->size = 0;
            file->content = NULL;
        }
        
        file->modified_time = 0; // Update with proper time
        return 0;
    } else {
        // File doesn't exist, create it
        return filesys_create_file(filename, content);
    }
}

// Check if file exists
int filesys_file_exists(const char* filename) {
    return filesys_find_file(filename) != NULL;
}

// Check if directory exists
int filesys_directory_exists(const char* dirname) {
    return filesys_find_directory(dirname) != NULL;
}

// Get current path
char* filesys_get_current_path(void) {
    return fs ? fs->current_path : NULL;
}


int filesys_delete_file(const char* filename) {
    directory_t* current_dir = filesys_get_current_directory();
    if (!current_dir) {
        return -1;
    }
    
    for (size_t i = 0; i < current_dir->file_count; i++) {
        if (current_dir->files[i] && strcmp(current_dir->files[i]->name, filename) == 0) {
            // Free file content and entry
            if (current_dir->files[i]->content) {
                free(current_dir->files[i]->content);
            }
            free(current_dir->files[i]);
            
            // Shift remaining files
            for (size_t j = i; j < current_dir->file_count - 1; j++) {
                current_dir->files[j] = current_dir->files[j + 1];
            }
            current_dir->file_count--;
            current_dir->files[current_dir->file_count] = NULL;
            
            return 0;
        }
    }
    return -1; // File not found
}

// Delete directory (recursive)
int filesys_delete_directory(const char* dirname) {
    directory_t* target_dir = filesys_find_directory(dirname);
    if (!target_dir || target_dir->dir_id == 0) { // Can't delete root
        return -1;
    }
    
    // Delete all files in the directory
    for (size_t i = 0; i < target_dir->file_count; i++) {
        if (target_dir->files[i]) {
            if (target_dir->files[i]->content) {
                free(target_dir->files[i]->content);
            }
            free(target_dir->files[i]);
        }
    }
    
    // Delete all subdirectories recursively
    for (size_t i = 0; i < fs->directory_count; i++) {
        if (fs->directories[i] && fs->directories[i]->parent_dir_id == target_dir->dir_id) {
            // Temporarily change to parent to delete subdirectory
            uint32_t old_current = fs->current_dir_id;
            fs->current_dir_id = target_dir->dir_id;
            filesys_delete_directory(fs->directories[i]->name);
            fs->current_dir_id = old_current;
        }
    }
    
    // Remove directory from filesystem
    for (size_t i = 0; i < fs->directory_count; i++) {
        if (fs->directories[i] == target_dir) {
            free(fs->directories[i]);
            
            // Shift remaining directories
            for (size_t j = i; j < fs->directory_count - 1; j++) {
                fs->directories[j] = fs->directories[j + 1];
                // Update directory IDs
                if (fs->directories[j]) {
                    fs->directories[j]->dir_id = j;
                }
            }
            fs->directory_count--;
            fs->directories[fs->directory_count] = NULL;
            
            // Update current directory if we deleted it
            if (fs->current_dir_id >= fs->directory_count) {
                fs->current_dir_id = 0; // Go to root
                strcpy(fs->current_path, "/");
            }
            
            return 0;
        }
    }
    return -1;
}

// Print filesystem tree
void filesys_print_tree(uint32_t dir_id, int depth) {
    if (!fs || dir_id >= fs->directory_count || !fs->directories[dir_id]) {
        return;
    }
    
    directory_t* dir = fs->directories[dir_id];
    
    // Print indentation
    for (int i = 0; i < depth; i++) {
        print("  ");
    }
    
    if (depth == 0) {
        print("/\n");
    } else {
        print(dir->name);
        print("/\n");
    }
    
    // Print files in this directory
    for (size_t i = 0; i < dir->file_count; i++) {
        if (dir->files[i]) {
            for (int j = 0; j < depth + 1; j++) {
                print("  ");
            }
            print(dir->files[i]->name);
            print("\n");
        }
    }
    
    // Print subdirectories recursively
    for (size_t i = 0; i < fs->directory_count; i++) {
        if (fs->directories[i] && fs->directories[i]->parent_dir_id == dir_id && i != dir_id) {
            filesys_print_tree(i, depth + 1);
        }
    }
}

// Copy file
int filesys_copy_file(const char* src, const char* dest) {
    file_entry_t* src_file = filesys_find_file(src);
    if (!src_file) {
        return -1; // Source file not found
    }
    
    // Check if destination already exists
    if (filesys_find_file(dest)) {
        return -2; // Destination already exists
    }
    
    return filesys_create_file(dest, src_file->content);
}

// Move file
int filesys_move_file(const char* src, const char* dest) {
    if (filesys_copy_file(src, dest) == 0) {
        return filesys_delete_file(src);
    }
    return -1;
}

// Get file information
int filesys_get_file_info(const char* filename) {
    file_entry_t* file = filesys_find_file(filename);
    if (!file) {
        print("File not found: ");
        print(filename);
        print("\n");
        return -1;
    }
    
    print("\nFile Information:\n");
    print("Name: ");
    print(file->name);
    print("\nSize: ");
    char size_str[32];
    itoa(file->size, size_str, 10);
    print(size_str);
    print(" bytes\n");
    print("Type: ");
    print(file->type == FILE_TYPE_REGULAR ? "Regular file" : "Directory");
    print("\nFile ID: ");
    char id_str[32];
    itoa(file->file_id, id_str, 10);
    print(id_str);
    print("\n\n");
    
    return 0;
}

// Path utilities
char* filesys_normalize_path(const char* path) {
    if (!path) return NULL;
    
    char* normalized = (char*)malloc(MAX_PATH_LENGTH);
    if (!normalized) return NULL;
    
    // Simple normalization - remove double slashes and trailing slashes
    strcpy(normalized, path);
    
    // Remove trailing slash (except for root)
    size_t len = strlen(normalized);
    if (len > 1 && normalized[len - 1] == '/') {
        normalized[len - 1] = '\0';
    }
    
    return normalized;
}

char* filesys_get_parent_path(const char* path) {
    if (!path) return NULL;
    
    char* parent = (char*)malloc(MAX_PATH_LENGTH);
    if (!parent) return NULL;
    
    strcpy(parent, path);
    
    // Find last slash
    char* last_slash = strrchr(parent, '/');
    if (last_slash) {
        if (last_slash == parent) {
            // Root directory
            strcpy(parent, "/");
        } else {
            *last_slash = '\0';
        }
    } else {
        strcpy(parent, ".");
    }
    
    return parent;
}

char* filesys_get_basename(const char* path) {
    if (!path) return NULL;
    
    char* basename = (char*)malloc(MAX_FILENAME_LENGTH);
    if (!basename) return NULL;
    
    char* last_slash = strrchr(path, '/');
    if (last_slash) {
        strcpy(basename, last_slash + 1);
    } else {
        strcpy(basename, path);
    }
    
    return basename;
}

int filesys_is_absolute_path(const char* path) {
    return path && path[0] == '/';
}

// Cleanup filesystem
void filesys_cleanup(void) {
    if (!fs) return;
    
    // Free all files
    for (size_t i = 0; i < fs->directory_count; i++) {
        if (fs->directories[i]) {
            for (size_t j = 0; j < fs->directories[i]->file_count; j++) {
                if (fs->directories[i]->files[j]) {
                    if (fs->directories[i]->files[j]->content) {
                        free(fs->directories[i]->files[j]->content);
                    }
                    free(fs->directories[i]->files[j]);
                }
            }
            free(fs->directories[i]);
        }
    }
    
    free(fs);
    fs = NULL;
}

// Enhanced text editor integration
int make_file(const char* filename, const char* content) {
    return filesys_create_file(filename, content);
}

// Additional utility functions for shell integration
void filesys_pwd(void) {
    print("Current directory: ");
    print(filesys_get_current_path());
    print("\n");
}

void filesys_ls(void) {
    filesys_list_directory();
}

int filesys_cd(const char* path) {
    if (!path || strlen(path) == 0) {
        // Go to root if no path specified
        return filesys_change_directory("/");
    }
    
    int result = filesys_change_directory(path);
    if (result != 0) {
        print("cd: ");
        print(path);
        print(": No such directory\n");
    }
    return result;
}

int filesys_mkdir(const char* dirname) {
    int result = filesys_create_directory(dirname);
    if (result == -2) {
        print("mkdir: ");
        print(dirname);
        print(": Directory already exists\n");
    } else if (result != 0) {
        print("mkdir: Failed to create directory ");
        print(dirname);
        print("\n");
    }
    return result;
}

int filesys_rmdir(const char* dirname) {
    int result = filesys_delete_directory(dirname);
    if (result != 0) {
        print("rmdir: Failed to remove directory ");
        print(dirname);
        print("\n");
    }
    return result;
}

int filesys_rm(const char* filename) {
    int result = filesys_delete_file(filename);
    if (result != 0) {
        print("rm: ");
        print(filename);
        print(": No such file\n");
    }
    return result;
}

void filesys_tree(void) {
    print("\nFilesystem tree:\n");
    filesys_print_tree(0, 0);
    print("\n");
}

int filesys_cat(const char* filename) {
    char buffer[FILE_BUFFER_SIZE];
    int result = filesys_read_file(filename, buffer, sizeof(buffer));
    
    if (result < 0) {
        print("cat: ");
        print(filename);
        print(": No such file or file too large\n");
        return result;
    }
    
    print("\n--- Content of ");
    print(filename);
    print(" ---\n");
    print(buffer);
    print("\n--- End of file ---\n\n");
    
    return 0;
}

int filesys_cp(const char* src, const char* dest) {
    int result = filesys_copy_file(src, dest);
    if (result == -1) {
        print("cp: ");
        print(src);
        print(": No such file\n");
    } else if (result == -2) {
        print("cp: ");
        print(dest);
        print(": File already exists\n");
    }
    return result;
}

int filesys_mv(const char* src, const char* dest) {
    int result = filesys_move_file(src, dest);
    if (result != 0) {
        print("mv: Failed to move ");
        print(src);
        print(" to ");
        print(dest);
        print("\n");
    }
    return result;
}

void filesys_stat(const char* filename) {
    filesys_get_file_info(filename);
}

// Command wrapper functions for shell integration
void filesys_ls_command(int argc, char* argv[]) {
    filesys_ls();
}

void filesys_cd_command(int argc, char* argv[]) {
    if (argc < 2) {
        filesys_cd("/");  // Go to root if no argument
    } else {
        filesys_cd(argv[1]);
    }
}

void filesys_pwd_command(int argc, char* argv[]) {
    filesys_pwd();
}

void filesys_mkdir_command(int argc, char* argv[]) {
    if (argc < 2) {
        print("Usage: mkdir <directory_name>\n");
        return;
    }
    filesys_mkdir(argv[1]);
}

void filesys_rmdir_command(int argc, char* argv[]) {
    if (argc < 2) {
        print("Usage: rmdir <directory_name>\n");
        return;
    }
    filesys_rmdir(argv[1]);
}

void filesys_rm_command(int argc, char* argv[]) {
    if (argc < 2) {
        print("Usage: rm <filename>\n");
        return;
    }
    filesys_rm(argv[1]);
}

void filesys_cat_command(int argc, char* argv[]) {
    if (argc < 2) {
        print("Usage: cat <filename>\n");
        return;
    }
    filesys_cat(argv[1]);
}

void filesys_cp_command(int argc, char* argv[]) {
    if (argc < 3) {
        print("Usage: cp <source> <destination>\n");
        return;
    }
    filesys_cp(argv[1], argv[2]);
}

void filesys_mv_command(int argc, char* argv[]) {
    if (argc < 3) {
        print("Usage: mv <source> <destination>\n");
        return;
    }
    filesys_mv(argv[1], argv[2]);
}

void filesys_tree_command(int argc, char* argv[]) {
    filesys_tree();
}

void filesys_stat_command(int argc, char* argv[]) {
    if (argc < 2) {
        print("Usage: stat <filename>\n");
        return;
    }
    filesys_stat(argv[1]);
}
