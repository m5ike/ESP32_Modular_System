#ifndef CONTROL_FS_H
#define CONTROL_FS_H

#include "../ModuleManager.h"
#include <SPIFFS.h>
#include <FS.h>

#define FS_MAX_SIZE_DEFAULT 2097152  // 2 MB
#define LOG_MAX_SIZE_DEFAULT 1048576 // 1 MB
#define LOG_FILE_PATH "/logs/system.log"
#define CONFIG_FILE_PATH "/config/global.json"

class CONTROL_FS : public Module {
private:
    size_t fsMaxSize;
    size_t logMaxSize;
    bool fsInitialized;
    
    bool initFileSystem();
    bool checkAndCreateDirectories();
    String getLogTimestamp();
    
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
    
    // Utility
    size_t getFreeSpace();
    size_t getUsedSpace();
    size_t getTotalSpace();
    bool formatFileSystem();
};

#endif
