#include <stdint.h> // Include for uint8_t type
#include "../io/io.h"
/* keyboard interface IO port: data and control */
#define KBRD_INTRFC 0x64

/* keyboard interface bits */
#define KBRD_BIT_KDATA 0 /* keyboard data is in buffer (output buffer is empty) (bit 0) */
#define KBRD_BIT_UDATA 1 /* user data is in buffer (command buffer is empty) (bit 1) */

#define KBRD_IO 0x60 /* keyboard IO port */
#define KBRD_RESET 0xFE /* reset CPU command */

#define bit(n) (1 << (n)) /* Set bit n to 1 */

/* Check if bit n in flags is set */
#define check_flag(flags, n) ((flags) & bit(n))

void reboot_command(int argc, char* argv[]) {
    uint8_t temp;


    /* Clear all keyboard buffers (output and command buffers) */
    do {
        temp = inb(KBRD_INTRFC); /* Read status from keyboard interface */
        if (check_flag(temp, KBRD_BIT_KDATA)) {
            inb(KBRD_IO); /* Read keyboard data to clear it */
        }
    } while (check_flag(temp, KBRD_BIT_UDATA)); /* Continue until command buffer is empty */

    outb(KBRD_INTRFC, KBRD_RESET); /* Pulse CPU reset line */

    /* If that didn't work, halt the CPU */
    while (1) {
        asm volatile ("hlt"); /* Halt the CPU */
    }
}
