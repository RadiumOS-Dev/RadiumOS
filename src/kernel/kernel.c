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
#include "../cpu/cpu.h"
#include "../driver/driver.h"

#include "../arp/arp.h"
#include "../rtl8139/rtl8139.h"
#include "../icmp/icmp.h"

#include "../commands/mempop.h"
#include "../commands/brainz.h"
#include "../commands/clear.h"
#include "../commands/echo.h"
#include "../commands/exit.h"
#include "../commands/reboot.h"
#include "../commands/help.h"
#include "../commands/tui.h"
#include "../commands/meow.h"
#include "../commands/text.h"
#include "../commands/settings.h"
#include "../commands/ping.h"
#include "../commands/radifetch.h"
#include "../commands/gambling.h"


void kernel_main() {
    terminal_initialize();
    memory_init();
    speaker_init();
    debug_memory_status();
    initialize_cpu_info();
    init_physical_memory();
    set_keyboard_leds(0);
    toggle_caps_lock();
    

    if (!rtc_init()) {
        handle_error("\nRTC - Initialize Failed\n", "kernel");
    } else {
        print("RTC - Initialized.☢ \n");
    }
    
    if (!setup_gdt()) {
        handle_error("\nGDT - Initialize Failed\n", "kernel");
    } else {
        print("GDT - Initialized.\n");
    }
    
    if (!setup_interrupts()) {
        handle_error("\nINTERRUPTS - Initialize Failed\n", "kernel");
    } else {
        print("INTERRUPTS - Initialized.\n");
    }
    
    if (!setup_pit(1000)) {
        handle_error("\nPIT - Initialize Failed\n", "kernel");
    } else {
        print("PIT - Initialized.\n");
    }
    
    if (!setup_tasks()) {
        handle_error("TASKS - Initialize Failed\n", "kernel");
    } else {
        print("TASKS - Initialized.\n");
    }
    
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
    if (!register_command("ping", "Pings server", text)) {
        system_error("Command registration", "0x128");
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
    if (!register_command("textspace", "MPOP text editor", textspace_command)) {
        system_error("Command registration", "0x134");
    }
    if (!register_command("gambling", "Gamble (blackjack)", gambling_command)) {
        system_error("Command registration", "0x134");
    }
    if (radifetch_init() != 0) {
        handle_error("RADIFETCH - Initialize Failed\n", "kernel");
    } else {
        print("RADIFETCH - Initialized.\n");
    }
        if (!rtl8139_init()) {
        handle_error("RTL8139 - Initialize Failed\n", "kernel");
    } else {
        print("RTL8139 - Initialized.\n");
    }
    
    terminal_setcolor(VGA_COLOR_GREEN);
    print("System initialization complete!\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    
    print("\n");

    //speaker_play_error_sound();
    arp_init(RTL8139->mac_address, "10.0.2.2");
    //meltdown_screen("Test Meltdown", __FILE__, 167, 0x14, 1230, 190);
    keyboard_read_input();
    
    

}
