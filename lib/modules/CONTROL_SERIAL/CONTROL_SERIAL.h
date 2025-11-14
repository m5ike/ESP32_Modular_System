#ifndef CONTROL_SERIAL_H
#define CONTROL_SERIAL_H

#include "ModuleBase.h"

// Forward declaration
class ModuleManager;

class CONTROL_SERIAL : public ModuleBase {
private:
    String inputBuffer;
    ModuleManager* moduleManager;
    bool echoEnabled;
    
    void processCommand(const String& cmd) {
        String command = cmd;
        command.trim();
        command.toLowerCase();
        
        if (command.length() == 0) return;
        
        if (debugMode) {
            logf("INFO", "Command received: %s", command.c_str());
        }
        
        // Parse command
        int spaceIndex = command.indexOf(' ');
        String mainCmd = spaceIndex > 0 ? command.substring(0, spaceIndex) : command;
        String args = spaceIndex > 0 ? command.substring(spaceIndex + 1) : "";
        
        // Handle commands
        if (mainCmd == "help") {
            printHelp();
        }
        else if (mainCmd == "status") {
            printStatus();
        }
        else if (mainCmd == "modules") {
            listModules();
        }
        else if (mainCmd == "start") {
            startModule(args);
        }
        else if (mainCmd == "stop") {
            stopModule(args);
        }
        else if (mainCmd == "restart") {
            restartModule(args);
        }
        else if (mainCmd == "config") {
            showConfig(args);
        }
        else if (mainCmd == "debug") {
            setDebug(args);
        }
        else if (mainCmd == "clear") {
            Serial.write(27);       // ESC
            Serial.print("[2J");    // Clear screen
            Serial.write(27);       // ESC
            Serial.print("[H");     // Move cursor to home
        }
        else if (mainCmd == "echo") {
            echoEnabled = !echoEnabled;
            Serial.printf("Echo %s\n", echoEnabled ? "ON" : "OFF");
        }
        else if (mainCmd == "reboot") {
            Serial.println("Rebooting...");
            delay(1000);
            ESP.restart();
        }
        else {
            Serial.println("Unknown command. Type 'help' for available commands.");
        }
        
        Serial.print("\n> ");
    }
    
    void printHelp() {
        Serial.println("\n=== ESP32 Modular System - Command List ===");
        Serial.println("help                 - Show this help");
        Serial.println("status               - Show system status");
        Serial.println("modules              - List all modules");
        Serial.println("start <module>       - Start a module");
        Serial.println("stop <module>        - Stop a module");
        Serial.println("restart <module>     - Restart a module");
        Serial.println("config <module>      - Show module configuration");
        Serial.println("debug <module> on/off - Enable/disable module debug");
        Serial.println("clear                - Clear screen");
        Serial.println("echo                 - Toggle echo on/off");
        Serial.println("reboot               - Reboot system");
        Serial.println("==========================================");
    }
    
    void printStatus() {
        Serial.println("\n=== System Status ===");
        Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
        Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
        Serial.printf("Chip Model: %s\n", ESP.getChipModel());
        Serial.printf("Chip Revision: %d\n", ESP.getChipRevision());
        Serial.printf("CPU Freq: %d MHz\n", ESP.getCpuFreqMHz());
        Serial.printf("Flash Size: %d bytes\n", ESP.getFlashChipSize());
        Serial.println("====================");
    }
    
    void listModules();
    void startModule(const String& name);
    void stopModule(const String& name);
    void restartModule(const String& name);
    void showConfig(const String& name);
    void setDebug(const String& args);
    
public:
    CONTROL_SERIAL() : ModuleBase("CONTROL_SERIAL"), 
                       moduleManager(nullptr),
                       echoEnabled(true) {
        priority = 80;
        autostart = true;
    }
    
    void setModuleManager(ModuleManager* mgr) {
        moduleManager = mgr;
    }
    
    bool init() override {
        log("INFO", "Initializing Serial module...");
        
        Serial.begin(SERIAL_BAUD);
        delay(100);
        
        Serial.println("\n\n=================================");
        Serial.println("ESP32 Modular System v1.0.0");
        Serial.println("=================================");
        Serial.println("Type 'help' for available commands");
        Serial.print("\n> ");
        
        return true;
    }
    
    bool start() override {
        log("INFO", "Starting Serial module...");
        return true;
    }
    
    bool stop() override {
        log("INFO", "Stopping Serial module...");
        return true;
    }
    
    bool test() override {
        log("INFO", "Testing Serial module...");
        
        if (!Serial) {
            log("ERROR", "Serial not available");
            return false;
        }
        
        log("INFO", "Serial test passed");
        return true;
    }
    
    void loop() override {
        while (Serial.available()) {
            char c = Serial.read();
            
            if (c == '\n' || c == '\r') {
                if (inputBuffer.length() > 0) {
                    Serial.println(); // New line
                    processCommand(inputBuffer);
                    inputBuffer = "";
                }
            }
            else if (c == '\b' || c == 127) { // Backspace
                if (inputBuffer.length() > 0) {
                    inputBuffer.remove(inputBuffer.length() - 1);
                    if (echoEnabled) {
                        Serial.write('\b');
                        Serial.write(' ');
                        Serial.write('\b');
                    }
                }
            }
            else if (c >= 32 && c <= 126) { // Printable characters
                inputBuffer += c;
                if (echoEnabled) {
                    Serial.write(c);
                }
            }
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
        
        echoEnabled = (*config)["echo"] | true;
        
        log("INFO", "Config loaded successfully");
        return true;
    }
    
    bool saveConfig() override {
        (*config)["baud"] = SERIAL_BAUD;
        (*config)["echo"] = echoEnabled;
        
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
};

// Implementation of methods that need ModuleManager
void CONTROL_SERIAL::listModules() {
    if (!moduleManager) {
        Serial.println("Module manager not available");
        return;
    }
    
    Serial.println("\n=== Modules ===");
    Serial.println("Name                State    Priority  Autostart  Debug");
    Serial.println("--------------------------------------------------------------");
    
    // This will be implemented in main.cpp since we need access to ModuleManager
}

void CONTROL_SERIAL::startModule(const String& name) {
    if (!moduleManager || name.length() == 0) {
        Serial.println("Invalid module name or manager not available");
        return;
    }
    
    Serial.printf("Starting module: %s\n", name.c_str());
    // Implementation in main.cpp
}

void CONTROL_SERIAL::stopModule(const String& name) {
    if (!moduleManager || name.length() == 0) {
        Serial.println("Invalid module name or manager not available");
        return;
    }
    
    Serial.printf("Stopping module: %s\n", name.c_str());
    // Implementation in main.cpp
}

void CONTROL_SERIAL::restartModule(const String& name) {
    stopModule(name);
    delay(500);
    startModule(name);
}

void CONTROL_SERIAL::showConfig(const String& name) {
    if (!moduleManager || name.length() == 0) {
        Serial.println("Invalid module name or manager not available");
        return;
    }
    
    Serial.printf("Configuration for: %s\n", name.c_str());
    // Implementation in main.cpp
}

void CONTROL_SERIAL::setDebug(const String& args) {
    int spaceIndex = args.indexOf(' ');
    if (spaceIndex < 0) {
        Serial.println("Usage: debug <module> on/off");
        return;
    }
    
    String moduleName = args.substring(0, spaceIndex);
    String state = args.substring(spaceIndex + 1);
    state.trim();
    state.toLowerCase();
    
    if (!moduleManager) {
        Serial.println("Module manager not available");
        return;
    }
    
    Serial.printf("Setting debug for %s: %s\n", moduleName.c_str(), state.c_str());
    // Implementation in main.cpp
}

#endif // CONTROL_SERIAL_H
