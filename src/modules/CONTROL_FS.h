/**
 * @file CONTROL_FS.h
 * @brief Filesystem and configuration storage module interface.
 * @author Michael Kojdl
 * @email michael@kojdl.com
 * @date 2025-11-16
 * @version 1.0.1
 */
/**
 * @file CONTROL_FS.h
 * @brief Filesystem and configuration storage module interface.
 * @author Michael Kojdl
 * @email michael@kojdl.com
 * @date 2025-11-16
 * @version 1.0.1
 */
#ifndef CONTROL_FS_H
#define CONTROL_FS_H

#include "../ModuleManager.h"
#include <SPIFFS.h>
#include <FS.h>
#include "FSDefaults.h"
#include "ConfigManager.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#define FS_MAX_SIZE_DEFAULT 2097152  // 2 MB
#define LOG_MAX_SIZE_DEFAULT 1048576 // 1 MB
#define LOG_FILE_PATH "/logs/system.log"
#define CONFIG_FILE_PATH "/config.json"

/**
 * @class CONTROL_FS
 * @brief Manages SPIFFS, system logging, and configuration integration.
 */
/**
 * @class CONTROL_FS
 * @brief Manages SPIFFS operations, logs, and ConfigManager integration.
 * @details Provides file and directory operations, logging utilities, and config load/save.
 */
/**
 * @class CONTROL_FS
 * @brief Manages SPIFFS operations, logs, and ConfigManager integration.
 * @details Provides file and directory operations, logging utilities, and config load/save.
 */
class CONTROL_FS : public Module {
private:
    size_t fsMaxSize;
    size_t logMaxSize;
    bool fsInitialized;
    SemaphoreHandle_t fsMutex;
    ConfigManager* configManager;
    
    bool initFileSystem();
    bool checkAndCreateDirectories();
    String getLogTimestamp();
    bool validateConfigs();
    bool initVersionAndPopulate();
    size_t countFiles();
    
public:
    CONTROL_FS();
    ~CONTROL_FS();
    
    // Module interface
    bool init() override;
    bool start() override;
    bool stop() override;
    bool update() override;
    bool test() override;
    DynamicJsonDocument getStatus() override;
    
    // File operations
    bool writeFile(const String& path, const String& content, const char* mode = "w");
    String readFile(const String& path);
    bool deleteFile(const String& path);
    bool fileExists(const String& path);
    size_t getFileSize(const String& path);
    
    // Directory operations
    bool createDirectory(const String& path);
    bool removeDirectory(const String& path);
    bool listDirectory(const String& path, std::vector<String>& files);
    
    // Logging
    bool writeLog(const String& message, const char* level = "INFO");
    String readLogs(size_t maxLines = 100);
    bool clearLogs();
    size_t getLogSize();
    
    // Configuration
    bool loadGlobalConfig(DynamicJsonDocument& doc);
    bool saveGlobalConfig(const DynamicJsonDocument& doc);
    bool loadModuleConfig(const String& moduleName, DynamicJsonDocument& doc);
    bool saveModuleConfig(const String& moduleName, const DynamicJsonDocument& doc);
    
    // ConfigManager integration
    ConfigManager* getConfigManager() { return configManager; }
    bool initConfigManager();
    bool migrateLegacyConfigs();
    bool auditFileSystem(bool fix = true);
    
    // Utility
    size_t getFreeSpace();
    size_t getUsedSpace();
    size_t getTotalSpace();
    bool formatFileSystem();
};

#endif
