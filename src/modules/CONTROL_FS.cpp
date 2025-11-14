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
}

CONTROL_FS::~CONTROL_FS() {
    stop();
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
    
    fsInitialized = true;
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
    std::vector<String> dirs = {"/", "/cfg", "/logs", "/web", "/config", "/data", "/tmp", "/test"};
    for (const String& d : dirs) {
        std::vector<String> files;
        if (listDirectory(d, files)) {
            for (const String& f : files) {
                size_t sz = getFileSize(f);
                String content = readFile(f);
                String preview = content.substring(0, 20);
                String kb = String(((float)sz) / 1024.0, 2);
                log(getLogTimestamp() + " " + f + " " + kb + "kB " + preview);
            }
        }
    }
    log("File system test passed");
    return true;
}

DynamicJsonDocument CONTROL_FS::getStatus() {
    DynamicJsonDocument doc(1024);
    
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
    std::vector<String> dirs = {"/config", "/logs", "/web", "/data", "/tmp", "/test", "/cfg"};
    
    for (const String& dir : dirs) {
        if (!createDirectory(dir)) {
            Serial.println("Failed to create directory: " + dir);
        }
    }
    
    return true;
}

bool CONTROL_FS::validateConfigs() {
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
            writeFile(FS_DEFAULTS[i].path, FS_DEFAULTS[i].content);
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
    
    File file = SPIFFS.open(path, mode);
    if (!file) {
        log("Failed to open file for writing: " + path, "ERROR");
        return false;
    }
    
    size_t written = file.print(content);
    file.close();
    
    if (debugEnabled) {
        log("Written " + String(written) + " bytes to " + path);
    }
    
    return written > 0;
}

String CONTROL_FS::readFile(const String& path) {
    if (!fsInitialized) return "";
    
    if (!fileExists(path)) {
        log("File does not exist: " + path, "WARN");
        return "";
    }
    
    File file = SPIFFS.open(path, "r");
    if (!file) {
        log("Failed to open file for reading: " + path, "ERROR");
        return "";
    }
    
    String content = file.readString();
    file.close();
    
    if (debugEnabled) {
        log("Read " + String(content.length()) + " bytes from " + path);
    }
    
    return content;
}

bool CONTROL_FS::deleteFile(const String& path) {
    if (!fsInitialized) return false;
    
    if (!fileExists(path)) {
        log("File does not exist: " + path, "WARN");
        return false;
    }
    
    bool success = SPIFFS.remove(path);
    if (success && debugEnabled) {
        log("Deleted file: " + path);
    }
    
    return success;
}

bool CONTROL_FS::fileExists(const String& path) {
    if (!fsInitialized) return false;
    return SPIFFS.exists(path);
}

size_t CONTROL_FS::getFileSize(const String& path) {
    if (!fsInitialized || !fileExists(path)) return 0;
    
    File file = SPIFFS.open(path, "r");
    if (!file) return 0;
    
    size_t size = file.size();
    file.close();
    
    return size;
}

bool CONTROL_FS::createDirectory(const String& path) {
    if (!fsInitialized) return false;
    
    // SPIFFS doesn't have real directories, but we can create a marker file
    String markerPath = path + "/.dir";
    return writeFile(markerPath, "0");
}

bool CONTROL_FS::removeDirectory(const String& path) {
    // Not fully implemented for SPIFFS
    return true;
}

bool CONTROL_FS::listDirectory(const String& path, std::vector<String>& files) {
    if (!fsInitialized) return false;
    
    File root = SPIFFS.open(path);
    if (!root || !root.isDirectory()) {
        return false;
    }
    
    File file = root.openNextFile();
    while (file) {
        files.push_back(file.name());
        file = root.openNextFile();
    }
    
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
    
    // Return last N lines
    std::vector<String> lines;
    int lastIndex = 0;
    int index = 0;
    
    while ((index = allLogs.indexOf('\n', lastIndex)) != -1) {
        lines.push_back(allLogs.substring(lastIndex, index + 1));
        lastIndex = index + 1;
    }
    
    String result = "";
    size_t start = lines.size() > maxLines ? lines.size() - maxLines : 0;
    for (size_t i = start; i < lines.size(); i++) {
        result += lines[i];
    }
    
    return result;
}

bool CONTROL_FS::clearLogs() {
    return writeFile(LOG_FILE_PATH, "");
}

size_t CONTROL_FS::getLogSize() {
    return getFileSize(LOG_FILE_PATH);
}

bool CONTROL_FS::loadGlobalConfig(DynamicJsonDocument& doc) {
    String configStr = readFile(CONFIG_FILE_PATH);
    if (configStr.length() == 0) {
        log(String("Global config file not found: ") + String(CONFIG_FILE_PATH), "WARN");
        return false;
    }
    
    DeserializationError error = deserializeJson(doc, configStr);
    if (error) {
        log("Failed to parse global config: " + String(error.c_str()), "ERROR");
        return false;
    }
    
    log("Global config loaded successfully");
    return true;
}

bool CONTROL_FS::saveGlobalConfig(const DynamicJsonDocument& doc) {
    String configStr;
    serializeJsonPretty(doc, configStr);
    
    if (!writeFile(CONFIG_FILE_PATH, configStr)) {
        log("Failed to save global config", "ERROR");
        return false;
    }
    
    log("Global config saved successfully");
    return true;
}

bool CONTROL_FS::loadModuleConfig(const String& moduleName, DynamicJsonDocument& doc) {
    String path = "/config/" + moduleName + ".json";
    String configStr = readFile(path);
    
    if (configStr.length() == 0) {
        log("Module config not found: " + moduleName, "WARN");
        return false;
    }
    
    DeserializationError error = deserializeJson(doc, configStr);
    if (error) {
        log("Failed to parse module config: " + moduleName, "ERROR");
        return false;
    }
    
    return true;
}

bool CONTROL_FS::saveModuleConfig(const String& moduleName, const DynamicJsonDocument& doc) {
    String path = "/config/" + moduleName + ".json";
    String configStr;
    serializeJsonPretty(doc, configStr);
    
    return writeFile(path, configStr);
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
