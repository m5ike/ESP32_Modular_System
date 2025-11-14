#include "ModuleManager.h"
#include "modules/CONTROL_FS.h"
#include <algorithm>

ModuleManager* ModuleManager::instance = nullptr;

// Module implementation
Module::Module(const char* name) : moduleName(name), state(MODULE_DISABLED), 
                                   priority(0), autoStart(false), 
                                   debugEnabled(false), version("1.0.0") {
    config = new DynamicJsonDocument(2048);
}

Module::~Module() {
    if (config) delete config;
}

bool Module::loadConfig(DynamicJsonDocument& doc) {
    if (doc.containsKey(moduleName)) {
        JsonObject modConfig = doc[moduleName];
        
        if (modConfig.containsKey("priority")) 
            priority = modConfig["priority"];
        if (modConfig.containsKey("autoStart")) 
            autoStart = modConfig["autoStart"];
        if (modConfig.containsKey("debug")) 
            debugEnabled = modConfig["debug"];
        if (modConfig.containsKey("version")) 
            version = modConfig["version"].as<String>();
        if (modConfig.containsKey("state")) {
            String stateStr = modConfig["state"];
            if (stateStr == "enabled") setState(MODULE_ENABLED);
            else if (stateStr == "disabled") setState(MODULE_DISABLED);
        }
        
        return true;
    }
    return false;
}

bool Module::saveConfig() {
    // Implemented by CONTROL_FS
    return true;
}

void Module::log(const String& message, const char* level) {
    String logMsg = String("[") + level + "][" + moduleName + "] " + message;
    Serial.println(logMsg);
    
    // Also log to file system if CONTROL_FS is available
    Module* fsModule = ModuleManager::getInstance()->getModule("CONTROL_FS");
    if (fsModule && fsModule->getState() == MODULE_ENABLED) {
<<<<<<< HEAD
        // TODO: Call FS module logging function
=======
        CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsModule);
        fs->writeLog(logMsg, level);
>>>>>>> de1429e (commit)
    }
}

// ModuleManager implementation
ModuleManager::ModuleManager() {
}

ModuleManager* ModuleManager::getInstance() {
    if (instance == nullptr) {
        instance = new ModuleManager();
    }
    return instance;
}

bool ModuleManager::registerModule(Module* module) {
    if (module == nullptr) return false;
    
    // Check if module already exists
    for (auto* mod : modules) {
        if (mod->getName() == module->getName()) {
            Serial.println("Module " + module->getName() + " already registered");
            return false;
        }
    }
    
    modules.push_back(module);
    Serial.println("Module " + module->getName() + " registered successfully");
    return true;
}

bool ModuleManager::unregisterModule(const String& name) {
    for (size_t i = 0; i < modules.size(); i++) {
        if (modules[i]->getName() == name) {
            modules[i]->stop();
            delete modules[i];
            modules.erase(modules.begin() + i);
            return true;
        }
    }
    return false;
}

Module* ModuleManager::getModule(const String& name) {
    for (auto* mod : modules) {
        if (mod->getName() == name) {
            return mod;
        }
    }
    return nullptr;
}

void ModuleManager::sortModulesByPriority() {
    std::sort(modules.begin(), modules.end(), 
              [](Module* a, Module* b) { 
                  return a->getPriority() > b->getPriority(); 
              });
}

bool ModuleManager::initModules() {
    sortModulesByPriority();
    
    Serial.println("Initializing modules...");
    for (auto* mod : modules) {
        Serial.println("Init: " + mod->getName());
        if (!mod->init()) {
            mod->setState(MODULE_ERROR);
            Serial.println("Failed to init: " + mod->getName());
            return false;
        }
    }
    return true;
}

bool ModuleManager::startModules() {
    Serial.println("Starting modules...");
    for (auto* mod : modules) {
        if (mod->isAutoStart() && mod->getState() == MODULE_ENABLED) {
            Serial.println("Starting: " + mod->getName());
            if (!mod->start()) {
                mod->setState(MODULE_ERROR);
                Serial.println("Failed to start: " + mod->getName());
            }
        }
    }
    return true;
}

bool ModuleManager::stopModules() {
    Serial.println("Stopping modules...");
    for (auto* mod : modules) {
        if (mod->getState() == MODULE_ENABLED) {
            Serial.println("Stopping: " + mod->getName());
            mod->stop();
        }
    }
    return true;
}

bool ModuleManager::updateModules() {
    for (auto* mod : modules) {
        if (mod->getState() == MODULE_ENABLED) {
            mod->update();
        }
    }
    return true;
}

bool ModuleManager::loadGlobalConfig() {
<<<<<<< HEAD
    // This will be implemented after CONTROL_FS is created
    return true;
}

bool ModuleManager::saveGlobalConfig() {
    // This will be implemented after CONTROL_FS is created
=======
    Module* fsMod = getModule("CONTROL_FS");
    if (!fsMod) return false;
    CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsMod);
    DynamicJsonDocument doc(8192);
    if (!fs->loadGlobalConfig(doc)) return false;
    return applyConfig(doc);
}

bool ModuleManager::saveGlobalConfig() {
    Module* fsMod = getModule("CONTROL_FS");
    if (!fsMod) return false;
    CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsMod);
    DynamicJsonDocument doc(8192);
    for (Module* mod : modules) {
        DynamicJsonDocument status = mod->getStatus();
        doc[mod->getName()] = status;
    }
    return fs->saveGlobalConfig(doc);
}

bool ModuleManager::applyConfig(DynamicJsonDocument& doc) {
    for (Module* mod : modules) {
        mod->loadConfig(doc);
    }
>>>>>>> de1429e (commit)
    return true;
}
