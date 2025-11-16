#include "CONTROL_FS.h"

CONTROL_FS::CONTROL_FS() : Module("CONTROL_FS") {
    fsMaxSize = FS_MAX_SIZE_DEFAULT;
    logMaxSize = LOG_MAX_SIZE_DEFAULT;
    fsInitialized = false;
    priority = 100; // Highest priority
    autoStart = true;
    version = "1.0.1";
    TaskConfig tcfg = getTaskConfig();
    tcfg.name = "CONTROL_FS_TASK";
    tcfg.stackSize = 4096;
    tcfg.priority = 3;
    tcfg.core = 0;
    setTaskConfig(tcfg);
    fsMutex = nullptr;
    configManager = nullptr;
}

CONTROL_FS::~CONTROL_FS() {
    stop();
    if (configManager) {
        delete configManager;
        configManager = nullptr;
    }
}

bool CONTROL_FS::init() {
    log("Initializing file system...");
    
    if (!initFileSystem()) {
        log("Failed to initialize SPIFFS", "ERROR");
        setState(MODULE_ERROR);
        return false;
    }
    
    initVersionAndPopulate();
    
    if (!checkAndCreateDirectories()) {
        log("Failed to create directories", "ERROR");
        setState(MODULE_ERROR);
        return false;
    }
    
    if (!validateConfigs()) {
        log("Config validation failed", "ERROR");
    }
    
    // Initialize ConfigManager
    if (!initConfigManager()) {
        log("ConfigManager initialization failed", "ERROR");
        setState(MODULE_ERROR);
        return false;
    }
    
    fsInitialized = true;
    if (!fsMutex) fsMutex = xSemaphoreCreateMutex();
    setState(MODULE_ENABLED);
    
    size_t files = countFiles();
    size_t total = getTotalSpace();
    size_t used = getUsedSpace();
    size_t free = getFreeSpace();
    log("FS summary: files=" + String(files) + ", total=" + String(total) + ", used=" + String(used) + ", free=" + String(free));
    log("File system initialized successfully");
    return true;
}

bool CONTROL_FS::start() {
    if (!fsInitialized) {
        return init();
    }
    
    setState(MODULE_ENABLED);
    log("File system started");
    return true;
}

bool CONTROL_FS::stop() {
    if (fsInitialized) {
        SPIFFS.end();
        fsInitialized = false;
    }
    
    setState(MODULE_DISABLED);
    log("File system stopped");
    return true;
}

bool CONTROL_FS::update() {
    // Check log size and rotate if necessary
    if (getLogSize() > logMaxSize) {
        log("Log size exceeded, rotating logs", "WARN");
        // Keep last 1000 lines
        String logs = readLogs(1000);
        clearLogs();
        writeLog(logs.c_str());
    }
    
    return true;
}

bool CONTROL_FS::test() {
    log("Testing file system...");
    
    // Test write
    String testPath = "/test/test.txt";
    String testContent = "Test content " + String(millis());
    
    if (!writeFile(testPath, testContent)) {
        log("Write test failed", "ERROR");
        return false;
    }
    
    // Test read
    String readContent = readFile(testPath);
    if (readContent != testContent) {
        log("Read test failed", "ERROR");
        return false;
    }
    
    // Test delete
    if (!deleteFile(testPath)) {
        log("Delete test failed", "ERROR");
        return false;
    }
    
    size_t total = getTotalSpace();
    size_t used = getUsedSpace();
    size_t free = getFreeSpace();
    size_t filesRoot = countFiles();
    log("FS capacity total=" + String(total) + ", used=" + String(used) + ", free=" + String(free));
    log("FS files count=" + String(filesRoot));
    std::vector<String> dirs = {"/", "/logs", "/web", "/config", "/data", "/tmp", "/test"};
    for (const String& d : dirs) {
        std::vector<String> files;
        if (listDirectory(d, files)) {
            for (const String& name : files) {
                String path = name;
                if (path.length() == 0 || path[0] != '/') {
                    path = (d == "/") ? (String("/") + name) : (d + "/" + name);
                }
                File f = SPIFFS.open(path);
                if (!f) {
                    log(String("File open failed: ") + path, "WARN");
                    continue;
                }
                if (f.isDirectory()) { f.close(); continue; }
                size_t sz = f.size();
                f.close();
                String content = readFile(path);
                String preview = content.substring(0, 20);
                String kb = String(((float)sz) / 1024.0, 2);
                log(getLogTimestamp() + " " + path + " " + kb + "kB " + preview);
            }
        }
    }
    // Run comprehensive audit after basic tests
    bool auditOk = auditFileSystem(true);
    log(auditOk ? "File system test passed (audit OK)" : "File system test passed (audit found issues)");
    return auditOk;
}

DynamicJsonDocument CONTROL_FS::getStatus() {
    DynamicJsonDocument doc(2048);
    
    doc["module"] = moduleName;
    doc["state"] = state == MODULE_ENABLED ? "enabled" : "disabled";
    doc["version"] = version;
    doc["priority"] = priority;
    doc["autoStart"] = autoStart;
    doc["debug"] = debugEnabled;
    
    doc["totalSpace"] = getTotalSpace();
    doc["usedSpace"] = getUsedSpace();
    doc["freeSpace"] = getFreeSpace();
    doc["logSize"] = getLogSize();
    doc["fsMaxSize"] = fsMaxSize;
    doc["logMaxSize"] = logMaxSize;
    
    // Add ConfigManager status
    if (configManager) {
        JsonObject configMgr = doc.createNestedObject("configManager");
        configMgr["initialized"] = true;
        configMgr["version"] = configManager->getCurrentVersion();
        ConfigManager::ConfigStats stats = configManager->getStatistics();
        configMgr["backupCount"] = stats.backupCount;
        configMgr["lastBackup"] = stats.lastBackupTime;
        JsonObject s = configMgr.createNestedObject("stats");
        s["configSize"] = stats.configSize;
        s["totalBackupSize"] = stats.totalBackupSize;
        s["validConfigs"] = stats.validConfigs;
    } else {
        doc["configManager"] = "not_initialized";
    }
    
    return doc;
}

bool CONTROL_FS::initFileSystem() {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return false;
    }
    fsInitialized = true;
    
    Serial.println("SPIFFS mounted successfully");
    Serial.printf("Total space: %d bytes\n", SPIFFS.totalBytes());
    Serial.printf("Used space: %d bytes\n", SPIFFS.usedBytes());
    Serial.printf("Free space: %d bytes\n", SPIFFS.totalBytes() - SPIFFS.usedBytes());
    
    return true;
}

bool CONTROL_FS::checkAndCreateDirectories() {
    std::vector<String> dirs = {"/config", "/logs", "/web", "/data", "/tmp", "/test"};
    
    for (const String& dir : dirs) {
        if (!createDirectory(dir)) {
            Serial.println("Failed to create directory: " + dir);
        }
    }
    
    return true;
}

bool CONTROL_FS::auditFileSystem(bool fix) {
    log("Starting filesystem audit...");
    Module* lcdMod = ModuleManager::getInstance()->getModule("CONTROL_LCD");
    QueueBase* lcdQ = (lcdMod && lcdMod->getState() == MODULE_ENABLED) ? lcdMod->getQueue() : nullptr;
    auto pushLCD = [&](const String& msg){
        if (!lcdQ) return;
        DynamicJsonDocument* vars = new DynamicJsonDocument(128);
        JsonArray arr = vars->createNestedArray("v");
        arr.add(msg);
        QueueMessage* qm = new QueueMessage{genUUID4(), lcdMod->getName(), getName(), EVENT_DATA_READY, CALL_FUNCTION_ASYNC, String("lcd_log_append"), vars};
        lcdQ->send(qm);
    };
    pushLCD("Audit: scanning files...");
    std::vector<String> paths;
    std::vector<String> dirs = {"/", "/config", "/logs", "/web", "/data", "/backups"};
    for (const String& d : dirs) { std::vector<String> files; if (listDirectory(d, files)) { for (const String& f : files) paths.push_back((d == "/") ? f : (d + "/" + f)); } }
    size_t issues = 0;
    for (const String& p : paths) {
        size_t sz = getFileSize(p);
        String purpose = "generic";
        if (p == "/config.json") purpose = "global_config";
        else if (p == "/schema.json") purpose = "schema";
        else if (p.startsWith("/config/")) purpose = "module_config";
        else if (p.startsWith("/logs/")) purpose = "log";
        else if (p.startsWith("/backups/")) purpose = "backup";
        log(purpose + ": " + p + " size=" + String(sz));
        pushLCD(purpose + String(" ") + String(sz) + "B");
        if (p.endsWith(".json")) {
            String content = readFile(p);
            DynamicJsonDocument doc(16384);
            DeserializationError err = deserializeJson(doc, content);
            if (err) { log("JSON parse error: " + p + " - " + String(err.c_str()), "ERROR"); issues++; continue; }
            if (purpose == "global_config" && configManager) {
                ConfigValidationResult v = configManager->validateConfiguration(doc);
                if (v != CONFIG_VALID) {
                    log("Global config invalid: " + configManager->getValidationErrorString(v), "ERROR");
                    issues++;
                    if (fix) {
                        DynamicJsonDocument* cur = configManager->getConfiguration();
                        if (cur) { *cur = doc; }
                        configManager->migrateToLatestVersion();
                        configManager->saveConfiguration();
                    }
                }
            } else if (purpose == "module_config" && configManager) {
                int slash = p.lastIndexOf('/'); int dot = p.lastIndexOf('.');
                String modName = p.substring(slash + 1, dot);
                if (!configManager->validateModuleConfig(modName, doc)) { log("Module config invalid: " + modName, "WARN"); issues++; }
            }
        }
    }
    pushLCD("Audit: completed");
    log(String("Filesystem audit finished. Issues=") + String(issues));
    return issues == 0;
}

bool CONTROL_FS::validateConfigs() {
    // Legacy validation - will be replaced by ConfigManager
    DynamicJsonDocument doc(8192);
    String cfg = readFile(CONFIG_FILE_PATH);
    if (cfg.length() == 0) return false;
    DeserializationError err = deserializeJson(doc, cfg);
    if (err) return false;
    return true;
}

bool CONTROL_FS::initVersionAndPopulate() {
    String initVer = readFile("/.init");
    if (initVer != version) {
        SPIFFS.format();
        for (size_t i = 0; i < FS_DEFAULTS_COUNT; i++) {
            String path = String(FS_DEFAULTS[i].path);
            if (path.startsWith("/cfg/")) {
                path = String("/config/") + path.substring(5);
            } else if (path == "/config/global.json") {
                path = "/config.json";
            }
            writeFile(path, FS_DEFAULTS[i].content);
        }
        writeFile("/.init", version);
    }
    return true;
}

size_t CONTROL_FS::countFiles() {
    size_t count = 0;
    File root = SPIFFS.open("/");
    if (!root) return 0;
    File file = root.openNextFile();
    while (file) { count++; file = root.openNextFile(); }
    return count;
}

String CONTROL_FS::getLogTimestamp() {
    unsigned long ms = millis();
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24;
    
    char timestamp[32];
    snprintf(timestamp, sizeof(timestamp), "[%02ld:%02ld:%02ld:%03ld]", 
             hours % 24, minutes % 60, seconds % 60, ms % 1000);
    
    return String(timestamp);
}

bool CONTROL_FS::writeFile(const String& path, const String& content, const char* mode) {
    if (!fsInitialized) return false;
    if (fsMutex) xSemaphoreTake(fsMutex, portMAX_DELAY);
    
    File file = SPIFFS.open(path, mode);
    if (!file) {
        log("Failed to open file for writing: " + path, "ERROR");
        if (fsMutex) xSemaphoreGive(fsMutex);
        return false;
    }
    
    size_t written = file.print(content);
    file.close();
    if (fsMutex) xSemaphoreGive(fsMutex);
    
    if (debugEnabled) {
        log("Written " + String(written) + " bytes to " + path);
    }
    
    return written > 0;
}

String CONTROL_FS::readFile(const String& path) {
    if (!fsInitialized) return "";
    if (fsMutex) xSemaphoreTake(fsMutex, portMAX_DELAY);
    
    if (!fileExists(path)) {
        log("File does not exist: " + path, "WARN");
        if (fsMutex) xSemaphoreGive(fsMutex);
        return "";
    }
    
    File file = SPIFFS.open(path, "r");
    if (!file) {
        log("Failed to open file for reading: " + path, "ERROR");
        if (fsMutex) xSemaphoreGive(fsMutex);
        return "";
    }
    
    String content = file.readString();
    file.close();
    if (fsMutex) xSemaphoreGive(fsMutex);
    
    if (debugEnabled) {
        log("Read " + String(content.length()) + " bytes from " + path);
    }
    
    return content;
}

bool CONTROL_FS::deleteFile(const String& path) {
    if (!fsInitialized) return false;
    if (fsMutex) xSemaphoreTake(fsMutex, portMAX_DELAY);
    
    if (!fileExists(path)) {
        log("File does not exist: " + path, "WARN");
        if (fsMutex) xSemaphoreGive(fsMutex);
        return false;
    }
    
    bool success = SPIFFS.remove(path);
    if (success && debugEnabled) {
        log("Deleted file: " + path);
    }
    if (fsMutex) xSemaphoreGive(fsMutex);
    return success;
}

bool CONTROL_FS::fileExists(const String& path) {
    if (!fsInitialized) return false;
    bool exists = false;
    if (fsMutex) xSemaphoreTake(fsMutex, portMAX_DELAY);
    exists = SPIFFS.exists(path);
    if (fsMutex) xSemaphoreGive(fsMutex);
    return exists;
}

size_t CONTROL_FS::getFileSize(const String& path) {
    if (!fsInitialized) return 0;
    if (fsMutex) xSemaphoreTake(fsMutex, portMAX_DELAY);
    if (!SPIFFS.exists(path)) { if (fsMutex) xSemaphoreGive(fsMutex); return 0; }
    File file = SPIFFS.open(path, "r");
    if (!file) { if (fsMutex) xSemaphoreGive(fsMutex); return 0; }
    size_t size = file.size();
    file.close();
    if (fsMutex) xSemaphoreGive(fsMutex);
    return size;
}

bool CONTROL_FS::createDirectory(const String& path) {
    if (!fsInitialized) return false;
    if (fsMutex) xSemaphoreTake(fsMutex, portMAX_DELAY);
    
    // SPIFFS doesn't have real directories, but we can create a marker file
    String markerPath = path + "/.dir";
    bool ok = writeFile(markerPath, "0");
    if (fsMutex) xSemaphoreGive(fsMutex);
    return ok;
}

bool CONTROL_FS::removeDirectory(const String& path) {
    // Not fully implemented for SPIFFS
    return true;
}

bool CONTROL_FS::listDirectory(const String& path, std::vector<String>& files) {
    if (!fsInitialized) return false;
    if (fsMutex) xSemaphoreTake(fsMutex, portMAX_DELAY);
    
    File root = SPIFFS.open(path);
    if (!root || !root.isDirectory()) {
        if (fsMutex) xSemaphoreGive(fsMutex);
        return false;
    }
    
    File file = root.openNextFile();
    while (file) {
        files.push_back(file.name());
        file = root.openNextFile();
    }
    if (fsMutex) xSemaphoreGive(fsMutex);
    return true;
}

bool CONTROL_FS::writeLog(const String& message, const char* level) {
    if (!fsInitialized) {
        Serial.println(message);
        return false;
    }
    
    String logEntry = getLogTimestamp() + " [" + String(level) + "] " + message + "\n";
    const char* path = LOG_FILE_PATH;
    if (strcmp(level, "DEBUG") == 0) {
        path = "/logs/debug.log";
    }
    return writeFile(path, logEntry, "a");
}

String CONTROL_FS::readLogs(size_t maxLines) {
    if (!fsInitialized) return "";
    
    String allLogs = readFile(LOG_FILE_PATH);
    if (allLogs.length() == 0) return "";
    
    size_t cap = max((size_t)1, min(maxLines, (size_t)200));
    String* ring = new String[cap];
    size_t write = 0;
    size_t stored = 0;
    int lastIndex = 0;
    int index = 0;
    while ((index = allLogs.indexOf('\n', lastIndex)) != -1) {
        String line = allLogs.substring(lastIndex, index + 1);
        ring[write] = line;
        write = (write + 1) % cap;
        if (stored < cap) stored++;
        lastIndex = index + 1;
    }
    if (lastIndex < allLogs.length()) {
        String tail = allLogs.substring(lastIndex);
        ring[write] = tail + "\n";
        write = (write + 1) % cap;
        if (stored < cap) stored++;
    }
    String result = "";
    size_t startIdx = (stored < cap) ? 0 : write;
    for (size_t i = 0; i < stored; i++) {
        size_t pos = (startIdx + i) % cap;
        result += ring[pos];
    }
    delete[] ring;
    return result;
}

bool CONTROL_FS::clearLogs() {
    return writeFile(LOG_FILE_PATH, "");
}

size_t CONTROL_FS::getLogSize() {
    return getFileSize(LOG_FILE_PATH);
}

bool CONTROL_FS::loadGlobalConfig(DynamicJsonDocument& doc) {
    if (!configManager) {
        log("ConfigManager not initialized", "ERROR");
        return false;
    }
    DynamicJsonDocument* cfg = configManager->getConfiguration();
    if (!cfg) {
        log("ConfigManager has no configuration", "ERROR");
        return false;
    }
    doc = *cfg;
    return true;
}

bool CONTROL_FS::saveGlobalConfig(const DynamicJsonDocument& doc) {
    if (!configManager) {
        log("ConfigManager not initialized", "ERROR");
        return false;
    }
    DynamicJsonDocument* cfg = configManager->getConfiguration();
    if (!cfg) {
        log("ConfigManager has no configuration", "ERROR");
        return false;
    }
    *cfg = doc;
    ConfigValidationResult v = configManager->validateConfiguration();
    if (v != CONFIG_VALID) {
        log("Configuration validation failed: " + configManager->getValidationErrorString(v), "ERROR");
        return false;
    }
    return configManager->saveConfiguration();
}

bool CONTROL_FS::loadModuleConfig(const String& moduleName, DynamicJsonDocument& doc) {
    if (!configManager) {
        log("ConfigManager not initialized", "ERROR");
        return false;
    }
    return configManager->loadModuleConfig(moduleName, doc);
}

bool CONTROL_FS::saveModuleConfig(const String& moduleName, const DynamicJsonDocument& doc) {
    if (!configManager) {
        log("ConfigManager not initialized", "ERROR");
        return false;
    }
    if (!configManager->saveModuleConfig(moduleName, doc)) {
        log("Failed to set module config in ConfigManager: " + moduleName, "ERROR");
        return false;
    }
    return configManager->saveConfiguration();
}

size_t CONTROL_FS::getFreeSpace() {
    if (!fsInitialized) return 0;
    return SPIFFS.totalBytes() - SPIFFS.usedBytes();
}

size_t CONTROL_FS::getUsedSpace() {
    if (!fsInitialized) return 0;
    return SPIFFS.usedBytes();
}

size_t CONTROL_FS::getTotalSpace() {
    if (!fsInitialized) return 0;
    return SPIFFS.totalBytes();
}

bool CONTROL_FS::formatFileSystem() {
    log("Formatting file system...", "WARN");
    
    if (fsInitialized) {
        SPIFFS.end();
    }
    
    bool success = SPIFFS.format();
    
    if (success) {
        log("File system formatted successfully");
        return init();
    } else {
        log("Failed to format file system", "ERROR");
        return false;
    }
}

bool CONTROL_FS::initConfigManager() {
    log("Initializing ConfigManager...");
    
    configManager = new ConfigManager();
    
    // Load schema from filesystem
    String schemaContent = readFile("/schema.json");
    if (schemaContent.length() == 0) {
        log("Schema file not found, creating default schema", "WARN");
        // Create default schema file
        String defaultSchema = R"({
            "$schema": "http://json-schema.org/draft-07/schema#",
            "type": "object",
            "properties": {
                "version": {"type": "string", "pattern": "^\\d+\\.\\d+\\.\\d+$"},
                "system": {
                    "type": "object",
                    "properties": {
                        "name": {"type": "string", "minLength": 1, "maxLength": 50},
                        "debug": {"type": "boolean"},
                        "timezone": {"type": "string", "enum": ["UTC", "EST", "PST", "CST"]}
                    },
                    "required": ["name"]
                }
            },
            "required": ["version", "system"]
        })";
        writeFile("/schema.json", defaultSchema);
        schemaContent = defaultSchema;
    }
    
    // Initialize ConfigManager and load schema
    if (!configManager->initialize("/config")) {
        log("Failed to initialize ConfigManager", "ERROR");
        return false;
    }
    configManager->loadSchemaFromFile("/schema.json");
    
    // Load or create default configuration
    String configContent = readFile("/config.json");
    if (configContent.length() == 0) {
        log("Configuration file not found, creating default configuration", "WARN");
        // Create default configuration
        String defaultConfig = R"({
            "version": "2.0.0",
            "system": {
                "name": "ESP32_Modular_System",
                "debug": true,
                "timezone": "UTC"
            },
            "filesystem": {
                "max_size": 2097152,
                "log_max_size": 1048576,
                "auto_format": false,
                "enable_cache": true
            }
        })";
        writeFile("/config.json", defaultConfig);
        configContent = defaultConfig;
    }
    
    // Load configuration, or create defaults if missing
    if (!configManager->loadConfiguration()) {
        if (!configManager->createDefaultConfiguration()) {
            log("Failed to create default configuration", "ERROR");
            return false;
        }
    }
    
    ::configManager = configManager;
    log("ConfigManager initialized successfully");
    return true;
}

bool CONTROL_FS::migrateLegacyConfigs() {
    log("Migrating legacy configurations...");
    
    if (!configManager) {
        log("ConfigManager not initialized", "ERROR");
        return false;
    }
    
    // Check for legacy module configs in /cfg/ directory
    std::vector<String> legacyFiles;
    if (listDirectory("/cfg", legacyFiles)) {
        for (const String& file : legacyFiles) {
            if (file.endsWith(".json")) {
                String moduleName = file.substring(0, file.length() - 5); // Remove .json
                String legacyConfig = readFile("/cfg/" + file);
                
                if (legacyConfig.length() > 0) {
                    log("Found legacy config for module: " + moduleName);
                    
                    // Try to migrate the legacy config to new format
                    DynamicJsonDocument legacyDoc(1024);
                    DeserializationError error = deserializeJson(legacyDoc, legacyConfig);
                    
                    if (!error) {
                        // Save legacy config into new structure
                        if (configManager->saveModuleConfig(moduleName, legacyDoc)) {
                            log("Successfully migrated config for: " + moduleName);
                        } else {
                            log("Failed to migrate config for: " + moduleName, "WARN");
                        }
                    }
                }
            }
        }
    }
    
    // Save migrated configuration
    if (configManager->saveConfiguration()) {
        log("Legacy configuration migration completed");
        return true;
    } else {
        log("Failed to save migrated configuration", "ERROR");
        return false;
    }
}
