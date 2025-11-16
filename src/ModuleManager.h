#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include <functional>
#include "FreeRTOSTypes.h"
#include "TaskBase.h"
#include "QueueBase.h"

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
    bool critical;
    TaskBase* taskBase;
    QueueBase* queueBase;
    TaskConfig taskCfg;
    QueueConfig queueCfg;
    bool useTask;
    bool useQueue;
    
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
    virtual bool callFunctionByName(const String& name, DynamicJsonDocument* params, String& result) { return false; }
    
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
    bool isCritical() const { return critical; }
    TaskBase* getTask() const { return taskBase; }
    QueueBase* getQueue() const { return queueBase; }
    TaskConfig getTaskConfig() const { return taskCfg; }
    QueueConfig getQueueConfig() const { return queueCfg; }
    bool getUseTask() const { return useTask; }
    bool getUseQueue() const { return useQueue; }
    
    // Setters
    void setState(ModuleState s) { state = s; }
    void setPriority(int p) { priority = p; }
    void setAutoStart(bool a) { autoStart = a; }
    void setDebugEnabled(bool d) { debugEnabled = d; }
    void setCritical(bool c) { critical = c; }
    void attachTask(TaskBase* t) { taskBase = t; }
    void attachQueue(QueueBase* q) { queueBase = q; }
    void setTaskConfig(const TaskConfig& c) { taskCfg = c; }
    void setQueueConfig(const QueueConfig& c) { queueCfg = c; }
    void setUseTask(bool u) { useTask = u; }
    void setUseQueue(bool u) { useQueue = u; }
    
    // Logging
  void log(const String& message, const char* level = "INFO");
};

// Module Manager
class ModuleManager {
private:
    std::vector<Module*> modules;
    static ModuleManager* instance;
    std::vector<String> lcdLogs;
    bool wifiConnectedLast;
    
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
    bool startModuleTask(Module* mod);
    bool ensureModuleQueue(Module* mod);
    
    bool loadGlobalConfig();
    bool saveGlobalConfig();
    bool applyConfig(DynamicJsonDocument& doc);
    
    std::vector<Module*> getModules() { return modules; }
    
    void sortModulesByPriority();
    void appendLCDLog(const String& line);
    void renderLoadingStep(const String& op, int percent);
};

#endif
