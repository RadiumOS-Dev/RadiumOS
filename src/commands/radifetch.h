#ifndef RADIFETCH_H
#define RADIFETCH_H

#include <stdint.h>

/**
 * @file radifetch.h
 * @brief System information display utility for RadiumOS
 * 
 * RadiFetch displays comprehensive system information including:
 * - CPU details (brand, architecture, features)
 * - Memory usage with visual representation
 * - Cache information
 * - System uptime and status
 * - Color palette demonstration
 */

/**
 * @brief Display comprehensive system information
 * 
 * Shows detailed system information including CPU specs, memory usage,
 * cache details, and system status in a visually appealing format
 * similar to neofetch but tailored for RadiumOS.
 */
void radifetch(void);

/**
 * @brief Command handler for radifetch command
 * 
 * This function serves as the command interface for the radifetch utility.
 * It can be registered with the command system to allow users to run
 * radifetch from the shell.
 * 
 * @param argc Number of command line arguments
 * @param argv Array of command line argument strings
 * 
 * Usage examples:
 * - radifetch          : Display full system information
 * - radifetch --help   : Show help information (if implemented)
 * - radifetch --short  : Display condensed information (if implemented)
 */
void radifetch_command(int argc, char* argv[]);

/**
 * @brief Display condensed system information
 * 
 * Shows a shorter version of system information with only
 * the most essential details.
 */
void radifetch_short(void);

/**
 * @brief Display only CPU information
 * 
 * Shows detailed CPU information including brand, features,
 * cache, and topology.
 */
void radifetch_cpu(void);

/**
 * @brief Display only memory information
 * 
 * Shows memory usage statistics with visual representation.
 */
void radifetch_memory(void);

/**
 * @brief Display help information for radifetch
 * 
 * Shows usage information and available options for the
 * radifetch command.
 */
void radifetch_help(void);

/**
 * @brief Initialize radifetch system
 * 
 * Initializes any required subsystems for radifetch operation.
 * Should be called during system startup.
 * 
 * @return 0 on success, -1 on failure
 */
int radifetch_init(void);

/**
 * @brief Cleanup radifetch resources
 * 
 * Cleans up any allocated resources used by radifetch.
 * Should be called during system shutdown.
 */
void radifetch_cleanup(void);

#endif // RADIFETCH_H
