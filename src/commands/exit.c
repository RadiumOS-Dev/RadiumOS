#include "../terminal/terminal.h"
#include "../io/io.h"
#include "../timers/timer.h"
#include "../driver/driver.h"

void exit_command(int argc, char* argv[]) {
    terminal_setcolor(VGA_COLOR_GREEN);
    print("\n\n\nThank you for using RadiumOS!\n\n\n");
    terminal_setcolor(VGA_COLOR_WHITE);

    // Disable interrupts
    asm volatile("cli");

    // Try ACPI shutdown first
    outw(0x604, 0x2000);  // QEMU
    outw(0x4004, 0x3400); // Bochs
    
    // If ACPI fails, try APM
    asm volatile(
        "movw $0x5307, %%ax\n"
        "movw $0x0001, %%bx\n"
        "movw $0x0003, %%cx\n"
        "int $0x15"
        :
        :
        : "ax", "bx", "cx"
    );
    // If all else fails, halt
    print("Shutdown failed. System halted.\n");
    while(1) {
        asm volatile("hlt");
    }
}
