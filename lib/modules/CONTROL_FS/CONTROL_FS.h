#ifndef CONTROL_FS_H
#define CONTROL_FS_H

#include "ModuleBase.h"
#include <SPIFFS.h>
#include <FS.h>

class CONTROL_FS : public ModuleBase {
private:
    size_t totalBytes;
    size_t usedBytes;
    size_t freeBytes;
    
    bool mountSPIFFS() {
        if (!SPIFFS.begin(true)) {
            logf("ERROR", "Failed to mount SPIFFS");
            return false;
        }
        
        totalBytes = SPIFFS.totalBytes();
        usedBytes = SPIFFS.usedBytes();
        freeBytes = totalBytes - usedBytes;
        
        logf("INFO", "SPIFFS mounted: %d bytes total, %d used, %d free", 
             totalBytes, usedBytes, freeBytes);
        return true;
    }
    
public:
    CONTROL_FS() : ModuleBase("CONTROL_FS") {
        priority = 100; // Highest priority
        autostart = true;
    }
    
    bool init() override {
        log("INFO", "Initializing File System module...");
        
        if (!mountSPIFFS()) {
            return false;
        }
        
        // Create directories if they don't exist
        createDirectories();
        
        return true;
    }
    
    bool start() override {
        log("INFO", "Starting File System module...");
        return true;
    }
    
    bool stop() override {
        log("INFO", "Stopping File System module...");
        SPIFFS.end();
        return true;
    }
    
    bool test() override {
        log("INFO", "Testing File System module...");
        
        // Test write
        File testFile = SPIFFS.open("/test.txt", "w");
        if (!testFile) {
            log("ERROR", "Failed to create test file");
            return false;
        }
        testFile.println("Test");
        testFile.close();
        
        // Test read
        testFile = SPIFFS.open("/test.txt", "r");
        if (!testFile) {
            log("ERROR", "Failed to read test file");
            return false;
        }
        String content = testFile.readString();
        testFile.close();
        
        // Clean up
        SPIFFS.remove("/test.txt");
        
        log("INFO", "File System test passed");
        return true;
    }
    
    void loop() override {
        // Update filesystem stats periodically
        static unsigned long lastUpdate = 0;
        if (millis() - lastUpdate > 60000) { // Every minute
            totalBytes = SPIFFS.totalBytes();
            usedBytes = SPIFFS.usedBytes();
            freeBytes = totalBytes - usedBytes;
            lastUpdate = millis();
        }
    }
    
    bool loadConfig() override {
        if (!SPIFFS.exists(configPath.c_str())) {
            log("WARN", "Config file not found, creating default");
            return saveConfig();
        }
        
        File file = SPIFFS.open(configPath.c_str(), "r");
        if (!file) {
            log("ERROR", "Failed to open config file");
            return false;
        }
        
        DeserializationError error = deserializeJson(*config, file);
        file.close();
        
        if (error) {
            logf("ERROR", "Failed to parse config: %s", error.c_str());
            return false;
        }
        
        log("INFO", "Config loaded successfully");
        return true;
    }
    
    bool saveConfig() override {
        (*config)["max_size"] = FS_MAX_SIZE;
        (*config)["log_max_size"] = LOG_MAX_SIZE;
        
        File file = SPIFFS.open(configPath.c_str(), "w");
        if (!file) {
            log("ERROR", "Failed to create config file");
            return false;
        }
        
        serializeJson(*config, file);
        file.close();
        
        log("INFO", "Config saved successfully");
        return true;
    }
    
    // Helper functions
    void createDirectories() {
        // SPIFFS doesn't have directories, but we can create empty marker files
        log("INFO", "Ensuring directory structure...");
    }
    
    // File operations
    bool writeFile(const String& path, const String& content) {
        File file = SPIFFS.open(path.c_str(), "w");
        if (!file) {
            logf("ERROR", "Failed to create file: %s", path.c_str());
            return false;
        }
        
        file.print(content);
        file.close();
        
        logf("INFO", "File written: %s", path.c_str());
        return true;
    }
    
    String readFile(const String& path) {
        if (!SPIFFS.exists(path.c_str())) {
            logf("ERROR", "File not found: %s", path.c_str());
            return "";
        }
        
        File file = SPIFFS.open(path.c_str(), "r");
        if (!file) {
            logf("ERROR", "Failed to open file: %s", path.c_str());
            return "";
        }
        
        String content = file.readString();
        file.close();
        
        return content;
    }
    
    bool deleteFile(const String& path) {
        if (!SPIFFS.remove(path.c_str())) {
            logf("ERROR", "Failed to delete file: %s", path.c_str());
            return false;
        }
        
        logf("INFO", "File deleted: %s", path.c_str());
        return true;
    }
    
    bool fileExists(const String& path) {
        return SPIFFS.exists(path.c_str());
    }
    
    size_t getFileSize(const String& path) {
        if (!SPIFFS.exists(path.c_str())) {
            return 0;
        }
        
        File file = SPIFFS.open(path.c_str(), "r");
        if (!file) {
            return 0;
        }
        
        size_t size = file.size();
        file.close();
        
        return size;
    }
    
    // Get filesystem stats
    size_t getTotalBytes() const { return totalBytes; }
    size_t getUsedBytes() const { return usedBytes; }
    size_t getFreeBytes() const { return freeBytes; }
    float getUsagePercent() const { 
        return totalBytes > 0 ? (float)usedBytes / totalBytes * 100.0f : 0.0f; 
    }
    
    String getStatusJson() override {
        DynamicJsonDocument doc(1024);
        doc["name"] = moduleName;
        doc["state"] = (int)state;
        doc["total_bytes"] = totalBytes;
        doc["used_bytes"] = usedBytes;
        doc["free_bytes"] = freeBytes;
        doc["usage_percent"] = getUsagePercent();
        
        String output;
        serializeJson(doc, output);
        return output;
    }
};

#endif // CONTROL_FS_H
