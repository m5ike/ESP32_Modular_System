#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

#include <Arduino.h>
#include <vector>
#include "ModuleBase.h"
#include "Config.h"

class ModuleManager {
private:
    std::vector<ModuleBase*> modules;
    bool initialized;
    
    // Sort modules by priority (higher priority first)
    void sortModulesByPriority() {
        std::sort(modules.begin(), modules.end(), 
            [](ModuleBase* a, ModuleBase* b) {
                return a->getPriority() > b->getPriority();
            });
    }
    
public:
    ModuleManager() : initialized(false) {}
    
    ~ModuleManager() {
        for (auto module : modules) {
            delete module;
        }
        modules.clear();
    }
    
    // Register a module
    bool registerModule(ModuleBase* module) {
        if (!module) return false;
        
        modules.push_back(module);
        DEBUG_I("Module registered: %s (priority: %d)", 
                module->getName().c_str(), module->getPriority());
        return true;
    }
    
    // Initialize all modules
    bool initializeAll() {
        sortModulesByPriority();
        
        DEBUG_I("Initializing %d modules...", modules.size());
        
        for (auto module : modules) {
            if (module->isEnabled()) {
                DEBUG_I("Initializing module: %s", module->getName().c_str());
                
                if (!module->loadConfig()) {
                    DEBUG_W("Failed to load config for module: %s", 
                            module->getName().c_str());
                }
                
                if (module->init()) {
                    module->setState(MODULE_INITIALIZING);
                    DEBUG_I("Module initialized: %s", module->getName().c_str());
                } else {
                    DEBUG_E("Failed to initialize module: %s", 
                            module->getName().c_str());
                    module->setState(MODULE_ERROR);
                }
            }
        }
        
        initialized = true;
        return true;
    }
    
    // Start all autostart modules
    bool startAll() {
        DEBUG_I("Starting autostart modules...");
        
        for (auto module : modules) {
            if (module->isEnabled() && module->isAutostart()) {
                DEBUG_I("Starting module: %s", module->getName().c_str());
                
                if (module->start()) {
                    module->setState(MODULE_RUNNING);
                    DEBUG_I("Module started: %s", module->getName().c_str());
                } else {
                    DEBUG_E("Failed to start module: %s", 
                            module->getName().c_str());
                    module->setState(MODULE_ERROR);
                }
            }
        }
        
        return true;
    }
    
    // Loop through all running modules
    void loopAll() {
        for (auto module : modules) {
            if (module->isRunning()) {
                module->loop();
            }
        }
    }
    
    // Find module by name
    ModuleBase* findModule(const String& name) {
        for (auto module : modules) {
            if (module->getName() == name) {
                return module;
            }
        }
        return nullptr;
    }
    
    // Start specific module
    bool startModule(const String& name) {
        ModuleBase* module = findModule(name);
        if (!module) {
            DEBUG_E("Module not found: %s", name.c_str());
            return false;
        }
        
        if (module->start()) {
            module->setState(MODULE_RUNNING);
            DEBUG_I("Module started: %s", name.c_str());
            return true;
        }
        
        DEBUG_E("Failed to start module: %s", name.c_str());
        module->setState(MODULE_ERROR);
        return false;
    }
    
    // Stop specific module
    bool stopModule(const String& name) {
        ModuleBase* module = findModule(name);
        if (!module) {
            DEBUG_E("Module not found: %s", name.c_str());
            return false;
        }
        
        if (module->stop()) {
            module->setState(MODULE_ENABLED);
            DEBUG_I("Module stopped: %s", name.c_str());
            return true;
        }
        
        DEBUG_E("Failed to stop module: %s", name.c_str());
        return false;
    }
    
    // Get all modules info as JSON
    String getModulesJson() {
        DynamicJsonDocument doc(4096);
        JsonArray array = doc.createNestedArray("modules");
        
        for (auto module : modules) {
            JsonObject obj = array.createNestedObject();
            obj["name"] = module->getName();
            obj["state"] = (int)module->getState();
            obj["priority"] = module->getPriority();
            obj["autostart"] = module->isAutostart();
            obj["debug"] = module->isDebugMode();
            obj["version"] = module->getVersion();
        }
        
        String output;
        serializeJson(doc, output);
        return output;
    }
    
    // Get module count
    size_t getModuleCount() const { return modules.size(); }
    
    // Get modules list
    const std::vector<ModuleBase*>& getModules() const { return modules; }
};

#endif // MODULE_MANAGER_H
