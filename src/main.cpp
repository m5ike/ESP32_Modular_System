#include <Arduino.h>
#include "Config.h"
#include "ModuleManager.h"

// Include all modules
#include "modules/CONTROL_FS.h"
#include "modules/CONTROL_LCD.h"
#include "modules/CONTROL_WIFI.h"
#include "modules/CONTROL_SERIAL.h"
#include "modules/CONTROL_WEB.h"
<<<<<<< HEAD
=======
#include "modules/CONTROL_RADAR.h"
>>>>>>> de1429e (commit)

// Global module manager
ModuleManager* moduleManager = nullptr;

// Module instances
CONTROL_FS* fsModule = nullptr;
CONTROL_LCD* lcdModule = nullptr;
CONTROL_WIFI* wifiModule = nullptr;
CONTROL_SERIAL* serialModule = nullptr;
CONTROL_WEB* webModule = nullptr;
<<<<<<< HEAD
=======
CONTROL_RADAR* radarModule = nullptr;
>>>>>>> de1429e (commit)

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
<<<<<<< HEAD
=======
    // Priority 50 - Ultrasonic Radar
    radarModule = new CONTROL_RADAR();
    moduleManager->registerModule(radarModule);
>>>>>>> de1429e (commit)
    
    DEBUG_I("Registered %d modules", moduleManager->getModules().size());
    
    // Initialize all modules
    DEBUG_I("Initializing modules...");
    if (!moduleManager->initModules()) {
        DEBUG_E("Failed to initialize modules!");
    }
    
    // Show status on LCD
    if (lcdModule && lcdModule->getState() == MODULE_ENABLED) {
        std::vector<String> lines;
        lines.push_back("Initialized");
        lcdModule->displayStatus("System", lines);
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
        std::vector<String> lines;
        lines.push_back("Ready");
        lcdModule->displayStatus("System", lines);
        
        // Display WiFi info
        if (wifiModule && wifiModule->getState() == MODULE_ENABLED) {
            String ip = wifiModule->getIP();
            lcdModule->drawText(10, 200, "WiFi: " + wifiModule->getSSID(), TFT_CYAN, 1);
            lcdModule->drawText(10, 215, "IP: " + ip, TFT_CYAN, 1);
        }
        
        // Display URL
        lcdModule->drawText(10, 240, "Web: http://192.168.4.1", TFT_YELLOW, 1);
    }
}

void loop() {
    // Run all module loops
    moduleManager->updateModules();
    
    // Small delay to prevent watchdog issues
    delay(10);
}
