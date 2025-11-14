#ifndef CONFIG_H
#define CONFIG_H

// ESP32 Board Configuration - 1.9" LCD ST7789
// Based on ESP32-WROOM-32 module

// LCD ST7789 Pin Configuration (SPI)
#define LCD_MOSI    23  // GPIO23 -> MOSI
#define LCD_SCLK    18  // GPIO18 -> SCLK
#define LCD_CS      15  // GPIO15 -> CS
#define LCD_DC      2   // GPIO2  -> DC
#define LCD_RST     4   // GPIO4  -> RST
#define LCD_BLK     32  // GPIO32 -> Backlight

// LCD Display Settings
#define LCD_WIDTH   170
#define LCD_HEIGHT  320

// Serial Configuration
#define SERIAL_BAUD 460800

// File System Configuration
#define FS_MAX_SIZE         2097152  // 2 MB
#define LOG_MAX_SIZE        1048576  // 1 MB
#define CONFIG_PATH         "/config/global.json"
#define MODULE_CONFIG_PATH  "/config/"
#define LOG_PATH            "/logs/"
#define WEB_PATH            "/web/"

// WiFi Configuration Defaults
#define WIFI_DEFAULT_SSID   "ESP32-LCD"
#define WIFI_DEFAULT_PSK    "12345678"
#define WIFI_AP_IP          "192.168.4.1"
#define WIFI_AP_GATEWAY     "192.168.4.1"
#define WIFI_AP_SUBNET      "255.255.255.0"

// Web Server Configuration
#define WEB_SERVER_PORT     80
#define WEB_API_PORT        80

// Module System
#define MAX_MODULES         16
#define MODULE_NAME_LENGTH  32

// Debug Levels
#define DEBUG_NONE          0
#define DEBUG_ERROR         1
#define DEBUG_WARNING       2
#define DEBUG_INFO          3
#define DEBUG_VERBOSE       4

// Default Debug Level
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL DEBUG_INFO
#endif

// Debug Macros
#if DEBUG_LEVEL >= DEBUG_ERROR
#define DEBUG_E(fmt, ...) Serial.printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
#else
#define DEBUG_E(fmt, ...)
#endif

#if DEBUG_LEVEL >= DEBUG_WARNING
#define DEBUG_W(fmt, ...) Serial.printf("[WARN] " fmt "\n", ##__VA_ARGS__)
#else
#define DEBUG_W(fmt, ...)
#endif

#if DEBUG_LEVEL >= DEBUG_INFO
#define DEBUG_I(fmt, ...) Serial.printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#else
#define DEBUG_I(fmt, ...)
#endif

#if DEBUG_LEVEL >= DEBUG_VERBOSE
#define DEBUG_V(fmt, ...) Serial.printf("[VERBOSE] " fmt "\n", ##__VA_ARGS__)
#else
#define DEBUG_V(fmt, ...)
#endif

#endif // CONFIG_H
