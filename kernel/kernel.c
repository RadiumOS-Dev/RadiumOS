#include "../terminal/terminal.h"
#include "../keyboard/keyboard.h"
#include "../scheduler/task.h"
#include "../timers/timer.h"
#include "../timers/date.h"
#include "../errors/error.h"
#include "../memory/memory.h"
#include "../sound/sound.h"
#include "../io/io.h"
#include "../mpop/mpop.h"
#include "../Avfs/Avfs.h"
#include "../driver/driver.h"
#include "../user/login.h"
#include "../user/sudo.h"
#include "../vga/tilingmanager.h"
#include "../cpu/cpu.h"
#include "../utility/utility.h"
#include "../script/script.h"
#include "../utility/random.h"



#include "../rtl8139/rtl8139.h"
#include "../commands/netcmd.h"
#include "../network/tcp.h"
#include "../network/telnet.h"

#include "../commands/mempop.h"
#include "../commands/brainz.h"
#include "../commands/clear.h"
#include "../commands/echo.h"
#include "../commands/exit.h"
#include "../commands/reboot.h"
#include "../commands/help.h"
#include "../commands/text.h"
#include "../commands/meow.h"
#include "../commands/rm.h"
#include "../commands/cat.h"
#include "../commands/settings.h"
#include "../commands/ls.h"
#include "../commands/tui.h"
#include "../commands/radifetch.h"
#include "../commands/cowsay.h"
void test_action(void) {
    // Simple visual feedback
    printr("Action executed!\n");
}
// Interactive Shell Page - Updates on Every Keystroke
// Add this as another page option in your welcome screen
// Complete File Manager - Single Function Implementation
// Add this to your kernel or commands file
    void refresh_files() {
        int file_count = 0;

    }
// Complete File Manager - Single Function Implementation
// Add this to your kernel or commands file
// COMPLETE WORKING FILE MANAGER - NO AVFS INTERNALS
// Replace your entire file_manager() function with this

void file_manager() {
    // File manager state
    int selected_index = 0;
    int scroll_offset = 0;
    int view_mode = 0; // 0=list, 1=details, 2=preview
    char search_filter[64] = {0};
    int filter_len = 0;
    bool show_help = false;
    bool need_refresh = true;
    
    // File list cache
    typedef struct {
        char name[AVFS_FILENAME_MAX];
        uint32_t size;
        bool matches_filter;
    } file_info_t;
    
    file_info_t files[AVFS_MAX_FILES];
    int file_count = 0;
    
    // Main file manager window
    vga_window_t fm_win = vga_create_centered_window(76, 24, VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    vga_win_set_title(&fm_win, "File Manager");
    
    int frame = 0;
    bool running = true;
    
    while (running) {
        // Refresh file list when needed
        if (need_refresh) {
            file_count = 0;
            
            // List of common/expected files to check
            const char* common_files[] = {
                "autoexec.rsh", "username.cfg", "password.cfg", "vop",
                "encrypt.cfg", "done.log", "warn.log", "error.log",
                "rtl8139.drv", "rtc.drv", "gdt.drv", "interrupts.drv",
                "pit.drv", "scheduler.drv", "test.txt", "readme.txt",
                "config.sys", "boot.ini", "system.dat", "network.cfg",
                NULL
            };
            
            // Check which files exist using avfs_get_filesize
            for (int i = 0; common_files[i] != NULL && file_count < AVFS_MAX_FILES; i++) {
                int size = avfs_get_filesize(common_files[i]);
                if (size >= 0) {  // File exists
                    strcpy(files[file_count].name, common_files[i]);
                    files[file_count].size = size;
                    
                    // Check if matches search filter
                    if (filter_len == 0) {
                        files[file_count].matches_filter = true;
                    } else {
                        files[file_count].matches_filter = 
                            (strstr(files[file_count].name, search_filter) != NULL);
                    }
                    
                    file_count++;
                }
            }
            
            need_refresh = false;
        }
        
        vga_win_clear(&fm_win);
        
        // === HEADER ===
        vga_win_puts_colored(&fm_win, 2, 2,
            "RadiumOS File Manager - Press H for Help",
            vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLUE));
        
        vga_win_draw_line_h(&fm_win, 2, 3, 72, 0xC4);
        
        // === SEARCH/FILTER BAR ===
        vga_win_puts_colored(&fm_win, 2, 4,
            "Search: ",
            vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLUE));
        
        vga_win_puts(&fm_win, 10, 4, search_filter);
        
        // Blinking cursor in search
        if (frame % 10 < 5) {
            vga_win_putc_colored(&fm_win, 10 + filter_len, 4, '_',
                vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
        }
        
        // File count
        char count_str[32];
        itoa(file_count, count_str, 10);
        vga_win_puts_colored(&fm_win, 60, 4, count_str,
            vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLUE));
        vga_win_puts_colored(&fm_win, 62, 4, " files",
            vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLUE));
        
        vga_win_draw_line_h(&fm_win, 2, 5, 72, 0xC4);
        
        // === FILE LIST ===
        int list_start_y = 6;
        int list_height = 12;
        int visible_count = 0;
        
        // Column headers
        if (view_mode == 1) { // Details view
            vga_win_puts_colored(&fm_win, 4, list_start_y,
                "Filename",
                vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLUE));
            vga_win_puts_colored(&fm_win, 40, list_start_y,
                "Size",
                vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLUE));
            vga_win_puts_colored(&fm_win, 55, list_start_y,
                "Blocks",
                vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLUE));
            list_start_y++;
            list_height--;
        }
        
        // Draw files
        for (int i = scroll_offset; i < file_count && visible_count < list_height; i++) {
            if (!files[i].matches_filter) continue;
            
            int y = list_start_y + visible_count;
            bool is_selected = (i == selected_index);
            
            // Selection highlight
            if (is_selected) {
                vga_win_fill_rect(&fm_win, 2, y, 72, 1, ' ',
                    vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_CYAN));
            }
            
            uint8_t text_color = is_selected ? 
                vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_CYAN) :
                vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
            
            // Selection indicator
            if (is_selected) {
                vga_win_puts_colored(&fm_win, 2, y, ">",
                    vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_CYAN));
            }
            
            // File name
            char display_name[40];
            strncpy(display_name, files[i].name, 35);
            display_name[35] = '\0';
            if (strlen(files[i].name) > 35) {
                strcat(display_name, "...");
            }
            
            vga_win_puts_colored(&fm_win, 4, y, display_name, text_color);
            
            // File size and details (if in details view)
            if (view_mode == 1) {
                char size_str[16];
                if (files[i].size < 1024) {
                    itoa(files[i].size, size_str, 10);
                    strcat(size_str, " B");
                } else if (files[i].size < 1024 * 1024) {
                    itoa(files[i].size / 1024, size_str, 10);
                    strcat(size_str, " KB");
                } else {
                    itoa(files[i].size / (1024 * 1024), size_str, 10);
                    strcat(size_str, " MB");
                }
                
                vga_win_puts_colored(&fm_win, 40, y, size_str, text_color);
                
                // Blocks
                int blocks = (files[i].size + 511) / 512;
                char block_str[16];
                itoa(blocks, block_str, 10);
                vga_win_puts_colored(&fm_win, 55, y, block_str, text_color);
            }
            
            visible_count++;
        }
        
        // === FILE PREVIEW (if selected and in preview mode) ===
        if (view_mode == 2 && file_count > 0 && selected_index < file_count && files[selected_index].matches_filter) {
            vga_win_draw_line_h(&fm_win, 2, 18, 72, 0xC4);
            vga_win_puts_colored(&fm_win, 2, 19,
                "Preview:",
                vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLUE));
            
            char preview_buf[256];
            int read_size = files[selected_index].size > 255 ? 255 : files[selected_index].size;
            
            if (read_size > 0 && avfs_read_file(files[selected_index].name, preview_buf, read_size, 0) == 0) {
                preview_buf[read_size] = '\0';
                
                // Show first line only
                int col = 2;
                for (int i = 0; i < read_size && col < 72 && preview_buf[i] != '\n'; i++) {
                    char ch = preview_buf[i];
                    if (ch < 32 || ch > 126) ch = '.'; // Non-printable
                    vga_win_putc_colored(&fm_win, col++, 20, ch,
                        vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLUE));
                }
            }
        }
        
        // === FOOTER / STATUS BAR ===
        vga_win_draw_line_h(&fm_win, 2, 21, 72, 0xC4);
        
        // Show current operation hints
        if (show_help) {
            vga_win_puts_colored(&fm_win, 2, 22,
                "UP/DN:Nav ENTER:View C:Create R:Rename D:Delete V:Mode ESC:Exit",
                vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLUE));
        } else {
            // Status info
            if (file_count > 0 && selected_index < file_count) {
                char status[70];
                strcpy(status, "File: ");
                strncat(status, files[selected_index].name, 50);
                vga_win_puts_colored(&fm_win, 2, 22, status,
                    vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
            } else if (file_count == 0) {
                vga_win_puts_colored(&fm_win, 2, 22, "No files found",
                    vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLUE));
            }
            
            // View mode indicator
            const char* mode_str = view_mode == 0 ? "List" : 
                                  view_mode == 1 ? "Details" : "Preview";
            vga_win_puts_colored(&fm_win, 60, 22, "Mode:",
                vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLUE));
            vga_win_puts_colored(&fm_win, 66, 22, mode_str,
                vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLUE));
        }
        
        vga_win_refresh(&fm_win);
        
        // === KEYBOARD INPUT ===
        int key = keyboard_key();
        
        if (key == 0x48) { // UP
            if (selected_index > 0) {
                // Find previous visible file
                do {
                    selected_index--;
                } while (selected_index > 0 && !files[selected_index].matches_filter);
                
                if (selected_index < scroll_offset) {
                    scroll_offset = selected_index;
                }
            }
        }
        else if (key == 0x50) { // DOWN
            if (selected_index < file_count - 1) {
                // Find next visible file
                do {
                    selected_index++;
                } while (selected_index < file_count - 1 && !files[selected_index].matches_filter);
                
                if (selected_index >= scroll_offset + list_height) {
                    scroll_offset = selected_index - list_height + 1;
                }
            }
        }
        else if (key == 0x1C && file_count > 0 && files[selected_index].matches_filter) { // ENTER - View file
            vga_destroy_window(&fm_win);
            terminal_clear();
            
            char* argv[] = {"cat", files[selected_index].name};
            cat_command(2, argv);
            
            printr("\nPress any key to return to file manager...\n");
            keyboard_wait_for_key(0);
            
            fm_win = vga_create_centered_window(76, 24, VGA_COLOR_WHITE, VGA_COLOR_BLUE);
            vga_win_set_title(&fm_win, "File Manager");
        }
        else if (key == 0x2E) { // C - Create file
            vga_destroy_window(&fm_win);
            terminal_clear();
            
            printr("Create New File\n");
            printr("===============\n\n");
            printr("Filename: ");
            
            char filename[64];
            keyboard_input(filename);
            
            if (strlen(filename) > 0) {
                printr("\nFile size (bytes): ");
                char size_str[16];
                keyboard_input(size_str);
                int size = atoi(size_str);
                
                if (size > 0 && size < 100000) {
                    if (avfs_create_file(filename, size) == 0) {
                        printr("\nFile created successfully!\n");
                        need_refresh = true;
                    } else {
                        printr("\nError: Failed to create file!\n");
                    }
                } else {
                    printr("\nError: Invalid size (1-99999)!\n");
                }
            }
            
            printr("\nPress any key to continue...\n");
            keyboard_wait_for_key(0);
            
            fm_win = vga_create_centered_window(76, 24, VGA_COLOR_WHITE, VGA_COLOR_BLUE);
            vga_win_set_title(&fm_win, "File Manager");
        }
        else if (key == 0x20 && file_count > 0 && files[selected_index].matches_filter) { // D - Delete file
            vga_destroy_window(&fm_win);
            terminal_clear();
            
            printr("Delete File\n");
            printr("===========\n\n");
            printr("Delete '%s'? (Y/N): ", files[selected_index].name);
            
            int confirm = keyboard_wait_for_key(0);
            
            if (confirm == 0x15) { // Y key
                if (avfs_remove_file(files[selected_index].name) == 0) {
                    printr("\n\nFile deleted successfully!\n");
                    need_refresh = true;
                    if (selected_index >= file_count - 1 && selected_index > 0) {
                        selected_index--;
                    }
                } else {
                    printr("\n\nError: Failed to delete file!\n");
                }
            } else {
                printr("\n\nCancelled.\n");
            }
            
            printr("\nPress any key to continue...\n");
            keyboard_wait_for_key(0);
            
            fm_win = vga_create_centered_window(76, 24, VGA_COLOR_WHITE, VGA_COLOR_BLUE);
            vga_win_set_title(&fm_win, "File Manager");
        }
        else if (key == 0x13 && file_count > 0 && files[selected_index].matches_filter) { // R - Rename file
            vga_destroy_window(&fm_win);
            terminal_clear();
            
            printr("Rename File\n");
            printr("===========\n\n");
            printr("Old name: %s\n", files[selected_index].name);
            printr("New name: ");
            
            char new_name[AVFS_FILENAME_MAX];
            keyboard_input(new_name);
            
            if (strlen(new_name) > 0) {
                // Read old file content
                int old_size = files[selected_index].size;
                char* temp_buf = (char*)malloc(old_size);
                
                if (temp_buf && avfs_read_file(files[selected_index].name, temp_buf, old_size, 0) == 0) {
                    // Create new file
                    if (avfs_create_file(new_name, old_size) == 0) {
                        // Write content to new file
                        if (avfs_write_file(new_name, temp_buf, old_size, 0) == 0) {
                            // Delete old file
                            if (avfs_remove_file(files[selected_index].name) == 0) {
                                printr("\nFile renamed successfully!\n");
                                need_refresh = true;
                            } else {
                                printr("\nError: Failed to remove old file!\n");
                            }
                        } else {
                            printr("\nError: Failed to write to new file!\n");
                        }
                    } else {
                        printr("\nError: Failed to create new file!\n");
                    }
                    
                    if (temp_buf) free(temp_buf);
                } else {
                    printr("\nError: Failed to read old file!\n");
                    if (temp_buf) free(temp_buf);
                }
            }
            
            printr("\nPress any key to continue...\n");
            keyboard_wait_for_key(0);
            
            fm_win = vga_create_centered_window(76, 24, VGA_COLOR_WHITE, VGA_COLOR_BLUE);
            vga_win_set_title(&fm_win, "File Manager");
        }
        else if (key == 0x2F) { // V - Change view mode
            view_mode = (view_mode + 1) % 3;
        }
        else if (key == 0x23) { // H - Toggle help
            show_help = !show_help;
        }
        else if (key == 0x0E) { // BACKSPACE - Edit search
            if (filter_len > 0) {
                search_filter[--filter_len] = '\0';
                need_refresh = true;
                selected_index = 0;
                scroll_offset = 0;
            }
        }
        else if (key >= 0x10 && key <= 0x32) { // Letters for search
            if (filter_len < 63) {
                const char key_map[] = "qwertyuiopasdfghjklzxcvbnm";
                int idx = -1;
                
                if (key >= 0x10 && key <= 0x19) idx = key - 0x10;       // q-p
                else if (key >= 0x1E && key <= 0x26) idx = key - 0x1E + 10; // a-l
                else if (key >= 0x2C && key <= 0x32) idx = key - 0x2C + 19; // z-m
                
                if (idx >= 0 && idx < 26) {
                    search_filter[filter_len++] = key_map[idx];
                    search_filter[filter_len] = '\0';
                    need_refresh = true;
                    selected_index = 0;
                    scroll_offset = 0;
                }
            }
        }
        else if (key == 0x01) { // ESC - Exit
            running = false;
        }
        
        frame++;
        sleep_ms(50); // ~20 FPS
    }
    
    vga_destroy_window(&fm_win);
    terminal_clear();
}
void show_interactive_shell_page(vga_window_t* parent_win) {
    // Destroy parent window
    vga_destroy_window(parent_win);
    
    // Create interactive shell window
    vga_window_t shell_win = vga_create_centered_window(
        76, 24,
        VGA_COLOR_WHITE,
        VGA_COLOR_BLACK
    );
    
    vga_win_set_title(&shell_win, "Interactive Shell Preview");
    
    // Command buffer
    char command[128] = {0};
    int cmd_pos = 0;
    
    // Command suggestions
    const char* commands[] = {
        "help", "ls", "cat", "rm", "echo", "clear", "date", "time",
        "ping", "ifconfig", "arp", "curl", "network", "radifetch",
        "mempop", "mpop", "brainfuck", "script", "settings", "reboot",
        "cowsay", "brainz", "tui", "onan", "exit"
    };
    int num_commands = 25;
    
    bool shell_active = true;
    int frame = 0;
    
    while (shell_active) {
        // Clear window
        vga_win_clear(&shell_win);
        
        // Draw header
        vga_win_puts_colored(&shell_win, 2, 2,
            "RadiumOS Interactive Shell - Type to see suggestions",
            vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
        
        vga_win_draw_line_h(&shell_win, 2, 3, 72, 0xC4);
        
        // Show prompt with current command
        vga_win_puts_colored(&shell_win, 2, 5,
            "root@radiumos:~$ ",
            vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
        
        vga_win_puts_colored(&shell_win, 19, 5,
            command,
            vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        
        // Blinking cursor
        if (frame % 10 < 5) {
            vga_win_putc_colored(&shell_win, 19 + cmd_pos, 5, '_',
                vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        }
        
        // Find matching commands
        vga_win_puts_colored(&shell_win, 2, 7,
            "Suggestions:",
            vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
        
        int suggestion_count = 0;
        int row = 8;
        
        if (cmd_pos > 0) {
            // Show matching commands
            for (int i = 0; i < num_commands && suggestion_count < 8; i++) {
                if (strncmp(commands[i], command, cmd_pos) == 0) {
                    // Highlight matching part
                    char display[64];
                    strcpy(display, "  ");
                    strcat(display, commands[i]);
                    
                    vga_win_puts_colored(&shell_win, 2, row,
                        display,
                        vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
                    
                    // Highlight the matching prefix
                    for (int j = 0; j < cmd_pos; j++) {
                        vga_win_putc_colored(&shell_win, 4 + j, row,
                            commands[i][j],
                            vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
                    }
                    
                    row++;
                    suggestion_count++;
                }
            }
            
            if (suggestion_count == 0) {
                vga_win_puts_colored(&shell_win, 2, 8,
                    "  No matching commands",
                    vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK));
            }
        } else {
            vga_win_puts_colored(&shell_win, 2, 8,
                "  Type to see available commands...",
                vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK));
        }
        
        // Command info section
        vga_win_puts_colored(&shell_win, 2, 17,
            "Quick Commands:",
            vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
        
        vga_win_puts_colored(&shell_win, 2, 18,
            "  TAB - Auto-complete    ENTER - Execute command",
            vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK));
        
        vga_win_puts_colored(&shell_win, 2, 19,
            "  ESC - Cancel           BACKSPACE - Delete char",
            vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK));
        
        // Footer
        vga_win_draw_line_h(&shell_win, 2, 21, 72, 0xC4);
        vga_win_puts_colored(&shell_win, 2, 22,
            "Press ESC to return to welcome screen",
            vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
        
        vga_win_refresh(&shell_win);
        
        // Handle keyboard input
        int key = keyboard_key();
        
        if (key >= 0x02 && key <= 0x0B) { // Number keys 1-0
            if (cmd_pos < 127) {
                const char num_chars[] = "1234567890";
                command[cmd_pos++] = num_chars[key - 0x02];
                command[cmd_pos] = '\0';
            }
        }
        else if (key >= 0x10 && key <= 0x32) { // Letter keys
            if (cmd_pos < 127) {
                // Simple scancode to char mapping
                const char key_map[] = {
                    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', // 0x10-0x19
                    '[', ']', '\n', 0, 'a', 's', 'd', 'f', 'g', 'h',  // 0x1A-0x23
                    'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', // 0x24-0x2D
                    'c', 'v', 'b', 'n', 'm'                           // 0x2E-0x32
                };
                
                if (key - 0x10 < 35 && key_map[key - 0x10] != 0 && key_map[key - 0x10] != '\n') {
                    command[cmd_pos++] = key_map[key - 0x10];
                    command[cmd_pos] = '\0';
                }
            }
        }
        else if (key == 0x39) { // SPACE
            if (cmd_pos < 127) {
                command[cmd_pos++] = ' ';
                command[cmd_pos] = '\0';
            }
        }
        else if (key == 0x0E) { // BACKSPACE
            if (cmd_pos > 0) {
                command[--cmd_pos] = '\0';
            }
        }
        else if (key == 0x0F) { // TAB - Auto-complete
            if (cmd_pos > 0) {
                // Find first matching command
                for (int i = 0; i < num_commands; i++) {
                    if (strncmp(commands[i], command, cmd_pos) == 0) {
                        strcpy(command, commands[i]);
                        cmd_pos = strlen(command);
                        break;
                    }
                }
            }
        }
        else if (key == 0x1C) { // ENTER - Execute
            if (cmd_pos > 0) {
                vga_destroy_window(&shell_win);
                terminal_clear();
                
                // Parse and execute command
                char* argv[16];
                int argc = 0;
                
                char* token = strtok(command, " ");
                while (token != NULL && argc < 16) {
                    argv[argc++] = token;
                    token = strtok(NULL, " ");
                }
                
                if (argc > 0) {
                    // Try to execute the command
                    if (strcmp(argv[0], "help") == 0) {
                        help_command(argc, argv);
                    } else if (strcmp(argv[0], "ls") == 0) {
                        ls_command(argc, argv);
                    } else if (strcmp(argv[0], "cat") == 0) {
                        cat_command(argc, argv);
                    } else if (strcmp(argv[0], "echo") == 0) {
                        echo_command(argc, argv);
                    } else if (strcmp(argv[0], "clear") == 0) {
                        clear(argc, argv);
                    } else if (strcmp(argv[0], "date") == 0) {
                        date_command(argc, argv);
                    } else if (strcmp(argv[0], "time") == 0) {
                        time_command(argc, argv);
                    } else if (strcmp(argv[0], "radifetch") == 0) {
                        radifetch_command(argc, argv);
                    } else if (strcmp(argv[0], "ifconfig") == 0) {
                        ifconfig_command(argc, argv);
                    } else if (strcmp(argv[0], "ping") == 0) {
                        ping_command(argc, argv);
                    } else if (strcmp(argv[0], "settings") == 0) {
                        settings_command(argc, argv);
                    } else if (strcmp(argv[0], "cowsay") == 0) {
                        cowsay_command(argc, argv);
                    } else if (strcmp(argv[0], "reboot") == 0) {
                        reboot_command(argc, argv);
                    } else {
                        terminal_setcolor(VGA_COLOR_LIGHT_RED);
                        printr("Unknown command: %s\n", argv[0]);
                        terminal_setcolor(VGA_COLOR_WHITE);
                        printr("Type 'help' for available commands\n");
                    }
                }
                
                printr("\nPress any key to return to interactive shell...\n");
                keyboard_wait_for_key(0);
                
                // Recreate shell window
                shell_win = vga_create_centered_window(76, 24, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
                vga_win_set_title(&shell_win, "Interactive Shell Preview");
                
                // Reset command
                command[0] = '\0';
                cmd_pos = 0;
                frame = 0;
            }
        }
        else if (key == 0x01) { // ESC - Exit
            shell_active = false;
        }
        
        frame++;
        sleep_ms(50); // 20 FPS
    }
    
    vga_destroy_window(&shell_win);
}

// ============================================================================
// HOW TO ADD THIS TO YOUR WELCOME SCREEN
// ============================================================================

// In the main welcome screen while loop, add this case:
/*
        else if (key == 0x1F) { // S - Interactive Shell
            show_interactive_shell_page(&main_win);
            
            // Recreate main window after returning
            main_win = vga_create_centered_window(70, 24, VGA_COLOR_WHITE, VGA_COLOR_CYAN);
            vga_win_set_title(&main_win, "Welcome to RadiumOS");
            // ... redraw main screen ...
            frame = 0;
        }
*/

// Update the navigation instructions to include:
// "[S] Shell Preview"

// ============================================================================
// ALTERNATIVE: FULL-FEATURED LIVE SHELL WITH HISTORY
// ============================================================================

void show_live_shell_with_history(vga_window_t* parent_win) {
    vga_destroy_window(parent_win);
    
    vga_window_t shell_win = vga_create_centered_window(76, 24, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_win_set_title(&shell_win, "Live Interactive Shell");
    
    // Command history
    char history[10][128];
    int history_count = 0;
    int history_index = -1;
    
    char command[128] = {0};
    int cmd_pos = 0;
    
    // Output buffer (last 10 lines)
    char output[10][128];
    int output_count = 0;
    
    const char* commands[] = {
        "help", "ls", "cat", "rm", "echo", "clear", "date", "time",
        "ping", "ifconfig", "arp", "curl", "network", "radifetch",
        "mempop", "mpop", "brainfuck", "script", "settings", "reboot"
    };
    int num_commands = 20;
    
    bool shell_active = true;
    int frame = 0;
    
    while (shell_active) {
        vga_win_clear(&shell_win);
        
        // Title
        vga_win_puts_colored(&shell_win, 2, 2,
            "Live Shell - Real-time Command Preview",
            vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
        vga_win_draw_line_h(&shell_win, 2, 3, 72, 0xC4);
        
        // Output section (previous commands)
        int out_row = 4;
        for (int i = 0; i < output_count && i < 5; i++) {
            vga_win_puts_colored(&shell_win, 2, out_row++, output[i],
                vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK));
        }
        
        // Current prompt
        int prompt_row = 10;
        vga_win_puts_colored(&shell_win, 2, prompt_row,
            "root@radiumos:~$ ",
            vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
        
        vga_win_puts_colored(&shell_win, 19, prompt_row, command,
            vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        
        // Cursor
        if (frame % 10 < 5) {
            vga_win_putc_colored(&shell_win, 19 + cmd_pos, prompt_row, '_',
                vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        }
        
        // Live suggestions
        vga_win_puts_colored(&shell_win, 2, 12,
            "Live Suggestions:",
            vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
        
        int suggestion_row = 13;
        int suggestion_count = 0;
        
        if (cmd_pos > 0) {
            for (int i = 0; i < num_commands && suggestion_count < 5; i++) {
                if (strncmp(commands[i], command, cmd_pos) == 0) {
                    vga_win_puts_colored(&shell_win, 4, suggestion_row++,
                        commands[i],
                        vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
                    suggestion_count++;
                }
            }
        }
        
        // Help footer
        vga_win_draw_line_h(&shell_win, 2, 20, 72, 0xC4);
        vga_win_puts_colored(&shell_win, 2, 21,
            "UP/DOWN: History | TAB: Complete | ENTER: Execute | ESC: Exit",
            vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
        
        vga_win_refresh(&shell_win);
        
        // Handle input
        int key = keyboard_key();
        
        // Letter and number input
        if ((key >= 0x02 && key <= 0x0B) || (key >= 0x10 && key <= 0x32)) {
            if (cmd_pos < 127) {
                char ch = 0;
                
                // Numbers
                if (key >= 0x02 && key <= 0x0B) {
                    const char nums[] = "1234567890";
                    ch = nums[key - 0x02];
                }
                // Letters
                else if (key >= 0x10 && key <= 0x32) {
                    const char keys[] = "qwertyuiopasdfghjklzxcvbnm";
                    int idx = -1;
                    if (key >= 0x10 && key <= 0x19) idx = key - 0x10;
                    else if (key >= 0x1E && key <= 0x26) idx = key - 0x1E + 10;
                    else if (key >= 0x2C && key <= 0x32) idx = key - 0x2C + 19;
                    
                    if (idx >= 0 && idx < 26) {
                        ch = keys[idx];
                    }
                }
                
                if (ch != 0) {
                    command[cmd_pos++] = ch;
                    command[cmd_pos] = '\0';
                }
            }
        }
        else if (key == 0x39) { // SPACE
            if (cmd_pos < 127) {
                command[cmd_pos++] = ' ';
                command[cmd_pos] = '\0';
            }
        }
        else if (key == 0x0E) { // BACKSPACE
            if (cmd_pos > 0) {
                command[--cmd_pos] = '\0';
            }
        }
        else if (key == 0x0F) { // TAB
            if (cmd_pos > 0) {
                for (int i = 0; i < num_commands; i++) {
                    if (strncmp(commands[i], command, cmd_pos) == 0) {
                        strcpy(command, commands[i]);
                        cmd_pos = strlen(command);
                        break;
                    }
                }
            }
        }
        else if (key == 0x48) { // UP - Previous history
            if (history_count > 0) {
                if (history_index == -1) {
                    history_index = history_count - 1;
                } else if (history_index > 0) {
                    history_index--;
                }
                strcpy(command, history[history_index]);
                cmd_pos = strlen(command);
            }
        }
        else if (key == 0x50) { // DOWN - Next history
            if (history_index >= 0) {
                if (history_index < history_count - 1) {
                    history_index++;
                    strcpy(command, history[history_index]);
                } else {
                    history_index = -1;
                    command[0] = '\0';
                }
                cmd_pos = strlen(command);
            }
        }
        else if (key == 0x1C) { // ENTER
            if (cmd_pos > 0) {
                // Add to history
                if (history_count < 10) {
                    strcpy(history[history_count++], command);
                } else {
                    for (int i = 0; i < 9; i++) {
                        strcpy(history[i], history[i + 1]);
                    }
                    strcpy(history[9], command);
                }
                history_index = -1;
                
                // Add to output
                char out_line[128];
                strcpy(out_line, "$ ");
                strcat(out_line, command);
                
                if (output_count < 10) {
                    strcpy(output[output_count++], out_line);
                } else {
                    for (int i = 0; i < 9; i++) {
                        strcpy(output[i], output[i + 1]);
                    }
                    strcpy(output[9], out_line);
                }
                
                // Execute (simplified)
                strcpy(out_line, "> Executed: ");
                strcat(out_line, command);
                if (output_count < 10) {
                    strcpy(output[output_count++], out_line);
                }
                
                // Reset
                command[0] = '\0';
                cmd_pos = 0;
            }
        }
        else if (key == 0x01) { // ESC
            shell_active = false;
        }
        
        frame++;
        sleep_ms(50);
    }
    
    vga_destroy_window(&shell_win);
}


void network_poll_task() {
    while (1) {
        // Poll for packets every 10ms
        rtl8139_poll();
        sleep_ms(10);
    }
}
// Helper function to mark driver as running
void mark_driver_running(const char* driver_name) {
    char filename[64];
    strcpy(filename, driver_name);
    strcat(filename, ".drv");
    
    const char* status = "RUNNING";
    avfs_create_file(filename, strlen(status));
    avfs_write_file(filename, status, strlen(status), 0);
}

// Helper function for boot status messages
void boot_status(bool success, const char* component, const char* message) {
    if (success) {
        print("[ ");
        terminal_setcolor(VGA_COLOR_GREEN);
        print("OK");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        print(" ] ");
        print(message);
        print("\n");
        done(message, component);
        mark_driver_running(component);
    } else {
        print("[ ");
        terminal_setcolor(VGA_COLOR_RED);
        print("FAIL");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        print(" ] ");
        print(message);
        print(" failed\n");
        error(message, component);
        handle_error(message, "kernel");
    }
}

void settings() {
    bool vop = false; // off by default
    char userinput[COMMAND_BUFFER_SIZE];
    
    // Check initial state from file
    if (insideFile("vop", "on")) {
        vop = true;
    }
    
    while (1) {
        terminal_clear();
        // Display current status
        printr("\n========== Settings ==========\n");
        if (vop) {
            printr("vop: on\n");
        } else {
            printr("vop: off\n");
        }
        printr("==============================\n");
        printr("Commands: 'vop' to toggle, 'exit' to quit\n> ");
        
        // Get user input
        keyboard_input(userinput);
        
        // Toggle vop setting
        if (strcmp(userinput, "vop") == 0) {
            vop = !vop; // Toggle the state
            
            // Save to file
            avfs_remove_file("vop");
            if (vop) {
                const char* status = "on";
                avfs_create_file("vop", strlen(status));
                avfs_write_file("vop", status, strlen(status), 0);
                info("VOP enabled", "vop");
                printr("VOP enabled!\n");
            } else {
                const char* status = "off";
                avfs_create_file("vop", strlen(status));
                avfs_write_file("vop", status, strlen(status), 0);
                info("VOP disabled", "vop");
                printr("VOP disabled!\n");
            }
        }
        // Exit command
        else if (strcmp(userinput, "exit") == 0) {
            printr("Exiting settings...\n");
            break;
        }
        else {
            printr("Unknown command. Try 'vop' or 'exit'\n");
        }
    }
}

void config() {
    // Perform checks once during configuration, without infinite looping
    if (MAX_ALLOCATIONS > 1001) {
        error("Max Allocations Exceeding System Recommendations", "memory");
        meltdown_screen("Max Allocations Exceeding System Recommendations !", __FILE__, 31, 0x14, 1230, 190);
        return;  // Exit early on critical error
    } else if (allocation_count > 61) {
        error("Too many allocations for hobby OS", "memory");
        printr("\nWhat the fuck are you doing.\nLike you do not need THAT many allocations for this hobby os.\n");
        return;  // Exit early on warning
    } 
    if (insideFile("float.config", "on")) {
        info("Float config enabled", "float.config");
        print("Meow");
    }
    done("Configuration complete", "config");
}

void cmd_script(int argc, char* argv[]) {
    if (argc < 2) {
        print("Usage: script <filename>\n");
        print("Execute a RadiOS script file (.rsh)\n");
        return;
    }
    
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    print("Executing script: ");
    print(argv[1]);
    print("\n\n");
    terminal_setcolor(VGA_COLOR_WHITE);
    
    int result = script_execute_file(argv[1]);
    
    if (result != 0) {
        terminal_setcolor(VGA_COLOR_LIGHT_RED);
        print("\nScript execution failed or file not found\n");
        terminal_setcolor(VGA_COLOR_WHITE);
    } else {
        terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
        print("\nScript completed successfully\n");
        terminal_setcolor(VGA_COLOR_WHITE);
    }
}

void rash(int argc, char* argv[]) {
    if (avfs_file_exists("autoexec.rsh")) {
        script_execute_file("autoexec.rsh");
    }
}



// Fixed curl implementation - kernel_main.c section

#define HTTP_PORT 80
#define MAX_RESPONSE_SIZE 8192
#define MAX_URL_LENGTH 2048
#define MAX_HOSTNAME_LENGTH 512

// HTTP request structure
typedef struct {
    char method[8];
    char hostname[MAX_HOSTNAME_LENGTH];
    char path[MAX_URL_LENGTH];
    uint16_t port;
    char* data;
    uint16_t data_length;
    bool verbose;
    uint32_t timeout_ms;
} http_request_t;

static uint16_t local_port = 50000;

// Parse URL - FIXED version without recursion
bool parse_url(const char* url, http_request_t* req) {
    const char* ptr = url;
    
    // Default port
    req->port = HTTP_PORT;
    
    // Check and skip protocol if present
    if (strncmp(ptr, "https://", 8) == 0) {
        print("Error: HTTPS not supported\n");
        return false;
    }
    
    if (strncmp(ptr, "http://", 7) == 0) {
        ptr += 7;  // Skip "http://"
    }
    // If no protocol specified, just continue (assume HTTP)
    
    // Extract hostname
    int host_idx = 0;
    while (*ptr && *ptr != '/' && *ptr != ':' && host_idx < MAX_HOSTNAME_LENGTH - 1) {
        req->hostname[host_idx++] = *ptr++;
    }
    req->hostname[host_idx] = '\0';
    
    if (host_idx == 0) {
        print("Error: Invalid hostname\n");
        print("Usage: curl http://example.com/path\n");
        return false;
    }
    
    // Check for custom port
    if (*ptr == ':') {
        ptr++;
        req->port = 0;
        while (*ptr >= '0' && *ptr <= '9') {
            req->port = req->port * 10 + (*ptr - '0');
            ptr++;
        }
        
        if (req->port == 0 || req->port > 65535) {
            print("Error: Invalid port number\n");
            return false;
        }
    }
    
    // Extract path
    if (*ptr == '/') {
        size_t path_len = strlen(ptr);
        if (path_len >= MAX_URL_LENGTH) {
            print("Error: Path too long (max ");
            char buf[16];
            itoa(MAX_URL_LENGTH - 1, buf, 10);
            print(buf);
            print(" characters)\n");
            return false;
        }
        strncpy(req->path, ptr, MAX_URL_LENGTH - 1);
        req->path[MAX_URL_LENGTH - 1] = '\0';
    } else {
        strcpy(req->path, "/");
    }
    
    return true;
}

// Resolve hostname to IP
uint32_t resolve_hostname(const char* hostname) {
    // Common hosts
    if (strcmp(hostname, "ntfy.sh") == 0) {
        return ip_parse("116.202.210.223");
    }
    
    if (strcmp(hostname, "example.com") == 0) {
        return ip_parse("93.184.216.34");
    }
    
    if (strcmp(hostname, "httpbin.org") == 0) {
        return ip_parse("54.166.163.67");
    }
    
    if (strcmp(hostname, "google.com") == 0 || strcmp(hostname, "www.google.com") == 0) {
        return ip_parse("142.250.190.78");
    }
    
    if (strcmp(hostname, "github.com") == 0) {
        return ip_parse("140.82.121.4");
    }
    
    // Try parsing as IP directly
    if (hostname[0] >= '0' && hostname[0] <= '9') {
        uint32_t ip = ip_parse(hostname);
        if (ip != 0) {
            return ip;
        }
    }
    
    // Check for invalid hostnames
    if (strcmp(hostname, "http") == 0 || strcmp(hostname, "https") == 0) {
        print("Error: URL must include hostname\n");
        print("Example: curl http://example.com\n");
        return 0;
    }
    
    print("Error: Cannot resolve hostname '");
    print(hostname);
    print("'\n");
    print("Supported hosts:\n");
    print("  - ntfy.sh\n");
    print("  - example.com\n");
    print("  - httpbin.org\n");
    print("  - google.com\n");
    print("  - github.com\n");
    print("  - Or use direct IP address\n");
    return 0;
}

// Build HTTP request string
int build_http_request(http_request_t* req, char* buffer, size_t buffer_size) {
    int len = 0;
    
    // Request line: METHOD /path HTTP/1.1
    len += snprintf(buffer + len, buffer_size - len, 
                    "%s %s HTTP/1.1\r\n", req->method, req->path);
    
    // Host header
    if (req->port == 80) {
        len += snprintf(buffer + len, buffer_size - len, 
                        "Host: %s\r\n", req->hostname);
    } else {
        len += snprintf(buffer + len, buffer_size - len, 
                        "Host: %s:%d\r\n", req->hostname, req->port);
    }
    
    // User-Agent
    len += snprintf(buffer + len, buffer_size - len, 
                    "User-Agent: RadiOS-curl/1.0\r\n");
    
    // Accept
    len += snprintf(buffer + len, buffer_size - len, 
                    "Accept: */*\r\n");
    
    // Connection
    len += snprintf(buffer + len, buffer_size - len, 
                    "Connection: close\r\n");
    
    // POST data headers
    if (req->data && req->data_length > 0) {
        len += snprintf(buffer + len, buffer_size - len, 
                        "Content-Type: text/plain; charset=utf-8\r\n");
        len += snprintf(buffer + len, buffer_size - len, 
                        "Content-Length: %d\r\n", req->data_length);
    }
    
    // End headers
    len += snprintf(buffer + len, buffer_size - len, "\r\n");
    
    // POST data body
    if (req->data && req->data_length > 0) {
        if (len + req->data_length < buffer_size) {
            memcpy(buffer + len, req->data, req->data_length);
            len += req->data_length;
        }
    }
    
    return len;
}

// Parse and display HTTP response
void display_http_response(uint8_t* data, uint16_t length, bool verbose) {
    // Find end of headers
    int body_start = 0;
    int status_code = 0;
    
    for (int i = 0; i < length - 3; i++) {
        if (data[i] == '\r' && data[i+1] == '\n' &&
            data[i+2] == '\r' && data[i+3] == '\n') {
            body_start = i + 4;
            break;
        }
    }
    
    // Parse status code
    if (length > 12 && data[0] == 'H' && data[1] == 'T' && data[2] == 'T' && data[3] == 'P') {
        char status_str[4] = {0};
        status_str[0] = data[9];
        status_str[1] = data[10];
        status_str[2] = data[11];
        status_code = atoi(status_str);
    }
    
    // Display status
    if (status_code > 0) {
        terminal_setcolor(VGA_COLOR_LIGHT_CYAN);
        printr("HTTP/%c.%c %d ", data[5], data[7], status_code);
        
        // Color code status
        if (status_code >= 200 && status_code < 300) {
            terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
        } else if (status_code >= 300 && status_code < 400) {
            terminal_setcolor(VGA_COLOR_LIGHT_BROWN);
        } else if (status_code >= 400) {
            terminal_setcolor(VGA_COLOR_LIGHT_RED);
        }
        
        // Find status text
        int start = 13;
        while (start < length && data[start] != '\r') {
            terminal_putchar(data[start++]);
        }
        terminal_setcolor(VGA_COLOR_WHITE);
        print("\n");
    }
    
    // Display headers if verbose
    if (verbose && body_start > 0) {
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        print("\n=== Response Headers ===\n");
        for (int i = 0; i < body_start - 2 && i < length; i++) {
            terminal_putchar(data[i]);
        }
        print("========================\n\n");
        terminal_setcolor(VGA_COLOR_WHITE);
    }
    
    // Display body
    if (body_start > 0 && body_start < length) {
        if (verbose) {
            terminal_setcolor(VGA_COLOR_LIGHT_GREY);
            print("=== Response Body ===\n");
            terminal_setcolor(VGA_COLOR_WHITE);
        }
        
        for (int i = body_start; i < length; i++) {
            terminal_putchar(data[i]);
        }
        
        if (verbose) {
            terminal_setcolor(VGA_COLOR_LIGHT_GREY);
            print("\n=====================\n");
            terminal_setcolor(VGA_COLOR_WHITE);
        }
    }
    
    print("\n");
}

// Make HTTP request
int http_request_tcp(http_request_t* req) {
    char request_buffer[2048];
    uint8_t response_buffer[MAX_RESPONSE_SIZE];
    
    // Step 1: Resolve hostname
    terminal_setcolor(VGA_COLOR_LIGHT_CYAN);
    print("* Resolving ");
    print(req->hostname);
    print("...\n");
    terminal_setcolor(VGA_COLOR_WHITE);
    
    uint32_t dest_ip = resolve_hostname(req->hostname);
    if (dest_ip == 0) {
        return -1;
    }
    
    char ip_str[32];
    ip_to_string(dest_ip, ip_str);
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    print("* Resolved to ");
    print(ip_str);
    print("\n");
    terminal_setcolor(VGA_COLOR_WHITE);
    
    // Step 2: TCP connect
    terminal_setcolor(VGA_COLOR_LIGHT_CYAN);
    printr("* Connecting to %s:%d...\n", req->hostname, req->port);
    terminal_setcolor(VGA_COLOR_WHITE);
    
    tcp_connection_t* conn = tcp_connect(dest_ip, req->port, local_port++);
    if (!conn) {
        terminal_setcolor(VGA_COLOR_LIGHT_RED);
        print("* Connection failed\n");
        terminal_setcolor(VGA_COLOR_WHITE);
        return -1;
    }
    
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    print("* Connected\n");
    terminal_setcolor(VGA_COLOR_WHITE);
    
    // Step 3: Build HTTP request
    int request_len = build_http_request(req, request_buffer, sizeof(request_buffer));
    if (request_len < 0) {
        tcp_close(conn);
        return -1;
    }
    
    if (req->verbose) {
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        print("\n> Sending request:\n");
        print("--------------------\n");
        for (int i = 0; i < request_len; i++) {
            terminal_putchar(request_buffer[i]);
        }
        print("--------------------\n\n");
        terminal_setcolor(VGA_COLOR_WHITE);
    }
    
    // Step 4: Send request
    terminal_setcolor(VGA_COLOR_LIGHT_CYAN);
    printr("* Sending %s request (%d bytes)...\n", req->method, request_len);
    terminal_setcolor(VGA_COLOR_WHITE);
    
    int sent = tcp_send_data(conn, (uint8_t*)request_buffer, request_len);
    if (sent < 0) {
        terminal_setcolor(VGA_COLOR_LIGHT_RED);
        print("* Send failed\n");
        terminal_setcolor(VGA_COLOR_WHITE);
        tcp_close(conn);
        return -1;
    }
    
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    printr("* Sent %d bytes\n", sent);
    terminal_setcolor(VGA_COLOR_WHITE);
    
    // Step 5: Receive response
    terminal_setcolor(VGA_COLOR_LIGHT_CYAN);
    print("* Waiting for response...\n");
    terminal_setcolor(VGA_COLOR_WHITE);
    
    int received = tcp_receive_data(conn, response_buffer, MAX_RESPONSE_SIZE, req->timeout_ms);
    if (received <= 0) {
        terminal_setcolor(VGA_COLOR_LIGHT_RED);
        print("* No response received\n");
        terminal_setcolor(VGA_COLOR_WHITE);
        tcp_close(conn);
        return -1;
    }
    
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    printr("* Received %d bytes\n", received);
    terminal_setcolor(VGA_COLOR_WHITE);
    
    // Step 6: Display response
    print("\n");
    display_http_response(response_buffer, received, req->verbose);
    
    // Step 7: Close connection
    tcp_close(conn);
    terminal_setcolor(VGA_COLOR_LIGHT_CYAN);
    print("* Connection closed\n");
    terminal_setcolor(VGA_COLOR_WHITE);
    
    return 0;
}

// Main curl command
void cmd_curl(int argc, char* argv[]) {
    if (argc < 2) {
        terminal_setcolor(VGA_COLOR_LIGHT_CYAN);
        print("curl - HTTP client for RadiOS\n\n");
        terminal_setcolor(VGA_COLOR_WHITE);
        print("Usage: curl [options] <url>\n\n");
        print("Options:\n");
        print("  -X METHOD    HTTP method (GET, POST, PUT, DELETE)\n");
        print("  -d DATA      POST data (automatically sets method to POST)\n");
        print("  -v           Verbose output (show headers)\n");
        print("  -m SECONDS   Timeout in seconds (default: 30)\n");
        print("  -h           Show this help\n");
        print("\nExamples:\n");
        print("  curl http://example.com/\n");
        print("  curl example.com                     # http:// is optional\n");
        print("  curl 192.168.1.1/api\n");
        print("  curl -d \"Hello World\" ntfy.sh/mytopic\n");
        print("  curl -X POST -d \"Test\" http://ntfy.sh/test\n");
        print("  curl -v httpbin.org/get\n");
        print("\nSupported:\n");
        print("  - HTTP/1.1 over TCP\n");
        print("  - GET, POST, PUT, DELETE methods\n");
        print("  - Custom ports (e.g., example.com:8080)\n");
        print("\nNot supported:\n");
        print("  - HTTPS (SSL/TLS)\n");
        print("  - Redirects\n");
        print("  - Cookies\n");
        print("  - Authentication\n");
        return;
    }
    
    http_request_t req = {0};
    strcpy(req.method, "GET");
    req.verbose = false;
    req.timeout_ms = 30000; // 30 seconds
    
    const char* url = NULL;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-X") == 0 && i + 1 < argc) {
            // HTTP method
            strncpy(req.method, argv[++i], sizeof(req.method) - 1);
            // Convert to uppercase
            for (int j = 0; req.method[j]; j++) {
                if (req.method[j] >= 'a' && req.method[j] <= 'z') {
                    req.method[j] = req.method[j] - 'a' + 'A';
                }
            }
        } 
        else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            // POST data
            req.data = argv[++i];
            req.data_length = strlen(req.data);
            // Auto-set method to POST
            if (strcmp(req.method, "GET") == 0) {
                strcpy(req.method, "POST");
            }
        }
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            req.verbose = true;
        }
        else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            int seconds = atoi(argv[++i]);
            if (seconds > 0 && seconds <= 300) {
                req.timeout_ms = seconds * 1000;
            } else {
                print("Warning: Invalid timeout, using default (30s)\n");
            }
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            // Show help
            cmd_curl(1, argv);
            return;
        }
        else if (argv[i][0] != '-') {
            url = argv[i];
        }
        else {
            terminal_setcolor(VGA_COLOR_LIGHT_RED);
            print("Unknown option: ");
            print(argv[i]);
            print("\n");
            terminal_setcolor(VGA_COLOR_WHITE);
            print("Try 'curl -h' for help\n");
            return;
        }
    }
    
    if (!url) {
        terminal_setcolor(VGA_COLOR_LIGHT_RED);
        print("Error: No URL specified\n");
        terminal_setcolor(VGA_COLOR_WHITE);
        print("Try 'curl -h' for help\n");
        return;
    }
    
    // Parse URL
    if (!parse_url(url, &req)) {
        return;
    }
    
    // Display request info
    print("\n");
    terminal_setcolor(VGA_COLOR_LIGHT_CYAN);
    printr("curl: %s http://%s:%d%s\n", 
           req.method, req.hostname, req.port, req.path);
    
    if (req.data) {
        printr("Data: \"%s\" (%d bytes)\n", req.data, req.data_length);
    }
    terminal_setcolor(VGA_COLOR_WHITE);
    
    print("\n");
    
    // Make the request
    http_request_tcp(&req);
}
// To register this command, add to your command initialization:
// register_command("curl", "Make HTTP requests to web servers", cmd_curl);

void kernel_main() {
    terminal_initialize();
    memory_init();
    
    speaker_init();
    speaker_enable();
    debug_memory_status();
    script_init();
    init_physical_memory();
    avfs_init();
    
    set_keyboard_leds(0);
    toggle_caps_lock();
    initialize_cpu_info();
        netstack_init(
    ip_parse("192.168.1.50"),      // Your IP
    ip_parse("255.255.255.0"),     // Subnet mask
    ip_parse("192.168.1.1")        // Gateway
);
    tcp_init();
    // Initialize log files FIRST (with proper size)
    print("[ ");
    terminal_setcolor(VGA_COLOR_GREEN);
    print("OK");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    print(" ] Creating system log files...\n");
    
    avfs_create_file("done.log", 8192);
    avfs_create_file("warn.log", 8192);
    avfs_create_file("error.log", 8192);
    
    // Enable logging to done.log
    set_log_file("done.log");

    // RTL8139 Network Driver
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    print("[ ");
    terminal_setcolor(VGA_COLOR_GREEN);
    print("OK");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    print(" ] Starting RTL8139 network driver...\n");
    radifetch_init();
    rtl8139_init();
    rtl8139_test_io_functions();
    rtl8139_verify_pci_config();
    done("RTL8139 driver loaded", "rtl8139");
    mark_driver_running("rtl8139");

    // RTC
    boot_status(rtc_init(), "rtc", "Real-Time Clock initialized");
    
    // GDT
    boot_status(setup_gdt(), "gdt", "Global Descriptor Table configured");
    
    // Interrupts
    boot_status(setup_interrupts(), "interrupts", "Interrupt handlers registered");
    
    // PIT
    boot_status(setup_pit(1000), "pit", "Programmable Interval Timer set to 1000Hz");
    
    // Task Scheduler
    boot_status(setup_tasks(), "scheduler", "Task scheduler ready");
    
    print("[ ");
    terminal_setcolor(VGA_COLOR_GREEN);
    print("OK");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    print(" ] Registering system commands...\n");
    
    // Command registrations
    if (!register_command("help", "Displays this message", help_command)) {
        error("Failed to register command: help", "command_registry");
        system_error("Command registration", "0x101");
    }
    if (!register_command("mempop", "Memory toolkit", mempop_command)) {
        error("Failed to register command: mempop", "command_registry");
        system_error("Command registration", "0x102");
    }
    if (!register_command("mpop", "Programming language", mpop_command)) {
        error("Failed to register command: mpop", "command_registry");
        system_error("Command registration", "0x115");
    }
    if (!register_command("brainz", "Theme Menu", brains_command)) {
        error("Failed to register command: brainz", "command_registry");
        system_error("Command registration", "0x103");
    }
    if (!register_command("clear", "Clears screen", clear)) {
        error("Failed to register command: clear", "command_registry");
        system_error("Command registration", "0x104");
    }
    if (!register_command("echo", "Echos back text", echo_command)) {
        error("Failed to register command: echo", "command_registry");
        system_error("Command registration", "0x105");
    }
    if (!register_command("reboot", "Reboots the OS", reboot_command)) {
        error("Failed to register command: reboot", "command_registry");
        system_error("Command registration", "0x106");
    }
    if (!register_command("date", "Shows current date and time", date_command)) {
        error("Failed to register command: date", "command_registry");
        system_error("Command registration", "0x108");
    }
    if (!register_command("time", "Shows current time", time_command)) {
        error("Failed to register command: time", "command_registry");
        system_error("Command registration", "0x109");
    }
    if (!register_command("timestamp", "Shows unix timestamp", timestamp_command)) {
        error("Failed to register command: timestamp", "command_registry");
        system_error("Command registration", "0x110");
    }
    if (!register_command("brainfuck", "Brainfuck language", brainfuck_command)) {
        error("Failed to register command: brainfuck", "command_registry");
        system_error("Command registration", "0x113");        
    }
    if (!register_command("settings", "Enter settings", settings_command)) {
        error("Failed to register command: settings", "command_registry");
        system_error("Command registration", "0x127");
    }
    if (!register_command("onan", "Text editor", textspace_command)) {
        error("Failed to register command: onan", "command_registry");
        system_error("Command registration", "0x128");
    }
    if (!register_command("ls", "List directory", ls_command)) {
        error("Failed to register command: ls", "command_registry");
        system_error("Command registration", "0x129");
    }
    if (!register_command("network", "Network utility", networkk_command)) {
        error("Failed to register command: network", "command_registry");
        system_error("Command registration", "0x130");
    }
    if (!register_command("cat", "Reads text file", cat_command)) {
        error("Failed to register command: cat", "command_registry");
        system_error("Command registration", "0x131");
    }
    if (!register_command("rm", "Removes file", rm_command)) {
        error("Failed to register command: rm", "command_registry");
        system_error("Command registration", "0x132");
    }
    if (!register_command("sudo", "Run command in sudoers", sudo_command)) {
        error("Failed to register command: sudo", "command_registry");
        system_error("Command registration", "0x133");
    }
    if (!register_command("tui", "Runs Gui", tui)) {
        error("Failed to register command: tui", "command_registry");
        system_error("Command registration", "0x134");
    }
    if (!register_command("radifetch", "Runs fetch", radifetch_command)) {
        error("Failed to register command: radifetch", "command_registry");
        system_error("Command registration", "0x135");
    }
    if (!register_command("cowsay", "Runs cowsay", cowsay_command)) {
        error("Failed to register command: cowsay", "command_registry");
        system_error("Command registration", "0x136");
    }
    if (!register_command("script", "Scripting language interpreter", cmd_script)) {
        error("Failed to register command: script", "command_registry");
        system_error("Command registration", "0x137");
    }
    if (!register_command("rash", "Executes 'autoexec.rsh' script", rash)) {
        error("Failed to register command: rash", "command_registry");
        system_error("Command registration", "0x138");
    }
    if (!register_command("exit", "Exits the OS", exit_command)) {
        error("Failed to register command: exit", "command_registry");
        system_error("Command registration", "0x139");
    }
    register_command("curl", "Make HTTP requests", cmd_curl);
    done("Command registry populated with 20 commands", "command_registry");
    print("[ ");
    terminal_setcolor(VGA_COLOR_GREEN);
    print("OK");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    printr(" ] Command registry populated with %d commands\n", 20);
    
    print("[ ");
    terminal_setcolor(VGA_COLOR_GREEN);
    print("OK");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    print(" ] Initializing network subsystem...\n");
    network_subsystem_init();
    done("Network subsystem initialized", "network");

    // Register network commands
    register_command("ping", "Ping a host", ping_command);
    register_command("arp", "ARP utilities", arp_command);
    register_command("ifconfig", "Interface configuration", ifconfig_command);
    
    done("Network commands registered", "network");
    print("[ ");
    terminal_setcolor(VGA_COLOR_GREEN);
    print("OK");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    print(" ] Network commands registered\n");
 
    done("System initialization complete", "kernel");
    print("[ ");
    terminal_setcolor(VGA_COLOR_GREEN);
    print("OK");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    print(" ] System initialization complete!\n");
    
    print("[ ");
    terminal_setcolor(VGA_COLOR_GREEN);
    print("OK");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    print(" ] Creating default user configuration...\n");
    const char* MPOP_ENCRYPTION = 
    "; XOR Encryption/Decryption Program\n"
    "; Usage: Set R0 to mode (0=encrypt, 1=decrypt)\n"
    ";        Set R1 to key length\n"
    ";        Set R2 to data length\n"
    ";        Store key at memory address 0\n"
    ";        Store data at memory address 100\n"
    ";        Result will be at memory address 200\n"
    "\n"
    "mov R0, 0\n"          // Mode: 0=encrypt, 1=decrypt
    "mov R1, 5\n"          // Key length
    "mov R2, 12\n"         // Data length
    "\n"
    "; Initialize key (example: 'HELLO')\n"
    "mov R3, 72\n"         // 'H'
    "store 0, R3\n"
    "mov R3, 69\n"         // 'E'
    "store 1, R3\n"
    "mov R3, 76\n"         // 'L'
    "store 2, R3\n"
    "store 3, R3\n"        // 'L'
    "mov R3, 79\n"         // 'O'
    "store 4, R3\n"
    "\n"
    "; Initialize data (example: 'Hello World!')\n"
    "mov R3, 72\n"         // 'H'
    "store 100, R3\n"
    "mov R3, 101\n"        // 'e'
    "store 101, R3\n"
    "mov R3, 108\n"        // 'l'
    "store 102, R3\n"
    "store 103, R3\n"      // 'l'
    "mov R3, 111\n"        // 'o'
    "store 104, R3\n"
    "mov R3, 32\n"         // ' '
    "store 105, R3\n"
    "mov R3, 87\n"         // 'W'
    "store 106, R3\n"
    "mov R3, 111\n"        // 'o'
    "store 107, R3\n"
    "mov R3, 114\n"        // 'r'
    "store 108, R3\n"
    "mov R3, 108\n"        // 'l'
    "store 109, R3\n"
    "mov R3, 100\n"        // 'd'
    "store 110, R3\n"
    "mov R3, 33\n"         // '!'
    "store 111, R3\n"
    "\n"
    "; Main encryption/decryption loop\n"
    "mov R4, 0\n"          // Index counter
    "encrypt_loop:\n"
    "CMP R4, R2\n"
    "JE done\n"
    "\n"
    "; Get data character\n"
    "mov R5, 100\n"
    "add R5, R4\n"
    "load R6, R5\n"        // R6 = data[i]
    "\n"
    "; Get key character\n"
    "mov R7, R4\n"
    "MOD R7, R1\n"         // R7 = i % key_length
    "load R8, R7\n"        // R8 = key[i % key_length]
    "\n"
    "; XOR operation\n"
    "XOR R6, R8\n"         // R6 = data[i] XOR key[i % key_length]
    "\n"
    "; Store result\n"
    "mov R9, 200\n"
    "add R9, R4\n"
    "store R9, R6\n"
    "\n"
    "; Increment counter\n"
    "INC R4\n"
    "JMP encrypt_loop\n"
    "\n"
    "done:\n"
    "; Print result\n"
    "PRINTC 82\n"          // 'R'
    "PRINTC 101\n"         // 'e'
    "PRINTC 115\n"         // 's'
    "PRINTC 117\n"         // 'u'
    "PRINTC 108\n"         // 'l'
    "PRINTC 116\n"         // 't'
    "PRINTC 58\n"          // ':'
    "PRINTC 32\n"          // ' '
    "PRINTC 10\n"          // newline
    "\n"
    "mov R4, 0\n"
    "print_loop:\n"
    "CMP R4, R2\n"
    "JE end\n"
    "mov R5, 200\n"
    "add R5, R4\n"
    "load R6, R5\n"
    "PRINTC R6\n"
    "INC R4\n"
    "JMP print_loop\n"
    "\n"
    "end:\n"
    "PRINTC 10\n"          // newline
    "HALT\n";
    avfs_create_file("encrypt.cfg", strlen(MPOP_ENCRYPTION));
    avfs_write_file("encrypt.cfg", MPOP_ENCRYPTION, strlen(MPOP_ENCRYPTION), 0);
const char* autoexec = 
        "% autoexec.rsh - Display System Information\n"
        "clear\n"
        "radifetch\n"
        "% Set environment variables\n"
        "set OS_NAME RadiOS\n"
        "set OS_VERSION Alpha-1.0\n"
        "set SHELL_VERSION 1.0\n"
        "set USERNAME root\n"
        "\n"
        "% Display system information\n"
        "echo System: $OS_NAME $OS_VERSION\n"
        "echo Shell Version: $SHELL_VERSION\n"
        "echo Current User: $USERNAME\n";
    
    avfs_create_file("autoexec.rsh", strlen(autoexec));
    avfs_write_file("autoexec.rsh", autoexec, strlen(autoexec), 0);

    const char *sample_c = "root";
    avfs_create_file("username.cfg", strlen(sample_c));
    avfs_write_file("username.cfg", sample_c, strlen(sample_c), 0);
    done("Created username configuration", "username.cfg");
    
    const char *sample_cc = "toor";
    avfs_create_file("password.cfg", strlen(sample_cc));
    avfs_write_file("password.cfg", sample_cc, strlen(sample_cc), 0);
    done("Created password configuration", "password.cfg");
    
    const char *vop = "off";
    avfs_create_file("vop", strlen(vop));
    avfs_write_file("vop", vop, strlen(vop), 0);
    done("VOP setting initialized", "vop");

    info("Default credentials: root/toor", "user_config");
    print("[ ");
    terminal_setcolor(VGA_COLOR_GREEN);
    print("OK");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    print(" ] Default credentials: root/toor\n");
   
    print("[ ");
    terminal_setcolor(VGA_COLOR_GREEN);
    print("OK");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    print(" ] Spawning background tasks...\n");
    
    
    done("Background tasks started", "scheduler");
    print("[ ");
    terminal_setcolor(VGA_COLOR_GREEN);
    print("OK");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    print(" ] Background tasks started\n");
    
    allocation_count = 61;
    speaker_play_error_sound();
    
    terminal_clear();
    
    done("Reached target Single-User System", "init");
    print("[ ");
    terminal_setcolor(VGA_COLOR_GREEN);
    print("OK");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    print(" ] Reached target Single-User System\n");
    
    done("Started Getty on tty1", "getty");
    print("[ ");
    terminal_setcolor(VGA_COLOR_GREEN);
    print("OK");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    print(" ] Started Getty on tty1\n");
    terminal_clear();
    
    //login();
    terminal_clear();
    script_init();
     
    // Create beautiful UI
    

// expanded_welcome_screen.c - Complete Welcome Screen in One Function
// Replace your current welcome screen code with this

// ============================================================================
// CORRECTED KERNEL_MAIN BOTTOM SECTION
// This code goes INSIDE kernel_main(), before the closing brace
// Replace from "create_task" onwards but BEFORE the final }
// ============================================================================

    // Background tasks
    create_task(1, (uint32_t)network_poll_task, 0xC00000, 0xBF0000, true);
    create_task(2, (uint32_t)keyboard_read_input, 0xD00000, 0xFF0000, true);

    // ===== WELCOME SCREEN =====
    
    vga_window_t main_win = vga_create_centered_window(70, 24, VGA_COLOR_WHITE, VGA_COLOR_CYAN);
    vga_win_set_title(&main_win, "Welcome to RadiumOS");
    
    // ASCII Logo
    vga_win_puts_colored(&main_win, 10, 2, "  ____            _ _                 ___  ____  ", vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_CYAN));
    vga_win_puts_colored(&main_win, 10, 3, " |  _ \\ __ _  __| (_)_   _ _ __ ___ / _ \\/ ___| ", vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_CYAN));
    vga_win_puts_colored(&main_win, 10, 4, " | |_) / _` |/ _` | | | | | '_ ` _ \\ | | \\___ \\ ", vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_CYAN));
    vga_win_puts_colored(&main_win, 10, 5, " |  _ < (_| | (_| | | |_| | | | | | | |_| |___) |", vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_CYAN));
    vga_win_puts_colored(&main_win, 10, 6, " |_| \\_\\__,_|\\__,_|_|\\__,_|_| |_| |_|\\___/|____/ ", vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_CYAN));
    
    vga_win_puts_centered(&main_win, 8, "A Modern Hobby Operating System");
    vga_win_puts_colored(&main_win, 3, 10, "Version: Alpha 1.0", vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_CYAN));
    vga_win_puts_colored(&main_win, 3, 11, "Developed by: Jose (Cube)", vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_CYAN));
    vga_win_puts_colored(&main_win, 3, 12, "Discord: @scp_2801", vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_CYAN));
    
    vga_win_puts_colored(&main_win, 3, 14, "Features:", vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_CYAN));
    vga_win_puts(&main_win, 3, 15, "  * Custom VGA Window Manager");
    vga_win_puts(&main_win, 3, 16, "  * TCP/IP Network Stack (RTL8139)");
    vga_win_puts(&main_win, 3, 17, "  * Virtual File System (AVFS)");
    vga_win_puts(&main_win, 3, 18, "  * Task Scheduler & Multitasking");
    vga_win_puts(&main_win, 3, 19, "  * Script Interpreter (RSH)");
    
    vga_win_draw_line_h(&main_win, 2, 21, 66, 0xC4);
    vga_win_puts_colored(&main_win, 3, 22, "[T] Terminal  [H] Help  [N] Network  [ESC] Skip", vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_CYAN));
    vga_win_refresh(&main_win);
    
    // Interactive keyboard handler
    int frame = 0;
    bool waiting = true;
    
    while (waiting) {
        // Animate T key (blinking effect)
        if (frame % 10 < 5) {
            vga_win_puts_colored(&main_win, 20, 22, "[T]", vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_WHITE));
        } else {
            vga_win_puts_colored(&main_win, 20, 22, "[T]", vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_CYAN));
        }
        vga_win_refresh(&main_win);
        
        int key = keyboard_key();
        
        if (key == 0x14) { // T - Terminal
            vga_destroy_window(&main_win);
            show_interactive_shell_page(&main_win);
            return;
        }
        else if (key == 0x23) { // H - Help
            vga_destroy_window(&main_win);
            
            vga_window_t help_win = vga_create_centered_window(68, 22, VGA_COLOR_WHITE, VGA_COLOR_BLUE);
            vga_win_set_title(&help_win, "Quick Start Guide");
            
            vga_win_puts_colored(&help_win, 2, 2, "Keyboard Shortcuts:", vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLUE));
            vga_win_puts(&help_win, 2, 4,  " T - Launch Terminal");
            vga_win_puts(&help_win, 2, 5,  " H - Show this help");
            vga_win_puts(&help_win, 2, 6,  " N - Network information");
            vga_win_puts(&help_win, 2, 7,  " F - File manager (ls)");
            vga_win_puts(&help_win, 2, 8,  " S - System info (radifetch)");
            vga_win_puts(&help_win, 2, 9,  " C - Settings");
            vga_win_puts(&help_win, 2, 10, " R - Reboot");
            
            vga_win_puts_colored(&help_win, 2, 12, "Essential Commands:", vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLUE));
            vga_win_puts(&help_win, 2, 14, " help      - Show all commands");
            vga_win_puts(&help_win, 2, 15, " ls        - List files");
            vga_win_puts(&help_win, 2, 16, " cat       - Display file");
            vga_win_puts(&help_win, 2, 17, " network   - Network utilities");
            vga_win_puts(&help_win, 2, 18, " radifetch - System info");
            
            vga_win_puts_centered(&help_win, 20, "Press any key to return...");
            vga_win_refresh(&help_win);
            keyboard_wait_for_key(0);
            vga_destroy_window(&help_win);
            
            // Recreate main window
            main_win = vga_create_centered_window(70, 24, VGA_COLOR_WHITE, VGA_COLOR_CYAN);
            vga_win_set_title(&main_win, "Welcome to RadiumOS");
            vga_win_puts_colored(&main_win, 10, 2, "  ____            _ _                 ___  ____  ", vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_CYAN));
            vga_win_puts_colored(&main_win, 10, 3, " |  _ \\ __ _  __| (_)_   _ _ __ ___ / _ \\/ ___| ", vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_CYAN));
            vga_win_puts_colored(&main_win, 10, 4, " | |_) / _` |/ _` | | | | | '_ ` _ \\ | | \\___ \\ ", vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_CYAN));
            vga_win_puts_colored(&main_win, 10, 5, " |  _ < (_| | (_| | | |_| | | | | | | |_| |___) |", vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_CYAN));
            vga_win_puts_colored(&main_win, 10, 6, " |_| \\_\\__,_|\\__,_|_|\\__,_|_| |_| |_|\\___/|____/ ", vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_CYAN));
            vga_win_puts_centered(&main_win, 8, "A Modern Hobby Operating System");
            vga_win_puts_colored(&main_win, 3, 10, "Version: Alpha 1.0", vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_CYAN));
            vga_win_puts_colored(&main_win, 3, 11, "Developed by: Jose (Cube)", vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_CYAN));
            vga_win_puts_colored(&main_win, 3, 12, "Discord: @scp_2801", vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_CYAN));
            vga_win_puts_colored(&main_win, 3, 14, "Features:", vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_CYAN));
            vga_win_puts(&main_win, 3, 15, "  * Custom VGA Window Manager");
            vga_win_puts(&main_win, 3, 16, "  * TCP/IP Network Stack (RTL8139)");
            vga_win_puts(&main_win, 3, 17, "  * Virtual File System (AVFS)");
            vga_win_puts(&main_win, 3, 18, "  * Task Scheduler & Multitasking");
            vga_win_puts(&main_win, 3, 19, "  * Script Interpreter (RSH)");
            vga_win_draw_line_h(&main_win, 2, 21, 66, 0xC4);
            vga_win_puts_colored(&main_win, 3, 22, "[T] Terminal  [H] Help  [N] Network  [ESC] Skip", vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_CYAN));
            vga_win_refresh(&main_win);
            frame = 0;
        }
        else if (key == 0x31) { // N - Network
            vga_destroy_window(&main_win);
            
            vga_window_t net_win = vga_create_centered_window(68, 20, VGA_COLOR_WHITE, VGA_COLOR_MAGENTA);
            vga_win_set_title(&net_win, "Network Status");
            vga_win_puts_colored(&net_win, 2, 2, "Network Configuration:", vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_MAGENTA));
            vga_win_puts(&net_win, 2, 4, " IP Address:     192.168.1.50");
            vga_win_puts(&net_win, 2, 5, " Subnet Mask:    255.255.255.0");
            vga_win_puts(&net_win, 2, 6, " Gateway:        192.168.1.1");
            vga_win_puts(&net_win, 2, 7, " Driver:         RTL8139");
            vga_win_puts(&net_win, 2, 8, " Status:         Active");
            vga_win_puts_colored(&net_win, 2, 10, "Available Commands:", vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_MAGENTA));
            vga_win_puts(&net_win, 2, 12, " ping <host>     - Ping network host");
            vga_win_puts(&net_win, 2, 13, " ifconfig        - Interface config");
            vga_win_puts(&net_win, 2, 14, " arp             - Show ARP table");
            vga_win_puts(&net_win, 2, 15, " curl <url>      - HTTP requests");
            vga_win_puts(&net_win, 2, 16, " network         - Network menu");
            vga_win_puts_centered(&net_win, 18, "Press any key to return...");
            vga_win_refresh(&net_win);
            keyboard_wait_for_key(0);
            vga_destroy_window(&net_win);
            
            // Recreate main window
            main_win = vga_create_centered_window(70, 24, VGA_COLOR_WHITE, VGA_COLOR_CYAN);
            vga_win_set_title(&main_win, "Welcome to RadiumOS");
            vga_win_puts_colored(&main_win, 10, 2, "  ____            _ _                 ___  ____  ", vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_CYAN));
            vga_win_puts_colored(&main_win, 10, 3, " |  _ \\ __ _  __| (_)_   _ _ __ ___ / _ \\/ ___| ", vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_CYAN));
            vga_win_puts_colored(&main_win, 10, 4, " | |_) / _` |/ _` | | | | | '_ ` _ \\ | | \\___ \\ ", vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_CYAN));
            vga_win_puts_colored(&main_win, 10, 5, " |  _ < (_| | (_| | | |_| | | | | | | |_| |___) |", vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_CYAN));
            vga_win_puts_colored(&main_win, 10, 6, " |_| \\_\\__,_|\\__,_|_|\\__,_|_| |_| |_|\\___/|____/ ", vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_CYAN));
            vga_win_puts_centered(&main_win, 8, "A Modern Hobby Operating System");
            vga_win_puts_colored(&main_win, 3, 10, "Version: Alpha 1.0", vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_CYAN));
            vga_win_puts_colored(&main_win, 3, 11, "Developed by: Jose (Cube)", vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_CYAN));
            vga_win_puts_colored(&main_win, 3, 12, "Discord: @scp_2801", vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_CYAN));
            vga_win_puts_colored(&main_win, 3, 14, "Features:", vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_CYAN));
            vga_win_puts(&main_win, 3, 15, "  * Custom VGA Window Manager");
            vga_win_puts(&main_win, 3, 16, "  * TCP/IP Network Stack (RTL8139)");
            vga_win_puts(&main_win, 3, 17, "  * Virtual File System (AVFS)");
            vga_win_puts(&main_win, 3, 18, "  * Task Scheduler & Multitasking");
            vga_win_puts(&main_win, 3, 19, "  * Script Interpreter (RSH)");
            vga_win_draw_line_h(&main_win, 2, 21, 66, 0xC4);
            vga_win_puts_colored(&main_win, 3, 22, "[T] Terminal  [H] Help  [N] Network  [ESC] Skip", vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_CYAN));
            vga_win_refresh(&main_win);
            frame = 0;
        }
        else if (key == 0x21) { // F - File manager
            vga_destroy_window(&main_win);
            terminal_clear();
            file_manager();
            printr("\nPress any key to return...\n");
            keyboard_wait_for_key(0);
            terminal_clear();
            waiting = false; // Exit to terminal
        }
        else if (key == 0x1F) { // S - System info
            vga_destroy_window(&main_win);
            terminal_clear();
            radifetch_command(0, NULL);
            printr("\nPress any key to return...\n");
            keyboard_wait_for_key(0);
            terminal_clear();
            waiting = false; // Exit to terminal
        }
        else if (key == 0x2E) { // C - Settings
            vga_destroy_window(&main_win);
            settings();
            terminal_clear();
            waiting = false; // Exit to terminal
        }
        else if (key == 0x13) { // R - Reboot
            vga_destroy_window(&main_win);
            terminal_clear();
            printr("Reboot system? (Y/N): ");
            int confirm = keyboard_wait_for_key(0);
            if (confirm == 0x15) { // Y key
                reboot_command(0, NULL);
            }
            terminal_clear();
            waiting = false; // Exit to terminal
        }
        else if (key == 0x01) { // ESC - Skip welcome
            waiting = false;
        }
        
        frame++;
        sleep_ms(50); // 20 FPS
    }
    
    // Clean up and start terminal
    vga_destroy_window(&main_win);
    terminal_clear();
    script_run_autoexec();
    enable_interrupts();
}

// ============================================================================
// INSTRUCTIONS:
// ============================================================================
// 1. Find the lines in kernel_main() that start with:
//    create_task(1, (uint32_t)network_poll_task, ...
//    create_task(2, (uint32_t)keyboard_read_input, ...
//
// 2. Delete everything from those lines to the closing } of kernel_main()
//
// 3. Paste this entire code block in that location
//
// 4. Make sure there's only ONE closing } for kernel_main() at the very end
// ============================================================================