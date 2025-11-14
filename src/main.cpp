#include <Arduino.h>
#include "Config.h"
#include "ModuleManager.h"

// Include all modules
#include "modules/CONTROL_FS.h"
#include "modules/CONTROL_LCD.h"
#include "modules/CONTROL_WIFI.h"
#include "modules/CONTROL_SERIAL.h"
#include "modules/CONTROL_WEB.h"
#include "modules/CONTROL_RADAR.h"

// Global module manager
ModuleManager* moduleManager = nullptr;

// Module instances
CONTROL_FS* fsModule = nullptr;
CONTROL_LCD* lcdModule = nullptr;
CONTROL_WIFI* wifiModule = nullptr;
CONTROL_SERIAL* serialModule = nullptr;
CONTROL_WEB* webModule = nullptr;
CONTROL_RADAR* radarModule = nullptr;

void setup() {
    // Initialize Serial first
    Serial.begin(SERIAL_BAUD);
    delay(1000);
    
    DEBUG_I("===========================================");
    DEBUG_I("ESP32 Modular System v1.0.0");
    DEBUG_I("===========================================");
    DEBUG_I("Chip: %s Rev %d", ESP.getChipModel(), ESP.getChipRevision());
    DEBUG_I("CPU Freq: %d MHz", ESP.getCpuFreqMHz());
    DEBUG_I("Free Heap: %d bytes", ESP.getFreeHeap());
    DEBUG_I("===========================================");
    
    // Get module manager instance
    moduleManager = ModuleManager::getInstance();
    
    // Create and register modules in priority order
    
    // Priority 100 - File System (must be first)
    fsModule = new CONTROL_FS();
    moduleManager->registerModule(fsModule);
    
    // Priority 90 - WiFi
    wifiModule = new CONTROL_WIFI();
    moduleManager->registerModule(wifiModule);
    
    wifiModule->setSSID("MikroTik-DDDB7E");
    wifiModule->setPassword("XVCI62P893M5");
    
    // Priority 85 - LCD
    lcdModule = new CONTROL_LCD();
    moduleManager->registerModule(lcdModule);
    
    // Priority 80 - Serial Console
    serialModule = new CONTROL_SERIAL();
    moduleManager->registerModule(serialModule);
    
    // Priority 75 - Web Server
    webModule = new CONTROL_WEB();
    moduleManager->registerModule(webModule);
    // Priority 50 - Ultrasonic Radar
    radarModule = new CONTROL_RADAR();
    moduleManager->registerModule(radarModule);
    
    DEBUG_I("Registered %d modules", moduleManager->getModules().size());
    
    // Initialize all modules
    DEBUG_I("Initializing modules...");
    if (!moduleManager->initModules()) {
        DEBUG_E("Failed to initialize modules!");
    }
    
    // Show status on LCD
    if (lcdModule && lcdModule->getState() == MODULE_ENABLED) {
        QueueBase* qb = lcdModule->getQueue();
        if (qb) {
            DynamicJsonDocument* vars = new DynamicJsonDocument(256);
            (*vars)["title"] = String("System");
            JsonArray arr = (*vars)["lines"].to<JsonArray>();
            arr.add("Initialized");
            QueueMessage* msg = new QueueMessage{genUUID4(), lcdModule->getName(), String("main"), EVENT_DATA_READY, CALL_FUNCTION_ASYNC, String("lcd_status"), vars};
            qb->send(msg);
        }
    }
    
    // Start all autostart modules
    DEBUG_I("Starting autostart modules...");
    if (!moduleManager->startModules()) {
        DEBUG_E("Failed to start modules!");
    }
    
    // Show final status
    DEBUG_I("===========================================");
    DEBUG_I("System Ready!");
    DEBUG_I("===========================================");
    
    if (lcdModule && lcdModule->getState() == MODULE_ENABLED) {
        QueueBase* qb = lcdModule->getQueue();
        if (qb) {
            DynamicJsonDocument* v1 = new DynamicJsonDocument(256);
            (*v1)["title"] = String("System");
            JsonArray arr1 = (*v1)["lines"].to<JsonArray>();
            arr1.add("Ready");
            QueueMessage* m1 = new QueueMessage{genUUID4(), lcdModule->getName(), String("main"), EVENT_DATA_READY, CALL_FUNCTION_ASYNC, String("lcd_status"), v1};
            qb->send(m1);
            if (wifiModule && wifiModule->getState() == MODULE_ENABLED) {
                String ip = wifiModule->getIP();
                DynamicJsonDocument* v2 = new DynamicJsonDocument(256);
                (*v2)["x"] = 10;
                (*v2)["y"] = 200;
                (*v2)["text"] = String("WiFi: ") + wifiModule->getSSID();
                (*v2)["color"] = (uint16_t)TFT_CYAN;
                QueueMessage* m2 = new QueueMessage{genUUID4(), lcdModule->getName(), String("main"), EVENT_DATA_READY, CALL_FUNCTION_ASYNC, String("lcd_text"), v2};
                qb->send(m2);
                DynamicJsonDocument* v3 = new DynamicJsonDocument(256);
                (*v3)["x"] = 10;
                (*v3)["y"] = 215;
                (*v3)["text"] = String("IP: ") + ip;
                (*v3)["color"] = (uint16_t)TFT_CYAN;
                QueueMessage* m3 = new QueueMessage{genUUID4(), lcdModule->getName(), String("main"), EVENT_DATA_READY, CALL_FUNCTION_ASYNC, String("lcd_text"), v3};
                qb->send(m3);
            }
            DynamicJsonDocument* v4 = new DynamicJsonDocument(256);
            (*v4)["x"] = 10;
            (*v4)["y"] = 240;
            (*v4)["text"] = String("Web: http://192.168.4.1");
            (*v4)["color"] = (uint16_t)TFT_YELLOW;
            QueueMessage* m4 = new QueueMessage{genUUID4(), lcdModule->getName(), String("main"), EVENT_DATA_READY, CALL_FUNCTION_ASYNC, String("lcd_text"), v4};
            qb->send(m4);
        }
    }
}

void loop() {
    // Run all module loops
    moduleManager->updateModules();
    
    // Small delay to prevent watchdog issues
    delay(10);
}
