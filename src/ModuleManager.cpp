#include "ModuleManager.h"
#include "modules/CONTROL_FS.h"
#include "modules/CONTROL_LCD.h"
#include "modules/CONTROL_WIFI.h"
#include "modules/CONTROL_WEB.h"
#include <algorithm>
#include "ModuleRegistry.h"
#include "FreeRTOSTypes.h"

ModuleManager* ModuleManager::instance = nullptr;

// Module implementation
Module::Module(const char* name) : moduleName(name), state(MODULE_DISABLED), 
                                   priority(0), autoStart(false), 
                                   debugEnabled(false), version("1.0.0") {
    config = new DynamicJsonDocument(2048);
    critical = false;
    taskBase = nullptr;
    queueBase = nullptr;
    useTask = true;
    useQueue = false;
    taskCfg = {String(name) + String("_TASK"), 4096, 3, nullptr, -1};
    queueCfg = {8, sizeof(QueueMessage*), portMAX_DELAY, pdMS_TO_TICKS(100), false};
}

Module::~Module() {
    if (config) delete config;
}

bool Module::loadConfig(DynamicJsonDocument& doc) {
    JsonObject modConfig;
    if (doc.containsKey(moduleName)) modConfig = doc[moduleName];
    else if (doc.containsKey("modules") && doc["modules"].containsKey(moduleName)) modConfig = doc["modules"][moduleName];
    if (!modConfig.isNull()) {
        if (modConfig.containsKey("priority")) priority = modConfig["priority"];
        if (modConfig.containsKey("autoStart")) autoStart = modConfig["autoStart"];
        if (modConfig.containsKey("debug")) debugEnabled = modConfig["debug"];
        if (modConfig.containsKey("version")) version = modConfig["version"].as<String>();
        if (modConfig.containsKey("state")) {
            String stateStr = modConfig["state"];
            if (stateStr == "enabled") setState(MODULE_ENABLED);
            else if (stateStr == "disabled") setState(MODULE_DISABLED);
        }
        if (modConfig.containsKey("critical")) critical = modConfig["critical"];
        if (modConfig.containsKey("freertos")) {
            JsonObject fr = modConfig["freertos"];
            if (fr.containsKey("task")) {
                JsonObject tk = fr["task"];
                if (tk.containsKey("name")) taskCfg.name = tk["name"].as<String>();
                if (tk.containsKey("stack")) taskCfg.stackSize = tk["stack"];
                if (tk.containsKey("priority")) taskCfg.priority = tk["priority"];
                if (tk.containsKey("core")) taskCfg.core = tk["core"];
                if (tk.containsKey("enabled")) useTask = tk["enabled"];
            }
            if (fr.containsKey("queue")) {
                JsonObject qj = fr["queue"];
                if (qj.containsKey("length")) queueCfg.length = qj["length"];
                queueCfg.itemSize = sizeof(QueueMessage*);
                if (qj.containsKey("send_timeout_ms")) queueCfg.sendTimeoutTicks = pdMS_TO_TICKS((int)qj["send_timeout_ms"]);
                if (qj.containsKey("recv_timeout_ms")) queueCfg.recvTimeoutTicks = pdMS_TO_TICKS((int)qj["recv_timeout_ms"]);
                if (qj.containsKey("enabled")) useQueue = qj["enabled"];
            }
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
        CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsModule);
        fs->writeLog(logMsg, level);
    }
    ModuleManager::getInstance()->appendLCDLog(logMsg);
}

// ModuleManager implementation
ModuleManager::ModuleManager() {
    wifiConnectedLast = false;
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
    int idx = 0;
    for (auto* mod : modules) {
        Serial.println("Init: " + mod->getName());
        int percent = (int)((idx * 100.0) / modules.size());
        renderLoadingStep(String("Init ")+mod->getName(), percent);
        if (!mod->init()) {
            mod->setState(MODULE_ERROR);
            Serial.println("Failed to init: " + mod->getName());
            if (mod->isCritical()) return false;
        }
        renderLoadingStep(String("Initialized ")+mod->getName(), percent);
        idx++;
    }
    renderLoadingStep("Init completed", 100);
    return true;
}

bool ModuleManager::startModules() {
    Serial.println("Starting modules...");
    int total = modules.size();
    int done = 0;
    for (auto* mod : modules) {
        if (mod->isAutoStart() && mod->getState() == MODULE_ENABLED) {
            Serial.println("Starting: " + mod->getName());
            done++;
            int percent = (int)((done * 100.0) / total);
            renderLoadingStep(String("Start ")+mod->getName(), percent);
            bool allowStart = true;
            if (mod->getName() == "CONTROL_WEB") {
                Module* wifiMod = getModule("CONTROL_WIFI");
                CONTROL_WIFI* wf = wifiMod ? static_cast<CONTROL_WIFI*>(wifiMod) : nullptr;
                if (wf && !wf->isWiFiConnected()) allowStart = false;
            }
            if (allowStart && !mod->start()) {
                mod->setState(MODULE_ERROR);
                Serial.println("Failed to start: " + mod->getName());
                if (mod->isCritical()) return false;
            }
            ensureModuleQueue(mod);
            startModuleTask(mod);
            renderLoadingStep(String("Started ")+mod->getName(), percent);
        }
    }
    renderLoadingStep("Start completed", 100);
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
    Module* wifiModule = getModule("CONTROL_WIFI");
    Module* webModule = getModule("CONTROL_WEB");
    if (wifiModule) {
        bool connected = false;
        CONTROL_WIFI* wf = static_cast<CONTROL_WIFI*>(wifiModule);
        connected = wf->isWiFiConnected();
        if (connected != wifiConnectedLast) {
            wifiConnectedLast = connected;
            if (webModule) {
                CONTROL_WEB* web = static_cast<CONTROL_WEB*>(webModule);
                if (connected) {
                    if (!web->isRunning()) web->start();
                    if (web->getTask()) web->getTask()->resume();
                } else {
                    if (web->isRunning()) web->stop();
                    if (web->getTask()) web->getTask()->suspend();
                }
            }
        }
    }
    for (auto* mod : modules) {
        if (mod->getState() == MODULE_ENABLED) {
            mod->update();
        }
    }
    return true;
}

bool ModuleManager::startModuleTask(Module* mod) {
    if (!mod->getUseTask()) return true;
    if (mod->getTask()) return true;
    TaskBase* tb = new TaskBase(mod, mod->getTaskConfig());
    auto runner = [](void* pv) {
      Module* m = static_cast<Module*>(pv);
      for (;;) {
        if (m->getState() == MODULE_ENABLED) m->update();
        vTaskDelay(pdMS_TO_TICKS(10));
      }
    };
    bool ok = tb->start(runner);
    if (ok) {
      mod->attachTask(tb);
      ModuleRegistry::getInstance()->registerTask(mod->getName(), tb->handle());
    }
    return ok;
}

bool ModuleManager::ensureModuleQueue(Module* mod) {
    if (!mod->getUseQueue()) return true;
    if (mod->getQueue()) return true;
    QueueBase* qb = new QueueBase(mod, mod->getQueueConfig());
    bool ok = qb->create();
    if (ok) mod->attachQueue(qb);
    return ok;
}

bool ModuleManager::loadGlobalConfig() {
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
    return true;
}

void ModuleManager::appendLCDLog(const String& line) {
    lcdLogs.push_back(line);
    while (lcdLogs.size() > 5) lcdLogs.erase(lcdLogs.begin());
    Module* lcdMod = getModule("CONTROL_LCD");
    if (!lcdMod) return;
    CONTROL_LCD* lcd = static_cast<CONTROL_LCD*>(lcdMod);
    if (!lcd || lcd->getState() != MODULE_ENABLED) return;
    TFT_eSPI* tft = lcd->getDisplay();
    if (!tft) return;
    int16_t yStart = LCD_HEIGHT - 70;
    tft->fillRect(0, yStart, LCD_WIDTH, 70, TFT_BLACK);
    tft->setTextColor(TFT_WHITE);
    tft->setTextSize(1);
    int16_t y = yStart + 4;
    for (const String& s : lcdLogs) {
        tft->setCursor(4, y);
        tft->print(s);
        y += 12;
    }
}

void ModuleManager::renderLoadingStep(const String& op, int percent) {
    Module* lcdMod = getModule("CONTROL_LCD");
    if (!lcdMod) return;
    CONTROL_LCD* lcd = static_cast<CONTROL_LCD*>(lcdMod);
    if (!lcd || lcd->getState() != MODULE_ENABLED) return;
    TFT_eSPI* tft = lcd->getDisplay();
    if (!tft) return;
    tft->fillRect(0, 0, LCD_WIDTH, 40, TFT_BLACK);
    lcd->drawCenteredText(18, "ESP32 Modular System", TFT_CYAN, 2);
    tft->fillRect(0, 60, LCD_WIDTH, 180, TFT_BLACK);
    lcd->drawCenteredText(120, op, TFT_WHITE, 2);
    lcd->drawProgressBar(20, LCD_HEIGHT - 90, LCD_WIDTH - 40, 16, percent, TFT_GREEN);
    appendLCDLog(String("[") + "INFO" + "][BOOT] " + op);
}
