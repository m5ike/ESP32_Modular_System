#include "CONTROL_SERIAL.h"
#include "CONTROL_FS.h"

CONTROL_SERIAL::CONTROL_SERIAL() : Module("CONTROL_SERIAL") {
    bufferIndex = 0;
    serialInitialized = false;
    memset(inputBuffer, 0, SERIAL_BUFFER_SIZE);
    priority = 80;
    autoStart = true;
    version = "1.0.0";
}

CONTROL_SERIAL::~CONTROL_SERIAL() {
    stop();
}

bool CONTROL_SERIAL::init() {
    log("Initializing serial control...");
    
    if (!Serial) {
        Serial.begin(115200);
        delay(100);
    }
    
    serialInitialized = true;
    setState(MODULE_ENABLED);
    
    log("Serial control initialized");
    printPrompt();
    
    return true;
}

bool CONTROL_SERIAL::start() {
    if (!serialInitialized) {
        return init();
    }
    
    setState(MODULE_ENABLED);
    log("Serial control started");
    printPrompt();
    return true;
}

bool CONTROL_SERIAL::stop() {
    serialInitialized = false;
    setState(MODULE_DISABLED);
    log("Serial control stopped");
    return true;
}

bool CONTROL_SERIAL::update() {
    if (state != MODULE_ENABLED) return true;
    
    processSerial();
    return true;
}

bool CONTROL_SERIAL::test() {
    log("Testing serial control...");
    
    if (!Serial) {
        log("Serial not available", "ERROR");
        return false;
    }
    
    Serial.println("Serial test - OK");
    log("Serial control test passed");
    return true;
}

DynamicJsonDocument CONTROL_SERIAL::getStatus() {
    DynamicJsonDocument doc(1024);
    
    doc["module"] = moduleName;
    doc["state"] = state == MODULE_ENABLED ? "enabled" : "disabled";
    doc["version"] = version;
    doc["priority"] = priority;
    doc["autoStart"] = autoStart;
    doc["debug"] = debugEnabled;
    doc["initialized"] = serialInitialized;
    
    return doc;
}

void CONTROL_SERIAL::processSerial() {
    while (Serial.available() > 0) {
        char c = Serial.read();
        
        if (c == '\n' || c == '\r') {
            if (bufferIndex > 0) {
                inputBuffer[bufferIndex] = '\0';
                String command = String(inputBuffer);
                command.trim();
                
                if (command.length() > 0) {
                    processCommand(command);
                }
                
                bufferIndex = 0;
                memset(inputBuffer, 0, SERIAL_BUFFER_SIZE);
                printPrompt();
            }
        } else if (c == 8 || c == 127) { // Backspace
            if (bufferIndex > 0) {
                bufferIndex--;
                Serial.print("\b \b");
            }
        } else if (bufferIndex < SERIAL_BUFFER_SIZE - 1) {
            inputBuffer[bufferIndex++] = c;
            Serial.print(c);
        }
    }
}

void CONTROL_SERIAL::processCommand(const String& command) {
    Serial.println(); // New line after command
    
    String cmd = command;
    cmd.toLowerCase();
    cmd.trim();
    
    if (cmd == "help" || cmd == "?") {
        cmdHelp();
    }
    else if (cmd == "status") {
        cmdStatus();
    }
    else if (cmd == "modules") {
        cmdModules();
    }
    else if (cmd.startsWith("module ")) {
        String moduleName = command.substring(7);
        moduleName.trim();
        cmdModuleInfo(moduleName);
    }
    else if (cmd.startsWith("start ")) {
        String moduleName = command.substring(6);
        moduleName.trim();
        cmdModuleStart(moduleName);
    }
    else if (cmd.startsWith("stop ")) {
        String moduleName = command.substring(5);
        moduleName.trim();
        cmdModuleStop(moduleName);
    }
    else if (cmd.startsWith("test ")) {
        String moduleName = command.substring(5);
        moduleName.trim();
        cmdModuleTest(moduleName);
    }
    else if (cmd.startsWith("config ")) {
        String moduleName = command.substring(7);
        moduleName.trim();
        cmdModuleConfig(moduleName);
    }
    else if (cmd.startsWith("logs")) {
        int lines = 20;
        if (cmd.length() > 5) {
            lines = command.substring(5).toInt();
        }
        cmdLogs(lines);
    }
    else if (cmd == "clearlogs") {
        cmdClearLogs();
    }
    else if (cmd == "restart") {
        cmdRestart();
    }
    else if (cmd == "clear") {
        Serial.print("\033[2J\033[H"); // Clear screen
    }
    else {
        Serial.println("Unknown command: " + command);
        Serial.println("Type 'help' for available commands");
    }
}

void CONTROL_SERIAL::printHelp() {
    Serial.println("\n========================================");
    Serial.println("ESP32 Serial Control - Commands");
    Serial.println("========================================");
    Serial.println("help, ?           - Show this help");
    Serial.println("status            - Show system status");
    Serial.println("modules           - List all modules");
    Serial.println("module <name>     - Show module info");
    Serial.println("start <name>      - Start module");
    Serial.println("stop <name>       - Stop module");
    Serial.println("test <name>       - Test module");
    Serial.println("config <name>     - Show module config");
    Serial.println("logs [n]          - Show last n log lines (default: 20)");
    Serial.println("clearlogs         - Clear all logs");
    Serial.println("restart           - Restart system");
    Serial.println("clear             - Clear screen");
    Serial.println("========================================\n");
}

void CONTROL_SERIAL::printPrompt() {
    Serial.print("\nESP32> ");
}

void CONTROL_SERIAL::cmdStatus() {
    Serial.println("\n========== System Status ==========");
    Serial.print("Uptime: ");
    Serial.print(millis() / 1000);
    Serial.println(" seconds");
    Serial.print("Free Heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
    Serial.print("Chip Model: ");
    Serial.println(ESP.getChipModel());
    Serial.print("CPU Freq: ");
    Serial.print(ESP.getCpuFreqMHz());
    Serial.println(" MHz");
    Serial.println("===================================");
}

void CONTROL_SERIAL::cmdModules() {
    Serial.println("\n========== Modules ==========");
    
    auto modules = ModuleManager::getInstance()->getModules();
    for (Module* mod : modules) {
        Serial.print(mod->getName());
        Serial.print(" - ");
        Serial.print(mod->getState() == MODULE_ENABLED ? "ENABLED" : "DISABLED");
        Serial.print(" (Priority: ");
        Serial.print(mod->getPriority());
        Serial.println(")");
    }
    
    Serial.println("=============================");
}

void CONTROL_SERIAL::cmdModuleInfo(const String& moduleName) {
    Module* mod = ModuleManager::getInstance()->getModule(moduleName);
    
    if (!mod) {
        Serial.println("Module not found: " + moduleName);
        return;
    }
    
    Serial.println("\n========== Module Info ==========");
    DynamicJsonDocument status = mod->getStatus();
    serializeJsonPretty(status, Serial);
    Serial.println("\n=================================");
}

void CONTROL_SERIAL::cmdModuleStart(const String& moduleName) {
    Module* mod = ModuleManager::getInstance()->getModule(moduleName);
    
    if (!mod) {
        Serial.println("Module not found: " + moduleName);
        return;
    }
    
    Serial.println("Starting module: " + moduleName);
    if (mod->start()) {
        Serial.println("Module started successfully");
    } else {
        Serial.println("Failed to start module");
    }
}

void CONTROL_SERIAL::cmdModuleStop(const String& moduleName) {
    Module* mod = ModuleManager::getInstance()->getModule(moduleName);
    
    if (!mod) {
        Serial.println("Module not found: " + moduleName);
        return;
    }
    
    Serial.println("Stopping module: " + moduleName);
    if (mod->stop()) {
        Serial.println("Module stopped successfully");
    } else {
        Serial.println("Failed to stop module");
    }
}

void CONTROL_SERIAL::cmdModuleTest(const String& moduleName) {
    Module* mod = ModuleManager::getInstance()->getModule(moduleName);
    
    if (!mod) {
        Serial.println("Module not found: " + moduleName);
        return;
    }
    
    Serial.println("Testing module: " + moduleName);
    if (mod->test()) {
        Serial.println("Module test PASSED");
    } else {
        Serial.println("Module test FAILED");
    }
}

void CONTROL_SERIAL::cmdModuleConfig(const String& moduleName) {
    cmdModuleInfo(moduleName); // Same as info for now
}

void CONTROL_SERIAL::cmdLogs(int lines) {
    Module* fsModule = ModuleManager::getInstance()->getModule("CONTROL_FS");
    
    if (!fsModule) {
        Serial.println("FS module not available");
        return;
    }
    
    CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsModule);
    String logs = fs->readLogs(lines);
    
    Serial.println("\n========== Logs ==========");
    Serial.print(logs);
    Serial.println("==========================");
}

void CONTROL_SERIAL::cmdClearLogs() {
    Module* fsModule = ModuleManager::getInstance()->getModule("CONTROL_FS");
    
    if (!fsModule) {
        Serial.println("FS module not available");
        return;
    }
    
    CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsModule);
    if (fs->clearLogs()) {
        Serial.println("Logs cleared successfully");
    } else {
        Serial.println("Failed to clear logs");
    }
}

void CONTROL_SERIAL::cmdRestart() {
    Serial.println("\nRestarting system in 3 seconds...");
    delay(1000);
    Serial.println("2...");
    delay(1000);
    Serial.println("1...");
    delay(1000);
    ESP.restart();
}

void CONTROL_SERIAL::cmdHelp() {
    printHelp();
}

void CONTROL_SERIAL::println(const String& message) {
    Serial.println(message);
}

void CONTROL_SERIAL::print(const String& message) {
    Serial.print(message);
}
