#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include <functional>

// Module states
enum ModuleState {
    MODULE_DISABLED = 0,
    MODULE_ENABLED = 1,
    MODULE_ERROR = 2,
    MODULE_TESTING = 3
};

// Module base class
class Module {
protected:
    String moduleName;
    ModuleState state;
    int priority;
    bool autoStart;
    bool debugEnabled;
    String version;
    DynamicJsonDocument* config;
    
public:
    Module(const char* name);
    virtual ~Module();
    
    // Core module functions
    virtual bool init() = 0;
    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual bool update() = 0;
    virtual bool test() = 0;
    virtual DynamicJsonDocument getStatus() = 0;
    
    // Configuration
    virtual bool loadConfig(DynamicJsonDocument& doc);
    virtual bool saveConfig();
    
    // Getters
    String getName() const { return moduleName; }
    ModuleState getState() const { return state; }
    int getPriority() const { return priority; }
    bool isAutoStart() const { return autoStart; }
    bool isDebugEnabled() const { return debugEnabled; }
    String getVersion() const { return version; }
    
    // Setters
    void setState(ModuleState s) { state = s; }
    void setPriority(int p) { priority = p; }
    void setAutoStart(bool a) { autoStart = a; }
    void setDebugEnabled(bool d) { debugEnabled = d; }
    
    // Logging
    void log(const String& message, const char* level = "INFO");
};

// Module Manager
class ModuleManager {
private:
    std::vector<Module*> modules;
    static ModuleManager* instance;
    std::vector<String> lcdLogs;
    
    ModuleManager();
    
public:
    static ModuleManager* getInstance();
    
    bool registerModule(Module* module);
    bool unregisterModule(const String& name);
    Module* getModule(const String& name);
    
    bool initModules();
    bool startModules();
    bool stopModules();
    bool updateModules();
    
    bool loadGlobalConfig();
    bool saveGlobalConfig();
    bool applyConfig(DynamicJsonDocument& doc);
    
    std::vector<Module*> getModules() { return modules; }
    
    void sortModulesByPriority();
    void appendLCDLog(const String& line);
    void renderLoadingStep(const String& op, int percent);
};

#endif
