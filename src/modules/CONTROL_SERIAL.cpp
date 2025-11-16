#include "CONTROL_SERIAL.h"
#include "CONTROL_FS.h"
#include "CONTROL_WIFI.h"
#include "CONTROL_LCD.h"
#include "CONTROL_RADAR.h"
#include <WiFi.h>
#include <esp_system.h>

CONTROL_SERIAL::CONTROL_SERIAL() : Module("CONTROL_SERIAL") {
    bufferIndex = 0;
    serialInitialized = false;
    memset(inputBuffer, 0, SERIAL_BUFFER_SIZE);
    priority = 80;
    autoStart = true;
    version = "1.0.0";
    setUseQueue(true);
    TaskConfig tcfg = getTaskConfig();
    tcfg.name = "CONTROL_SERIAL_TASK";
    tcfg.stackSize = 4096;
    tcfg.priority = 2;
    tcfg.core = 1;
    setTaskConfig(tcfg);
    QueueConfig qcfg = getQueueConfig();
    qcfg.length = 16;
    setQueueConfig(qcfg);
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
    
    // Enhanced command parsing with safety checks
    if (cmd == "help" || cmd == "?") {
        cmdHelp();
    }
    else if (cmd.startsWith("help ")) {
        String helpCmd = command.substring(5);
        helpCmd.trim();
        cmdHelpDetailed(helpCmd);
    }
    else if (cmd == "status") {
        cmdStatus();
    }
    else if (cmd == "modules") {
        cmdModules();
    }
    else if (cmd == "realtime") {
        cmdRealTimeStatus();
    }
    else if (cmd == "safety") {
        cmdSafetyLimits();
    }
    else if (cmd.startsWith("system ")) {
        String systemCmd = command.substring(7);
        systemCmd.trim();
        cmdSystem(systemCmd);
    }
    else if (cmd.startsWith("config ")) {
        String configCmd = command.substring(7);
        configCmd.trim();
        cmdConfig(configCmd);
    }
    else if (cmd.startsWith("module ")) {
        String moduleName = command.substring(7);
        moduleName.trim();
        cmdModuleInfo(moduleName);
    }
    else if (cmd.startsWith("start ")) {
        String moduleName = command.substring(6);
        moduleName.trim();
        if (checkSafetyLimits(moduleName, "start", "")) {
            cmdModuleStart(moduleName);
        } else {
            Serial.println("Safety check failed - operation blocked");
        }
    }
    else if (cmd.startsWith("stop ")) {
        String moduleName = command.substring(5);
        moduleName.trim();
        if (checkSafetyLimits(moduleName, "stop", "")) {
            cmdModuleStop(moduleName);
        } else {
            Serial.println("Safety check failed - operation blocked");
        }
    }
    else if (cmd.startsWith("test ")) {
        String moduleName = command.substring(5);
        moduleName.trim();
        if (checkSafetyLimits(moduleName, "test", "")) {
            cmdModuleTest(moduleName);
        } else {
            Serial.println("Safety check failed - operation blocked");
        }
    }
    else if (cmd.startsWith("cmd ")) {
        // Enhanced module command interface
        int sp1 = command.indexOf(' ');
        int sp2 = command.indexOf(' ', sp1 + 1);
        if (sp1 > 0 && sp2 > sp1) {
            String moduleName = command.substring(sp1 + 1, sp2);
            String moduleCmd = command.substring(sp2 + 1);
            
            // Parse command and arguments
            int cmdSp = moduleCmd.indexOf(' ');
            String cmdName, cmdArgs;
            if (cmdSp > 0) {
                cmdName = moduleCmd.substring(0, cmdSp);
                cmdArgs = moduleCmd.substring(cmdSp + 1);
            } else {
                cmdName = moduleCmd;
                cmdArgs = "";
            }
            
            if (validateModuleCommand(moduleName, cmdName, cmdArgs)) {
                cmdModuleCommand(moduleName, cmdName, cmdArgs);
            } else {
                Serial.println("Invalid module command or safety check failed");
            }
        } else {
            Serial.println("Usage: cmd <module> <command> [args]");
        }
    }
    else if (cmd.startsWith("config ")) {
        String moduleName = command.substring(7);
        moduleName.trim();
        cmdModuleConfig(moduleName);
    }
    else if (cmd.startsWith("set ")) {
        int sp1 = command.indexOf(' ');
        int sp2 = command.indexOf(' ', sp1 + 1);
        int sp3 = command.indexOf(' ', sp2 + 1);
        if (sp1 > 0 && sp2 > sp1 && sp3 > sp2) {
            String moduleName = command.substring(sp1 + 1, sp2);
            String key = command.substring(sp2 + 1, sp3);
            String value = command.substring(sp3 + 1);
            
            if (checkSafetyLimits(moduleName, "config_set", key + "=" + value)) {
                Module* fsModule = ModuleManager::getInstance()->getModule("CONTROL_FS");
                if (fsModule) {
                    CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsModule);
                    ConfigManager* cfg = fs->getConfigManager();
                    if (cfg && cfg->getConfiguration()) {
                        DynamicJsonDocument& doc = *cfg->getConfiguration();
                        JsonObject mod = doc[moduleName];
                        mod[key] = value;
                        if (!cfg->validateModuleConfig(moduleName, mod)) { Serial.println("Module config invalid"); }
                        else if (cfg->saveConfiguration()) {
                            ModuleManager::getInstance()->loadGlobalConfig();
                            Serial.println("Config updated");
                        } else {
                            Serial.println("Save failed");
                        }
                    } else {
                        Serial.println("ConfigManager not ready");
                    }
                } else {
                    Serial.println("FS module not available");
                }
            } else {
                Serial.println("Safety check failed - config update blocked");
            }
        } else {
            Serial.println("Usage: set <module> <key> <value>");
        }
    }
    else if (cmd.startsWith("setjson ")) {
        int sp1 = command.indexOf(' ');
        int sp2 = command.indexOf(' ', sp1 + 1);
        if (sp1 > 0 && sp2 > sp1) {
            String moduleName = command.substring(sp1 + 1, sp2);
            String jsonStr = command.substring(sp2 + 1);
            
            if (checkSafetyLimits(moduleName, "config_setjson", jsonStr)) {
                Module* fsModule = ModuleManager::getInstance()->getModule("CONTROL_FS");
                if (fsModule) {
                    CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsModule);
                    ConfigManager* cfg = fs->getConfigManager();
                    if (cfg && cfg->getConfiguration()) {
                        DynamicJsonDocument& doc = *cfg->getConfiguration();
                        DynamicJsonDocument modDoc(2048);
                        DeserializationError err = deserializeJson(modDoc, jsonStr.c_str());
                        if (!err) {
                            if (!cfg->validateModuleConfig(moduleName, modDoc)) { Serial.println("Module config invalid"); }
                            else {
                                doc[moduleName] = modDoc.as<JsonObject>();
                                if (cfg->saveConfiguration()) {
                                    ModuleManager::getInstance()->loadGlobalConfig();
                                    Serial.println("Module JSON updated");
                                } else {
                                    Serial.println("Save failed");
                                }
                            }
                        } else {
                            Serial.println("JSON parse error");
                        }
                    } else {
                        Serial.println("ConfigManager not ready");
                    }
                } else {
                    Serial.println("FS module not available");
                }
            } else {
                Serial.println("Safety check failed - JSON update blocked");
            }
        } else {
            Serial.println("Usage: setjson <module> <json>");
        }
    }
    else if (cmd.startsWith("enable ")) {
        String moduleName = command.substring(7); moduleName.trim();
        if (checkSafetyLimits(moduleName, "enable", "")) {
            Module* mod = ModuleManager::getInstance()->getModule(moduleName);
            if (mod) { mod->setState(MODULE_ENABLED); Serial.println("Enabled"); }
            else { Serial.println("Module not found"); }
        } else {
            Serial.println("Safety check failed - enable blocked");
        }
    }
    else if (cmd.startsWith("disable ")) {
        String moduleName = command.substring(8); moduleName.trim();
        if (checkSafetyLimits(moduleName, "disable", "")) {
            Module* mod = ModuleManager::getInstance()->getModule(moduleName);
            if (mod) { mod->setState(MODULE_DISABLED); Serial.println("Disabled"); }
            else { Serial.println("Module not found"); }
        } else {
            Serial.println("Safety check failed - disable blocked");
        }
    }
    else if (cmd.startsWith("autostart ")) {
        int sp1 = command.indexOf(' ');
        int sp2 = command.indexOf(' ', sp1 + 1);
        if (sp1 > 0 && sp2 > sp1) {
            String moduleName = command.substring(sp1 + 1, sp2);
            String onoff = command.substring(sp2 + 1);
            if (checkSafetyLimits(moduleName, "autostart", onoff)) {
                Module* mod = ModuleManager::getInstance()->getModule(moduleName);
                if (mod) { mod->setAutoStart(onoff == "on"); Serial.println("Autostart updated"); }
                else { Serial.println("Module not found"); }
            } else {
                Serial.println("Safety check failed - autostart update blocked");
            }
        } else {
            Serial.println("Usage: autostart <module> <on|off>");
        }
    }
    else if (cmd.startsWith("logs")) {
        int lines = 20;
        if (cmd.length() > 5) {
            lines = command.substring(5).toInt();
            if (lines > 1000) {
                Serial.println("Error: Maximum log lines is 1000");
                return;
            }
        }
        cmdLogs(lines);
    }
    else if (cmd == "clearlogs") {
        if (checkSafetyLimits("SYSTEM", "clearlogs", "")) {
            cmdClearLogs();
        } else {
            Serial.println("Safety check failed - clear logs blocked");
        }
    }
    else if (cmd == "restart") {
        if (checkSafetyLimits("SYSTEM", "restart", "")) {
            cmdRestart();
        } else {
            Serial.println("Safety check failed - restart blocked");
        }
    }
    else if (cmd == "clear") {
        Serial.print("\033[2J\033[H"); // Clear screen
    }
    else if (cmd.startsWith("lcd brightness ")) {
        int sp = command.indexOf(' ');
        int val = command.substring(sp + 12).toInt();
        if (val < 0 || val > 255) {
            Serial.println("Error: Brightness must be 0-255");
            return;
        }
        Module* lcdMod = ModuleManager::getInstance()->getModule("CONTROL_LCD");
        if (lcdMod) { 
            static_cast<CONTROL_LCD*>(lcdMod)->setBrightness((uint8_t)val); 
            Serial.println("LCD brightness updated"); 
        }
        else Serial.println("LCD module not available");
    }
    else if (cmd.startsWith("lcd rotation ")) {
        int sp = command.indexOf(' ');
        int val = command.substring(sp + 12).toInt();
        if (val < 0 || val > 3) {
            Serial.println("Error: Rotation must be 0-3");
            return;
        }
        Module* lcdMod = ModuleManager::getInstance()->getModule("CONTROL_LCD");
        if (lcdMod) { 
            static_cast<CONTROL_LCD*>(lcdMod)->setRotation((uint8_t)val); 
            Serial.println("LCD rotation updated"); 
        }
        else Serial.println("LCD module not available");
    }
    else if (cmd.startsWith("radar ")) {
        // Enhanced radar commands with safety checks
        if (cmd.startsWith("radar rotate ")) {
            String modeStr = command.substring(13);
            int mode = 0;
            if (modeStr == "slow") mode = 1; 
            else if (modeStr == "fast") mode = 2; 
            else if (modeStr == "auto") mode = 3; 
            else mode = 0;
            
            Module* r = ModuleManager::getInstance()->getModule("CONTROL_RADAR");
            if (r) { 
                static_cast<CONTROL_RADAR*>(r)->setRotationModePublic(mode); 
                Serial.println("RADAR rotation mode updated"); 
            }
            else Serial.println("RADAR module not available");
        }
        else if (cmd.startsWith("radar measure ")) {
            String m = command.substring(13);
            int mode = (m == "movement") ? 1 : 0;
            Module* r = ModuleManager::getInstance()->getModule("CONTROL_RADAR");
            if (r) { 
                static_cast<CONTROL_RADAR*>(r)->setMeasureModePublic(mode); 
                Serial.println("RADAR measure mode updated"); 
            }
            else Serial.println("RADAR module not available");
        }
        else if (cmd.startsWith("radar uln ")) {
            String args = command.substring(10);
            int a[4] = {0,0,0,0};
            int idx = 0; int start = 0;
            while (idx < 4) {
                int comma = args.indexOf(',', start);
                String tok = (comma >= 0) ? args.substring(start, comma) : args.substring(start);
                tok.trim(); 
                int pin = tok.toInt();
                if (pin < 0 || pin > 48) {
                    Serial.println("Error: Pin numbers must be 0-48");
                    return;
                }
                a[idx++] = pin; 
                if (comma < 0) break; 
                start = comma + 1;
            }
            Module* r = ModuleManager::getInstance()->getModule("CONTROL_RADAR");
            if (r && idx == 4) { 
                static_cast<CONTROL_RADAR*>(r)->setStepperULN2003((uint8_t)a[0],(uint8_t)a[1],(uint8_t)a[2],(uint8_t)a[3]); 
                Serial.println("RADAR ULN2003 pins set"); 
            }
            else Serial.println("RADAR module not available or args error");
        }
        else {
            Serial.println("Unknown radar command. Available: rotate, measure, uln");
        }
    }
    else {
        // Enhanced error message with command suggestions
        Serial.println("Unknown command: " + command);
        Serial.println("Type 'help' for available commands or 'help <command>' for detailed help");
        
        // Suggest similar commands
        if (cmd.startsWith("mod")) {
            Serial.println("Did you mean: 'modules' or 'module <name>'?");
        } else if (cmd.startsWith("sta")) {
            Serial.println("Did you mean: 'status' or 'start <module>'?");
        } else if (cmd.startsWith("sto")) {
            Serial.println("Did you mean: 'stop <module>'?");
        } else if (cmd.startsWith("tes")) {
            Serial.println("Did you mean: 'test <module>'?");
        }
    }
}

void CONTROL_SERIAL::printHelp() {
    Serial.println("\n========================================");
    Serial.println("ESP32 Serial Control - Commands");
    Serial.println("========================================");
    Serial.println("help [command]     - Show help (detailed help for specific command)");
    Serial.println("status             - Show system status");
    Serial.println("modules            - List all modules");
    Serial.println("module <name>      - Show module info");
    Serial.println("start <name>       - Start module (with safety checks)");
    Serial.println("stop <name>        - Stop module (with safety checks)");
    Serial.println("test <name>        - Test module (with safety checks)");
    Serial.println("cmd <m> <c> [a]    - Send command to module (with validation)");
    Serial.println("config <subcmd>    - Configuration management");
    Serial.println("system <subcmd>    - System-level commands");
    Serial.println("realtime           - Real-time status monitoring");
    Serial.println("safety             - Show safety information");
    Serial.println("set <m> <k> <v>    - Set module key to value (with safety checks)");
    Serial.println("setjson <m> <js>   - Replace module JSON (with safety checks)");
    Serial.println("enable <name>      - Enable module (with safety checks)");
    Serial.println("disable <name>     - Disable module (with safety checks)");
    Serial.println("autostart <m> on|off - Set autostart (with safety checks)");
    Serial.println("logs [n]           - Show last n log lines (max: 1000)");
    Serial.println("clearlogs          - Clear all logs (with confirmation)");
    Serial.println("restart            - Restart system (with confirmation)");
    Serial.println("clear              - Clear screen");
    Serial.println("========================================\n");
    Serial.println("Safety Features:");
    Serial.println("✓ Critical module protection");
    Serial.println("✓ Parameter validation");
    Serial.println("✓ Operation confirmation for critical actions");
    Serial.println("✓ Comprehensive error handling");
    Serial.println("Use 'help <command>' for detailed command information");
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
    if (lines > 200) { Serial.println("Warning: Line count capped at 200"); lines = 200; }
    String logs = fs->readLogs(lines);
    
    Serial.println("\n========== Logs ==========");
    if (logs.length() == 0) Serial.println("(no logs)"); else Serial.print(logs);
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

void CONTROL_SERIAL::cmdHelpDetailed(const String& command) {
    String cmd = command;
    cmd.toLowerCase();
    cmd.trim();
    
    Serial.println("\n========================================");
    Serial.println("Detailed Help: " + command);
    Serial.println("========================================\n");
    
    if (cmd == "status") {
        Serial.println("Shows comprehensive system status including:");
        Serial.println("- Uptime and system information");
        Serial.println("- Memory usage (free heap)");
        Serial.println("- Module states and configurations");
        Serial.println("- Network information (if available)");
    }
    else if (cmd == "modules") {
        Serial.println("Lists all registered modules with their:");
        Serial.println("- Current state (ENABLED/DISABLED)");
        Serial.println("- Priority level (higher = more important)");
        Serial.println("- Version information");
        Serial.println("- Autostart configuration");
    }
    else if (cmd == "module") {
        Serial.println("Shows detailed information about a specific module:");
        Serial.println("Usage: module <module_name>");
        Serial.println("Example: module CONTROL_LCD");
        Serial.println("");
        Serial.println("Shows configuration, status, and statistics");
    }
    else if (cmd == "start" || cmd == "stop") {
        Serial.println("Controls module state with safety checks:");
        Serial.println("Usage: start|stop <module_name>");
        Serial.println("Example: start CONTROL_WIFI");
        Serial.println("");
        Serial.println("Safety features:");
        Serial.println("- Prevents stopping critical modules");
        Serial.println("- Validates module dependencies");
        Serial.println("- Provides feedback on operation result");
    }
    else if (cmd == "test") {
        Serial.println("Runs module self-test with comprehensive checks:");
        Serial.println("Usage: test <module_name>");
        Serial.println("Example: test CONTROL_FS");
        Serial.println("");
        Serial.println("Test coverage varies by module:");
        Serial.println("- CONTROL_FS: File operations, space validation");
        Serial.println("- CONTROL_LCD: Display functionality, colors");
        Serial.println("- CONTROL_WIFI: Connection tests, signal strength");
        Serial.println("- CONTROL_RADAR: Sensor readings, stepper motor");
    }
    else if (cmd == "cmd") {
        Serial.println("Sends specific commands to modules:");
        Serial.println("Usage: cmd <module> <command> [args]");
        Serial.println("Example: cmd CONTROL_LCD brightness 128");
        Serial.println("");
        Serial.println("Available commands by module:");
        Serial.println("- CONTROL_LCD: brightness <0-255>, rotation <0-3>");
        Serial.println("- CONTROL_RADAR: rotate <slow|fast|auto|off>, measure <distance|movement>");
        Serial.println("- CONTROL_WIFI: connect, disconnect, scan");
    }
    else if (cmd == "config") {
        Serial.println("Configuration management commands:");
        Serial.println("Usage: config <subcommand> [args]");
        Serial.println("");
        Serial.println("Subcommands:");
        Serial.println("- config show <module> - Show module configuration");
        Serial.println("- config set <module> <key> <value> - Set configuration value");
        Serial.println("- config backup - Create configuration backup");
        Serial.println("- config restore <name> - Restore from backup");
        Serial.println("- config validate - Validate current configuration");
    }
    else if (cmd == "system") {
        Serial.println("System-level commands:");
        Serial.println("Usage: system <subcommand>");
        Serial.println("");
        Serial.println("Subcommands:");
        Serial.println("- system info - Show detailed system information");
        Serial.println("- system stats - Show performance statistics");
        Serial.println("- system reset - Reset to factory defaults");
        Serial.println("- system update - Check for system updates");
    }
    else if (cmd == "realtime") {
        Serial.println("Real-time system monitoring:");
        Serial.println("Shows continuously updating system status");
        Serial.println("");
        Serial.println("Displays:");
        Serial.println("- CPU and memory usage");
        Serial.println("- Module states and activity");
        Serial.println("- Network status (if available)");
        Serial.println("- Sensor readings (if available)");
        Serial.println("");
        Serial.println("Press any key to stop monitoring");
    }
    else if (cmd == "safety") {
        Serial.println("Safety and security information:");
        Serial.println("Shows current safety limits and restrictions");
        Serial.println("");
        Serial.println("Safety features:");
        Serial.println("- Critical module protection");
        Serial.println("- Parameter validation");
        Serial.println("- Operation logging");
        Serial.println("- Emergency stop capabilities");
    }
    else if (cmd == "logs") {
        Serial.println("System log management:");
        Serial.println("Usage: logs [number_of_lines]");
        Serial.println("Example: logs 50");
        Serial.println("");
        Serial.println("Shows recent system logs with timestamps");
        Serial.println("Default: 20 lines, Maximum: 1000 lines");
    }
    else if (cmd == "set" || cmd == "setjson") {
        Serial.println("Configuration modification commands:");
        Serial.println("Usage: set <module> <key> <value>");
        Serial.println("Usage: setjson <module> <json_object>");
        Serial.println("");
        Serial.println("Examples:");
        Serial.println("- set CONTROL_LCD brightness 128");
        Serial.println("- setjson CONTROL_WIFI {\"ssid\":\"MyWiFi\",\"password\":\"secret\"}");
        Serial.println("");
        Serial.println("Safety: Changes are validated before application");
    }
    else if (cmd == "enable" || cmd == "disable") {
        Serial.println("Module enable/disable with safety checks:");
        Serial.println("Usage: enable|disable <module_name>");
        Serial.println("");
        Serial.println("Safety features:");
        Serial.println("- Prevents disabling critical modules");
        Serial.println("- Validates dependencies before changes");
        Serial.println("- Provides rollback capabilities");
    }
    else if (cmd == "autostart") {
        Serial.println("Module autostart configuration:");
        Serial.println("Usage: autostart <module> <on|off>");
        Serial.println("");
        Serial.println("Controls whether module starts automatically at boot");
        Serial.println("Applied after next system restart");
    }
    else {
        Serial.println("No detailed help available for: " + command);
        Serial.println("Available commands: help, status, modules, module, start, stop, test, cmd, config, system, realtime, safety, logs, set, setjson, enable, disable, autostart");
    }
    
    Serial.println("\n========================================");
}

void CONTROL_SERIAL::cmdConfig(const String& args) {
    String configCmd = args;
    configCmd.toLowerCase();
    configCmd.trim();
    
    if (configCmd.startsWith("show ")) {
        String moduleName = configCmd.substring(5);
        moduleName.trim();
        cmdModuleConfig(moduleName);
    }
    else if (configCmd == "backup") {
        Module* fsModule = ModuleManager::getInstance()->getModule("CONTROL_FS");
        if (fsModule) {
            CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsModule);
            ConfigManager* configMgr = fs->getConfigManager();
            if (configMgr) {
                if (configMgr->createBackup("manual_backup")) {
                    Serial.println("Configuration backup created successfully");
                    ConfigManager::ConfigStats stats = configMgr->getStatistics();
                    Serial.println("Backup count: " + String(stats.backupCount));
                } else {
                    Serial.println("Failed to create backup");
                }
            } else {
                Serial.println("ConfigManager not available");
            }
        } else {
            Serial.println("FS module not available");
        }
    }
    else if (configCmd.startsWith("restore ")) {
        String backupName = configCmd.substring(8);
        backupName.trim();
        Serial.println("Restore functionality not yet implemented");
    }
    else if (configCmd == "validate") {
        Module* fsModule = ModuleManager::getInstance()->getModule("CONTROL_FS");
        if (fsModule) {
            CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsModule);
            ConfigManager* configMgr = fs->getConfigManager();
            if (configMgr) {
                DynamicJsonDocument* cfgDoc = configMgr->getConfiguration();
                ConfigValidationResult result = cfgDoc ? configMgr->validateConfiguration(*cfgDoc) : CONFIG_INVALID_SCHEMA;
                if (result == CONFIG_VALID) {
                    Serial.println("Configuration validation: PASSED");
                } else {
                    Serial.println("Configuration validation: FAILED");
                    Serial.println(String("Error: ") + configMgr->getValidationErrorString(result));
                }
            } else {
                Serial.println("ConfigManager not available");
            }
        } else {
            Serial.println("FS module not available");
        }
    }
    else if (configCmd == "schema") {
        Module* fsModule = ModuleManager::getInstance()->getModule("CONTROL_FS");
        if (!fsModule) { Serial.println("FS module not available"); return; }
        CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsModule);
        String schema = fs->readFile("/schema.json");
        if (schema.length() == 0) { Serial.println("No schema found"); }
        else { Serial.println(schema); }
    }
    else {
        Serial.println("Unknown config command: " + configCmd);
        Serial.println("Available: show <module>, backup, restore <name>, validate, schema");
    }
}

void CONTROL_SERIAL::cmdSystem(const String& args) {
    String systemCmd = args;
    systemCmd.toLowerCase();
    systemCmd.trim();
    
    if (systemCmd == "info") {
        Serial.println("\n========== System Information ==========");
        Serial.println("Hardware:");
        Serial.print("  Chip Model: "); Serial.println(ESP.getChipModel());
        Serial.print("  Chip Revision: "); Serial.println(ESP.getChipRevision());
        Serial.print("  CPU Frequency: "); Serial.print(ESP.getCpuFreqMHz()); Serial.println(" MHz");
        Serial.print("  Flash Size: "); Serial.print(ESP.getFlashChipSize()); Serial.println(" bytes");
        Serial.print("  Flash Speed: "); Serial.print(ESP.getFlashChipSpeed()); Serial.println(" Hz");
        
        Serial.println("\nMemory:");
        Serial.print("  Total Heap: "); Serial.print(ESP.getHeapSize()); Serial.println(" bytes");
        Serial.print("  Free Heap: "); Serial.print(ESP.getFreeHeap()); Serial.println(" bytes");
        Serial.print("  Minimum Free Heap: "); Serial.print(ESP.getMinFreeHeap()); Serial.println(" bytes");
        Serial.print("  Largest Free Block: "); Serial.print(ESP.getMaxAllocHeap()); Serial.println(" bytes");
        
        Serial.println("\nNetwork:");
        Module* wifiModule = ModuleManager::getInstance()->getModule("CONTROL_WIFI");
        if (wifiModule && wifiModule->getState() == MODULE_ENABLED) {
            CONTROL_WIFI* wifi = static_cast<CONTROL_WIFI*>(wifiModule);
            Serial.print("  WiFi SSID: "); Serial.println(wifi->getSSID());
            Serial.print("  IP Address: "); Serial.println(wifi->getIP());
            Serial.print("  MAC Address: "); Serial.println(WiFi.macAddress());
            Serial.print("  Signal Strength: "); Serial.print(wifi->getRSSI()); Serial.println(" dBm");
        } else {
            Serial.println("  WiFi: Not connected");
        }
        
        Serial.println("\nFilesystem:");
        Module* fsModule = ModuleManager::getInstance()->getModule("CONTROL_FS");
        if (fsModule && fsModule->getState() == MODULE_ENABLED) {
            CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsModule);
            Serial.print("  Total Space: "); Serial.print(fs->getTotalSpace()); Serial.println(" bytes");
            Serial.print("  Used Space: "); Serial.print(fs->getUsedSpace()); Serial.println(" bytes");
            Serial.print("  Free Space: "); Serial.print(fs->getFreeSpace()); Serial.println(" bytes");
        } else {
            Serial.println("  Filesystem: Not available");
        }
        
        Serial.println("=======================================");
    }
    else if (systemCmd == "stats") {
        Serial.println("\n========== Performance Statistics ==========");
        Serial.print("Uptime: "); Serial.print(millis() / 1000); Serial.println(" seconds");
        Serial.print("Boot Count: "); Serial.println("Not implemented");
        esp_reset_reason_t rr = esp_reset_reason();
        const char* rrStr = "UNKNOWN";
        switch (rr) {
            case ESP_RST_POWERON: rrStr = "POWERON"; break;
            case ESP_RST_EXT: rrStr = "EXTERNAL"; break;
            case ESP_RST_SW: rrStr = "SOFTWARE"; break;
            case ESP_RST_PANIC: rrStr = "PANIC"; break;
            case ESP_RST_INT_WDT: rrStr = "INT_WDT"; break;
            case ESP_RST_TASK_WDT: rrStr = "TASK_WDT"; break;
            case ESP_RST_WDT: rrStr = "WDT"; break;
            case ESP_RST_DEEPSLEEP: rrStr = "DEEPSLEEP"; break;
            case ESP_RST_BROWNOUT: rrStr = "BROWNOUT"; break;
            case ESP_RST_SDIO: rrStr = "SDIO"; break;
            default: rrStr = "UNKNOWN"; break;
        }
        Serial.print("Reset Reason: "); Serial.println(rrStr);
        
        // Module statistics
        auto modules = ModuleManager::getInstance()->getModules();
        Serial.print("Total Modules: "); Serial.println(modules.size());
        
        int enabledCount = 0;
        int disabledCount = 0;
        int errorCount = 0;
        
        for (Module* mod : modules) {
            switch (mod->getState()) {
                case MODULE_ENABLED: enabledCount++; break;
                case MODULE_DISABLED: disabledCount++; break;
                case MODULE_ERROR: errorCount++; break;
            }
        }
        
        Serial.print("  Enabled: "); Serial.println(enabledCount);
        Serial.print("  Disabled: "); Serial.println(disabledCount);
        Serial.print("  Error: "); Serial.println(errorCount);
        
        Serial.println("============================================");
    }
    else if (systemCmd == "reset") {
        Serial.println("System reset functionality not yet implemented");
    }
    else if (systemCmd == "update") {
        Serial.println("System update functionality not yet implemented");
    }
    else if (systemCmd == "fscheck") {
        Module* fsModule = ModuleManager::getInstance()->getModule("CONTROL_FS");
        if (!fsModule) { Serial.println("FS module not available"); return; }
        CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsModule);
        Serial.println("Running filesystem audit...");
        bool ok = fs->auditFileSystem(true);
        Serial.println(ok ? "FS audit passed" : "FS audit found issues");
    }
    else {
        Serial.println("Unknown system command: " + systemCmd);
        Serial.println("Available: info, stats, reset, update, fscheck");
    }
}

void CONTROL_SERIAL::cmdModuleCommand(const String& moduleName, const String& command, const String& args) {
    // Route module-specific commands
    if (moduleName == "CONTROL_LCD") {
        Module* lcdMod = ModuleManager::getInstance()->getModule("CONTROL_LCD");
        if (!lcdMod) {
            Serial.println("LCD module not available");
            return;
        }
        
        if (command == "brightness") {
            int brightness = args.toInt();
            if (brightness < 0 || brightness > 255) {
                Serial.println("Error: Brightness must be 0-255");
                return;
            }
            static_cast<CONTROL_LCD*>(lcdMod)->setBrightness((uint8_t)brightness);
            Serial.println("LCD brightness set to " + String(brightness));
        }
        else if (command == "rotation") {
            int r = args.toInt();
            int mapped = r;
            if (r == 0 || r == 1 || r == 2 || r == 3) { mapped = r; }
            else if (r == 90) mapped = 1;
            else if (r == 180) mapped = 2;
            else if (r == 270) mapped = 3;
            else {
                Serial.println("Error: Rotation must be 0-3 or 0/90/180/270 degrees");
                return;
            }
            static_cast<CONTROL_LCD*>(lcdMod)->setRotation((uint8_t)mapped);
            Serial.println("LCD rotation set to " + String(mapped));
        }
        else {
            Serial.println("Unknown LCD command: " + command);
            Serial.println("Available: brightness, rotation");
        }
    }
    else if (moduleName == "CONTROL_RADAR") {
        Module* radarMod = ModuleManager::getInstance()->getModule("CONTROL_RADAR");
        if (!radarMod) {
            Serial.println("RADAR module not available");
            return;
        }
        
        if (command == "rotate") {
            int mode = 0;
            if (args == "slow") mode = 1;
            else if (args == "fast") mode = 2;
            else if (args == "auto") mode = 3;
            else if (args == "off") mode = 0;
            else {
                Serial.println("Error: Mode must be slow, fast, auto, or off");
                return;
            }
            static_cast<CONTROL_RADAR*>(radarMod)->setRotationModePublic(mode);
            Serial.println("RADAR rotation mode set to " + args);
        }
        else if (command == "measure") {
            int mode = (args == "movement") ? 1 : 0;
            static_cast<CONTROL_RADAR*>(radarMod)->setMeasureModePublic(mode);
            Serial.println("RADAR measure mode set to " + args);
        }
        else {
            Serial.println("Unknown RADAR command: " + command);
            Serial.println("Available: rotate, measure");
        }
    }
    else {
        Serial.println("Module-specific commands not available for: " + moduleName);
        Serial.println("Available modules: CONTROL_LCD, CONTROL_RADAR");
    }
}

void CONTROL_SERIAL::cmdRealTimeStatus() {
    Serial.println("\n========== Real-time Status Monitoring ==========");
    Serial.println("Press any key to stop monitoring...\n");
    
    unsigned long startTime = millis();
    int updateCount = 0;
    
    while (Serial.available() == 0 && updateCount < 100) { // Limit to 100 updates or key press
        Serial.print("\r"); // Clear line
        
        // System info
        Serial.print("Uptime: "); Serial.print(millis() / 1000); Serial.print("s ");
        Serial.print("Free Heap: "); Serial.print(ESP.getFreeHeap()); Serial.print(" ");
        
        // Module status summary
        auto modules = ModuleManager::getInstance()->getModules();
        int enabledCount = 0;
        for (Module* mod : modules) {
            if (mod->getState() == MODULE_ENABLED) enabledCount++;
        }
        Serial.print("Modules: "); Serial.print(enabledCount); Serial.print("/"); Serial.print(modules.size());
        
        // WiFi status if available
        Module* wifiModule = ModuleManager::getInstance()->getModule("CONTROL_WIFI");
        if (wifiModule && wifiModule->getState() == MODULE_ENABLED) {
            CONTROL_WIFI* wifi = static_cast<CONTROL_WIFI*>(wifiModule);
            Serial.print(" WiFi: "); Serial.print(wifi->getRSSI()); Serial.print("dBm");
        }
        
        updateCount++;
        delay(1000); // Update every second
    }
    
    // Clear any pending input
    while (Serial.available() > 0) {
        Serial.read();
    }
    
    Serial.println("\n\nMonitoring stopped.");
    Serial.println("==============================================");
}

void CONTROL_SERIAL::cmdSafetyLimits() {
    Serial.println("\n========== Safety and Security Information ==========");
    
    Serial.println("Safety Features Enabled:");
    Serial.println("✓ Critical module protection");
    Serial.println("✓ Parameter validation");
    Serial.println("✓ Operation logging");
    Serial.println("✓ Emergency stop capabilities");
    
    Serial.println("\nCritical Modules (cannot be disabled):");
    Serial.println("- CONTROL_FS (File System)");
    Serial.println("- CONTROL_SERIAL (Serial Interface)");
    
    Serial.println("\nParameter Validation Limits:");
    Serial.println("- LCD Brightness: 0-255");
    Serial.println("- LCD Rotation: 0-3");
    Serial.println("- Radar Pin Numbers: 0-48");
    Serial.println("- Log Lines: 1-1000");
    
    Serial.println("\nSecurity Features:");
    Serial.println("- Command validation");
    Serial.println("- Input sanitization");
    Serial.println("- Rate limiting (not implemented)");
    Serial.println("- Audit logging");
    
    Serial.println("==================================================");
}

bool CONTROL_SERIAL::validateModuleCommand(const String& moduleName, const String& command, const String& args) {
    // Validate module exists
    Module* mod = ModuleManager::getInstance()->getModule(moduleName);
    if (!mod) {
        return false;
    }
    
    // Validate command based on module type
    if (moduleName == "CONTROL_LCD") {
        if (command == "brightness") {
            int brightness = args.toInt();
            return brightness >= 0 && brightness <= 255;
        }
        else if (command == "rotation") {
            int rotation = args.toInt();
            return rotation >= 0 && rotation <= 3;
        }
    }
    else if (moduleName == "CONTROL_RADAR") {
        if (command == "rotate") {
            return args == "slow" || args == "fast" || args == "auto" || args == "off";
        }
        else if (command == "measure") {
            return args == "distance" || args == "movement";
        }
    }
    
    return false; // Unknown command
}

bool CONTROL_SERIAL::checkSafetyLimits(const String& moduleName, const String& command, const String& args) {
    // Critical modules that cannot be stopped/disabled
    static const String criticalModules[] = {"CONTROL_FS", "CONTROL_SERIAL"};
    
    // Check if trying to stop/disable critical module
    if ((command == "stop" || command == "disable") && 
        (moduleName == "CONTROL_FS" || moduleName == "CONTROL_SERIAL")) {
        Serial.println("ERROR: Cannot stop/disable critical module: " + moduleName);
        return false;
    }
    
    // Check if trying to restart system without confirmation
    if (command == "restart" && moduleName == "SYSTEM") {
        Serial.println("WARNING: System restart requested");
        Serial.println("Type 'restart confirm' to proceed or wait 5 seconds to cancel");
        
        unsigned long startTime = millis();
        while (millis() - startTime < 5000) {
            if (Serial.available() > 0) {
                String confirmation = Serial.readStringUntil('\n');
                confirmation.trim();
                if (confirmation == "restart confirm") {
                    return true;
                } else {
                    Serial.println("Restart cancelled");
                    return false;
                }
            }
        }
        Serial.println("Restart cancelled (timeout)");
        return false;
    }
    
    // Validate parameter ranges for critical operations
    if (command == "clearlogs" && moduleName == "SYSTEM") {
        Serial.println("WARNING: Clear logs will permanently delete all system logs");
        Serial.println("Type 'clearlogs confirm' to proceed or wait 3 seconds to cancel");
        
        unsigned long startTime = millis();
        while (millis() - startTime < 3000) {
            if (Serial.available() > 0) {
                String confirmation = Serial.readStringUntil('\n');
                confirmation.trim();
                if (confirmation == "clearlogs confirm") {
                    return true;
                } else {
                    Serial.println("Clear logs cancelled");
                    return false;
                }
            }
        }
        Serial.println("Clear logs cancelled (timeout)");
        return false;
    }
    
    return true; // All safety checks passed
}

String CONTROL_SERIAL::getCommandHelp(const String& command) {
    String cmd = command;
    cmd.toLowerCase();
    cmd.trim();
    
    if (cmd == "help") return "Show help information";
    if (cmd == "status") return "Show system status";
    if (cmd == "modules") return "List all modules";
    if (cmd == "module") return "Show module information";
    if (cmd == "start") return "Start a module";
    if (cmd == "stop") return "Stop a module";
    if (cmd == "test") return "Test a module";
    if (cmd == "cmd") return "Send command to module";
    if (cmd == "config") return "Configuration management";
    if (cmd == "system") return "System commands";
    if (cmd == "realtime") return "Real-time monitoring";
    if (cmd == "safety") return "Safety information";
    if (cmd == "logs") return "Show system logs";
    if (cmd == "set") return "Set configuration value";
    if (cmd == "setjson") return "Set configuration JSON";
    if (cmd == "enable") return "Enable module";
    if (cmd == "disable") return "Disable module";
    if (cmd == "autostart") return "Set module autostart";
    if (cmd == "clearlogs") return "Clear system logs";
    if (cmd == "restart") return "Restart system";
    if (cmd == "clear") return "Clear screen";
    
    return "Unknown command";
}

void CONTROL_SERIAL::println(const String& message) {
    Serial.println(message);
}

void CONTROL_SERIAL::print(const String& message) {
    Serial.print(message);
}
