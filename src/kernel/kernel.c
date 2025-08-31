#include "../terminal/terminal.h"
#include "../keyboard/keyboard.h"
#include "../scheduler/task.h"
#include "../timers/timer.h"
#include "../timers/date.h"
#include "../errors/error.h"
#include "../memory/memory.h"

#include "../io/io.h"
#include "../mpop/mpop.h"
#include "../vfs/filesys.h"
#include "../chemicalCompounds/chem.h"
#include "../icmp/icmp.h"
#include "../login/login.h"
#include "../commands/mempop.h"
#include "../commands/brainz.h"
#include "../commands/clear.h"
#include "../commands/echo.h"
#include "../commands/exit.h"
#include "../commands/reboot.h"
#include "../commands/help.h"
#include "../commands/tui.h"
#include "../commands/meow.h"

#include "../commands/settings.h"
#include "../commands/ping.h"
#include "../commands/text.h"
#include "../commands/radifetch.h"
#include "../cpu/cpu.h"

void kernel_main() {
    // Initialize core systems first
    terminal_initialize();
    memory_init();
    //terminal_clear();
    debug_memory_status();
    // Initialize CPU information system
    initialize_cpu_info();
    init_physical_memory();
    
    // Initialize Real Time Clock
    if (!rtc_init()) {
        handle_error("\nRTC - Initialize Failed\n", "kernel");
    } else {
        print("RTC - Initialized.☢ \n");
    }
    
    // Initialize Global Descriptor Table
    if (!setup_gdt()) {
        handle_error("\nGDT - Initialize Failed\n", "kernel");
    } else {
        print("GDT - Initialized.\n");
    }
    
    // Initialize Interrupt Descriptor Table
    if (!setup_interrupts()) {
        handle_error("\nINTERRUPTS - Initialize Failed\n", "kernel");
    } else {
        print("INTERRUPTS - Initialized.\n");
    }
    
    // Initialize Programmable Interval Timer
    if (!setup_pit(1000)) {
        handle_error("\nPIT - Initialize Failed\n", "kernel");
    } else {
        print("PIT - Initialized.\n");
    }
    
    // Initialize Task Scheduler
    if (!setup_tasks()) {
        handle_error("TASKS - Initialize Failed\n", "kernel");
    } else {
        print("TASKS - Initialized.\n");
    }
    
    // Initialize filesystem
    if (filesys_init() != 0) {
        handle_error("FILESYSTEM - Initialize Failed\n", "kernel");
    } else {
        print("FILESYSTEM - Initialized.\n");
    }
    

    
    // Register all commands
    print("Registering commands...\n");
    
    if (!register_command("help", "Displays this message", help_command)) {
        system_error("Command registration", "0x101");
    }
    if (!register_command("mempop", "Memory toolkit", mempop_command)) {
        system_error("Command registration", "0x102");
    }
    if (!register_command("mpop", "Programming language", mpop_command)) {
        system_error("Command registration", "0x115");
    }
    if (!register_command("brainz", "Theme Menu", brains_command)) {
        system_error("Command registration", "0x103");
    }
    if (!register_command("clear", "Clears screen", clear)) {
        system_error("Command registration", "0x104");
    }
    if (!register_command("echo", "Echos back text", echo_command)) {
        system_error("Command registration", "0x105");
    }
    if (!register_command("reboot", "Reboots the OS", reboot_command)) {
        system_error("Command registration", "0x106");
    }
    if (!register_command("date", "Shows current date and time", date_command)) {
        system_error("Command registration", "0x108");
    }
    if (!register_command("time", "Shows current time", time_command)) {
        system_error("Command registration", "0x109");
    }
    if (!register_command("timestamp", "Shows unix timestamp", timestamp_command)) {
        system_error("Command registration", "0x110");
    }
    if (!register_command("exit", "Exits the OS", exit_command)) {
        system_error("Command registration", "0x111");
    }
    if (!register_command("tui", "Enters tui menu", tui)) {
        system_error("Command registration", "0x112");
    }
    if (!register_command("brainfuck", "Brainfuck language", brainfuck_command)) {
        system_error("Command registration", "0x113");        
    }
    if (!register_command("settings", "Enter settings", settings_command)) {
        system_error("Command registration", "0x127");
    }
    if (!register_command("text", "Enter text editor", text)) {
        system_error("Command registration", "0x128");
    }
    if (!register_command("chem", "Enter chemical compound lookup", chem_command)) {
        system_error("Command registration", "0x129");
    }
    if (!register_command("radifetch", "System info lookup", radifetch_command)) {
        system_error("Command registration", "0x130");
    }
    if (!register_command("network", "System network lookup", network_command)) {
        system_error("Command registration", "0x131");
    }
    if (!register_command("netdiag", "Network Dialog", netdiag_command)) {
        system_error("Command registration", "0x132");
    }

    
    print("Commands registered successfully.\n");

    // Create system tasks
    print("Creating system tasks...\n");

    
    
    print("System tasks created successfully.\n");
    
    // Initialize radifetch system
    if (radifetch_init() != 0) {
        handle_error("RADIFETCH - Initialize Failed\n", "kernel");
    } else {
        print("RADIFETCH - Initialized.\n");
    }
        // Initialize RTL8139 network card
        if (!rtl8139_init()) {
        handle_error("RTL8139 - Initialize Failed\n", "kernel");
    } else {
        print("RTL8139 - Initialized.\n");
    }

    terminal_setcolor(VGA_COLOR_GREEN);
    print("System initialization complete!\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    print("\n");
    terminal_clear();

    // Display welcome banner
    terminal_setcolor(VGA_COLOR_CYAN);
    print("\t\t\t\t\t\t\t          .  .           \n");
    print("\t\t\t\t\t\t\t          dOO  OOb       \n");
    print("\t\t\t\t\t\t\t         dOP'..'YOb      \n");
    print("\t\t\t\t\t\t\t         OOboOOodOO      \n");
    print("\t\t\t\t\t\t\t       ..YOP.  .YOP..    \n");
    print("\t\t\t\t\t\t\t     dOOOOOObOOdOOOOOOb  \n");
    print("\t\t\t\t\t\t\t    dOP' dOYO()OPOb 'YOb \n");
    print("\t\t\t\t\t\t\t        O   OOOO   O     \n");
    print("\t\t\t\t\t\t\t    YOb. YOdOOOObOP .dOP \n");
    print("\t\t\t\t\t\t\t     YOOOOOOP  YOOOOOOP  \n");
    print("\t\t\t\t\t\t\t       ''''      ''''    \n");
    terminal_setcolor(VGA_COLOR_WHITE);
    
    info("    Welcome to Radium OS\n", __FILE__);
    info("    Type 'help' for available commands", __FILE__);
    info("    Type 'radifetch' for system information", __FILE__);
    info("    Type 'network status' to check network card", __FILE__);
    

    keyboard_read_input();

    // Enable interrupts and start the system
    enable_interrupts();

}
