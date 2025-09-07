#include "../terminal/terminal.h"
#include "../utility/utility.h"
#include "../keyboard/keyboard.h"
#include "../io/io.h"
#include "returnRegister.h"

void intToHexString(uint8_t value, char* buffer) {
    // Convert the integer to a hexadecimal string
    const char hexDigits[] = "0123456789ABCDEF";
    buffer[0] = '0';
    buffer[1] = 'x';
    buffer[2] = hexDigits[(value >> 4) & 0x0F]; // High nibble
    buffer[3] = hexDigits[value & 0x0F];        // Low nibble
    buffer[4] = '\0'; // Null-terminate the string
}

uint16_t hexStringToUint16(const char* str) {
    uint16_t result = 0;
    while (*str) {
        result <<= 4; // Shift left by 4 bits
        if (*str >= '0' && *str <= '9') {
            result += *str - '0';
        } else if (*str >= 'A' && *str <= 'F') {
            result += *str - 'A' + 10;
        } else if (*str >= 'a' && *str <= 'f') {
            result += *str - 'a' + 10;
        }
        str++;
    }
    return result;
}



void returnRegister_command(int argc, char* argv[]) {
    char userinput[COMMAND_BUFFER_SIZE]; 
    print("\n\n\n`help` Shows this message.\n`run` runs inputted register codes such as: `0x70`\n`info` Gets the information about a register.\n\n\n");
    while (1) {
        keyboard_input(userinput); 

        if (strcmp(userinput, "help") == 0) {
            print("\n\n\n`help` Shows this message.\n`run` runs inputted register codes such as: `0x70`\n`info` Gets the information about a register.\n\n\n");
        } else if (strcmp(userinput, "run") == 0) {
            print("\n\n\nThank you for using Thornes register returner !\n\n\n"); 
            char registerinput[COMMAND_BUFFER_SIZE];
            keyboard_input(registerinput);

            // Convert the hexadecimal string to an integer
            uint16_t port = hexStringToUint16(registerinput);
            uint8_t value = 0xFF; // Example value to write, modify as needed
            outb(port, value); 
            print("Wrote value to port.\n");
        } else if (strcmp(userinput, "info") == 0) {
            char registerinput[COMMAND_BUFFER_SIZE];
            keyboard_input(registerinput);

            // Convert the hexadecimal string to an integer
            uint16_t port = hexStringToUint16(registerinput);
            uint8_t value = inb(port); // Read the value from the port
            
            // Prepare the output string
            char buffer[50];
            char portString[10];
            char valueString[10];
            
            // Convert port and value to hexadecimal strings
            intToHexString(port, portString);
            intToHexString(value, valueString);
            
            // Create the output message
            char message[100];
            int i = 0;
            const char* strings[] = { "Value at port ", portString, ": ", valueString, "\n" };
            for (int j = 0; j < 5; j++) {
                const char* str = strings[j];
                while (*str) {
                    message[i++] = *str++;
                }
            }
            message[i] = '\0'; // Null-terminate the message
            
            print(message);
        }
    }
}
