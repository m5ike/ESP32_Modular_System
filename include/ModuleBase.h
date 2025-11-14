#ifndef MODULE_BASE_H
#define MODULE_BASE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "Config.h"

// Module States
enum ModuleState {
    MODULE_DISABLED = 0,
    MODULE_ENABLED = 1,
    MODULE_ERROR = 2,
    MODULE_INITIALIZING = 3,
    MODULE_RUNNING = 4
};

// Base class for all modules
class ModuleBase {
protected:
    String moduleName;
    ModuleState state;
    uint8_t priority;
    bool autostart;
    bool testMode;
    bool debugMode;
    String version;
    String configPath;
    
    // Module configuration
    DynamicJsonDocument* config;
    
public:
    ModuleBase(const char* name) 
        : moduleName(name), 
          state(MODULE_DISABLED),
          priority(50),
          autostart(false),
          testMode(true),
          debugMode(false),
          version("1.0.0") {
        config = new DynamicJsonDocument(4096);
        configPath = String(MODULE_CONFIG_PATH) + moduleName + ".json";
    }
    
    virtual ~ModuleBase() {
        if (config) {
            delete config;
        }
    }
    
    // Pure virtual functions - must be implemented by derived classes
    virtual bool init() = 0;
    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual bool test() = 0;
    virtual void loop() = 0;
    virtual bool loadConfig() = 0;
    virtual bool saveConfig() = 0;
    
    // Getters
    String getName() const { return moduleName; }
    ModuleState getState() const { return state; }
    uint8_t getPriority() const { return priority; }
    bool isAutostart() const { return autostart; }
    bool isTestMode() const { return testMode; }
    bool isDebugMode() const { return debugMode; }
    String getVersion() const { return version; }
    
    // Setters
    void setState(ModuleState s) { state = s; }
    void setPriority(uint8_t p) { priority = p; }
    void setAutostart(bool a) { autostart = a; }
    void setTestMode(bool t) { testMode = t; }
    void setDebugMode(bool d) { debugMode = d; }
    
    // Status
    bool isEnabled() const { return state != MODULE_DISABLED; }
    bool isRunning() const { return state == MODULE_RUNNING; }
    bool hasError() const { return state == MODULE_ERROR; }
    
    // Debug logging
    void log(const char* level, const char* message) {
        if (debugMode || strcmp(level, "ERROR") == 0) {
            Serial.printf("[%s][%s] %s\n", moduleName.c_str(), level, message);
        }
    }
    
    void logf(const char* level, const char* format, ...) {
        if (debugMode || strcmp(level, "ERROR") == 0) {
            char buffer[256];
            va_list args;
            va_start(args, format);
            vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);
            Serial.printf("[%s][%s] %s\n", moduleName.c_str(), level, buffer);
        }
    }
    
    // Web/API Handlers - can be overridden
    virtual String getStatusJson() {
        DynamicJsonDocument doc(512);
        doc["name"] = moduleName;
        doc["state"] = (int)state;
        doc["priority"] = priority;
        doc["autostart"] = autostart;
        doc["debug"] = debugMode;
        doc["version"] = version;
        
        String output;
        serializeJson(doc, output);
        return output;
    }
    
    virtual bool handleWebRequest(const String& uri, const String& method, const String& body) {
        return false; // Not implemented by default
    }
    
    virtual String handleApiRequest(const String& endpoint, const String& method, const String& body) {
        return "{\"error\":\"Not implemented\"}";
    }
};

#endif // MODULE_BASE_H
