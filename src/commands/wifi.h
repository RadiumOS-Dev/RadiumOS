#ifndef WIFI_COMMAND_H
#define WIFI_COMMAND_H

#include <stdint.h>
#include <stdbool.h>
#include "../wifi/wifi.h"
#include "../terminal/terminal.h"
#include "../timers/timer.h"

// WiFi command function
void wifi_command(int argc, char* argv[]);

// Helper functions
void wifi_show_help(void);
void wifi_show_status(void);
void wifi_scan_networks(void);
void wifi_connect_network(const char* ssid, const char* password);
void wifi_disconnect_network(void);
void wifi_show_info(void);
void wifi_enable_device(void);
void wifi_disable_device(void);

// WiFi command error codes
#define WIFI_CMD_SUCCESS            0
#define WIFI_CMD_ERROR_NOT_INIT     -1
#define WIFI_CMD_ERROR_DISABLED     -2
#define WIFI_CMD_ERROR_NO_FIRMWARE  -3
#define WIFI_CMD_ERROR_HARDWARE     -4
#define WIFI_CMD_ERROR_INVALID_ARG  -5
#define WIFI_CMD_ERROR_NOT_CONNECTED -6

// WiFi command constants
#define WIFI_MAX_SSID_LENGTH        32
#define WIFI_MAX_PASSWORD_LENGTH    64
#define WIFI_MAX_NETWORKS_DISPLAY   20

// WiFi command status messages
#define WIFI_MSG_NOT_INITIALIZED    "WiFi not initialized"
#define WIFI_MSG_DEVICE_DISABLED    "WiFi device is disabled"
#define WIFI_MSG_NO_FIRMWARE        "WiFi firmware not loaded"
#define WIFI_MSG_HARDWARE_ERROR     "Hardware communication error"
#define WIFI_MSG_NOT_CONNECTED      "Not connected to any network"
#define WIFI_MSG_ALREADY_ENABLED    "WiFi device is already enabled"
#define WIFI_MSG_ALREADY_DISABLED   "WiFi device is already disabled"

#endif // WIFI_COMMAND_H
