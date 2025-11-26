#include "../keyboard/keyboard.h"
#include "../terminal/terminal.h"
#include "../Avfs/Avfs.h"
#include "../utility/utility.h"  

void login() {
    char username[256];
    char password[256];
    
    // Load credentials from files
    if (avfs_file_exists("username.cfg")) {     
        if (avfs_get_content("username.cfg", username, sizeof(username)) == 0) {
            // Successfully loaded username (silent for security)
        }
    } else {
        print("[ ");
        terminal_setcolor(VGA_COLOR_RED);
        print("FAIL");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        print(" ] No username.cfg found - cannot authenticate\n");
        print("         Run setup wizard to create user account first\n");
        return;
    }
    
    if (avfs_file_exists("password.cfg")) {     
        if (avfs_get_content("password.cfg", password, sizeof(password)) == 0) {
            // Successfully loaded password (silent for security)
        }
    } else {
        print("[ ");
        terminal_setcolor(VGA_COLOR_RED);
        print("FAIL");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        print(" ] No password.cfg found - cannot authenticate\n");
        print("         Seriously, did you even set up your system?\n");
        return;
    }
    
    // Display boot-like messages
    print("\n");
    print("[ ");
    terminal_setcolor(VGA_COLOR_GREEN);
    print("OK");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    print(" ] Started User Login Service.\n");
    
    print("[ ");
    terminal_setcolor(VGA_COLOR_GREEN);
    print("OK");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    print(" ] Reached target Login Prompts.\n");
    print("\n");
    
    // System info (Linux-like)
    print("RadiumOS 1.0 LTS tty1\n");
    print("\n");
    
    int attempts = 0;
    while (attempts < 3) {  
        char username_userinputted[COMMAND_BUFFER_SIZE];
        char password_userinputted[COMMAND_BUFFER_SIZE];
        
        print("localhost login: ");
        keyboard_input(username_userinputted);
        
        print("Password: ");
        keyboard_input_secure(password_userinputted);
        
        if (strcmp(username_userinputted, username) == 0) {
            if (strcmp(password_userinputted, password) == 0) {
                print("\n");
                print("[ ");
                terminal_setcolor(VGA_COLOR_GREEN);
                print("OK");
                terminal_setcolor(VGA_COLOR_LIGHT_GREY);
                printr(" ] Authentication successful for user '%s'\n", username);
                
                print("[ ");
                terminal_setcolor(VGA_COLOR_GREEN);
                print("OK");
                terminal_setcolor(VGA_COLOR_LIGHT_GREY);
                print(" ] Starting session...\n");
                
                print("[ ");
                terminal_setcolor(VGA_COLOR_GREEN);
                print("OK");
                terminal_setcolor(VGA_COLOR_LIGHT_GREY);
                print(" ] Mounted user home directory\n");
                
                print("[ ");
                terminal_setcolor(VGA_COLOR_GREEN);
                print("OK");
                terminal_setcolor(VGA_COLOR_LIGHT_GREY);
                print(" ] Loaded user environment variables\n");
                print("\n");
                
                print("Welcome back! Last login: just now (you literally just logged in)\n");
                print("You have 0 new mail. (because mail isn't implemented yet lol)\n");
                print("\n");
                break;
            } else {
                attempts++;
                print("\n");
                print("Login incorrect\n");
                print("[ ");
                terminal_setcolor(VGA_COLOR_LIGHT_BROWN);
                print("WARN");
                terminal_setcolor(VGA_COLOR_LIGHT_GREY);
                printr(" ] Failed password attempt %d of 3 for user '%s'\n", attempts, username_userinputted);
                print("         (nice try though)\n");
                print("\n");
            }
        } else {
            attempts++;
            print("\n");
            print("Login incorrect\n");
            print("[ ");
            terminal_setcolor(VGA_COLOR_LIGHT_BROWN);
            print("WARN");
            terminal_setcolor(VGA_COLOR_LIGHT_GREY);
            printr(" ] Failed login attempt %d of 3\n", attempts);
            print("         (that username doesn't even exist...)\n");
            print("\n");
        }
    }
    
    if (attempts >= 3) {
        print("\n");
        print("[ ");
        terminal_setcolor(VGA_COLOR_RED);
        print("FAIL");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        print(" ] Authentication failure: Too many failed attempts\n");
        
        print("[ ");
        terminal_setcolor(VGA_COLOR_RED);
        print("FAIL");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
        print(" ] User login service terminated\n");
        print("\n");
        
        print("Okay buddy, that's enough tries for now.\n");
        print("Maybe try remembering your password next time? Just a thought.\n");
        print("\n");
        while (1) {
            asm volatile ("nop");
        }
    }
}