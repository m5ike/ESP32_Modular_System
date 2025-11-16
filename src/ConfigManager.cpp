/**
 * @file ConfigManager.cpp
 * @brief Configuration management, validation, backups, and migration.
 * @author Michael Kojdl
 * @email michael@kojdl.com
 * @date 2025-11-16
 * @version 2.0.0
 */
#include "ConfigManager.h"
#include "FSDefaults.h"
#include <SPIFFS.h>
#include <MD5Builder.h>

// Global configuration manager instance
ConfigManager* configManager = nullptr;

ConfigManager::ConfigManager(fs::FS* fs) : filesystem(fs), currentConfig(nullptr), currentVersion(CONFIG_VERSION_CURRENT) {
    if (filesystem == nullptr) {
        filesystem = &SPIFFS;
    }
}

ConfigManager::~ConfigManager() {
    if (currentConfig) {
        delete currentConfig;
        currentConfig = nullptr;
    }
}

bool ConfigManager::initialize(const String& basePath) {
    if (!filesystem) {
        return false;
    }
    
    configPath = basePath + "/config.json";
    backupPath = basePath + "/backups";
    schemaPath = basePath + "/schema.json";
    
    // Create directories if they don't exist
    if (!filesystem->exists(basePath)) {
        filesystem->mkdir(basePath);
    }
    if (!filesystem->exists(backupPath)) {
        filesystem->mkdir(backupPath);
    }
    
    // Initialize current configuration
    currentConfig = new DynamicJsonDocument(16384); // 16KB initial size
    
    // Load default validation rules
    addDefaultValidationRules();
    
    return true;
}

bool ConfigManager::loadConfiguration() {
    return loadConfiguration(configPath);
}

bool ConfigManager::saveConfiguration() {
    return saveConfiguration(configPath);
}

bool ConfigManager::loadConfiguration(const String& path) {
    if (!isInitialized()) {
        return false;
    }
    
    DynamicJsonDocument tempDoc(16384);
    if (!loadConfigFromFile(path, tempDoc)) {
        return false;
    }
    
    // Validate configuration
    ConfigValidationResult result = validateConfiguration(tempDoc);
    if (result != CONFIG_VALID) {
        Serial.printf("[CONFIG] Validation failed: %s\n", getValidationErrorString(result).c_str());
        return false;
    }
    
    // Check version compatibility and migrate if needed
    String version = getConfigVersion(tempDoc);
    if (version != currentVersion) {
        Serial.printf("[CONFIG] Migrating from version %s to %s\n", version.c_str(), currentVersion.c_str());
        if (!migrateConfiguration(tempDoc, currentVersion)) {
            Serial.println("[CONFIG] Migration failed");
            return false;
        }
    }
    
    // Copy to current configuration
    *currentConfig = tempDoc;
    currentVersion = getConfigVersion(*currentConfig);
    
    Serial.println("[CONFIG] Configuration loaded successfully");
    return true;
}

bool ConfigManager::saveConfiguration(const String& path) {
    if (!isInitialized() || !currentConfig) {
        return false;
    }
    
    // Validate before saving
    ConfigValidationResult result = validateConfiguration(*currentConfig);
    if (result != CONFIG_VALID) {
        Serial.printf("[CONFIG] Cannot save invalid configuration: %s\n", getValidationErrorString(result).c_str());
        return false;
    }
    
    // Create backup before saving
    createBackup("auto_backup_before_save");
    
    if (!saveConfigToFile(path, *currentConfig)) {
        return false;
    }
    
    Serial.println("[CONFIG] Configuration saved successfully");
    return true;
}

bool ConfigManager::loadConfigFromFile(const String& path, DynamicJsonDocument& doc) {
    if (!filesystem->exists(path)) {
        Serial.printf("[CONFIG] Configuration file not found: %s\n", path.c_str());
        return false;
    }
    
    File file = filesystem->open(path, "r");
    if (!file) {
        Serial.printf("[CONFIG] Failed to open configuration file: %s\n", path.c_str());
        return false;
    }
    
    String content = file.readString();
    file.close();
    
    DeserializationError error = deserializeJson(doc, content);
    if (error) {
        Serial.printf("[CONFIG] Failed to parse configuration: %s\n", error.c_str());
        return false;
    }
    
    return true;
}

bool ConfigManager::saveConfigToFile(const String& path, const DynamicJsonDocument& doc) {
    File file = filesystem->open(path, "w");
    if (!file) {
        Serial.printf("[CONFIG] Failed to create configuration file: %s\n", path.c_str());
        return false;
    }
    
    String output;
    serializeJsonPretty(doc, output);
    
    size_t written = file.print(output);
    file.close();
    
    if (written == 0) {
        Serial.printf("[CONFIG] Failed to write configuration file: %s\n", path.c_str());
        return false;
    }
    
    return true;
}

ConfigValidationResult ConfigManager::validateConfiguration() {
    if (!currentConfig) {
        return CONFIG_INVALID_SCHEMA;
    }
    return validateConfiguration(*currentConfig);
}

ConfigValidationResult ConfigManager::validateConfiguration(const DynamicJsonDocument& doc) {
    // Check version compatibility
    String version = getConfigVersion(doc);
    if (!isVersionCompatible(version)) {
        return CONFIG_INVALID_VERSION;
    }
    
    // Validate schema
    if (!validateSchema(doc)) {
        return CONFIG_INVALID_SCHEMA;
    }
    
    // Validate required fields
    if (!validateRequiredFields(doc)) {
        return CONFIG_MISSING_REQUIRED;
    }
    
    // Validate value ranges
    if (!validateValueRanges(doc)) {
        return CONFIG_INVALID_VALUE;
    }
    
    return CONFIG_VALID;
}

bool ConfigManager::validateSchema(const DynamicJsonDocument& doc) {
    // Check basic structure
    if (!doc.containsKey("version") || !doc["version"].is<String>()) {
        return false;
    }
    
    if (!doc.containsKey("modules") || !doc["modules"].is<JsonObject>()) {
        return false;
    }
    
    // Validate each module configuration
    JsonObjectConst modules = doc["modules"].as<JsonObjectConst>();
    for (JsonPairConst kv : modules) {
        const char* moduleName = kv.key().c_str();
        JsonObjectConst moduleConfig = kv.value().as<JsonObjectConst>();
        
        if (!moduleConfig.containsKey("state") || !moduleConfig["state"].is<String>()) {
            return false;
        }
        
        if (!moduleConfig.containsKey("priority") || !moduleConfig["priority"].is<int>()) {
            return false;
        }
        
        if (!moduleConfig.containsKey("version") || !moduleConfig["version"].is<String>()) {
            return false;
        }
    }
    
    return true;
}

bool ConfigManager::validateRequiredFields(const DynamicJsonDocument& doc) {
    for (const auto& rule : validationRules) {
        if (rule.required) {
            JsonVariantConst value = getNestedValueConst(doc, rule.path);
            if (value.isNull()) {
                Serial.printf("[CONFIG] Missing required field: %s\n", rule.path.c_str());
                return false;
            }
        }
    }
    return true;
}

bool ConfigManager::validateValueRanges(const DynamicJsonDocument& doc) {
    for (const auto& rule : validationRules) {
        JsonVariantConst value = getNestedValueConst(doc, rule.path);
        if (value.isNull()) continue;
        
        // Type validation
        if (rule.type == "int" && !value.is<int>()) {
            Serial.printf("[CONFIG] Invalid type for %s: expected int\n", rule.path.c_str());
            return false;
        }
        if (rule.type == "bool" && !value.is<bool>()) {
            Serial.printf("[CONFIG] Invalid type for %s: expected bool\n", rule.path.c_str());
            return false;
        }
        if (rule.type == "string" && !value.is<String>()) {
            Serial.printf("[CONFIG] Invalid type for %s: expected string\n", rule.path.c_str());
            return false;
        }
        
        // Range validation omitted in simplified rules
        
        // Enum validation for strings
        if (rule.type == "string" && !rule.enumValues.empty()) {
            String strValue = value.as<String>();
            bool found = false;
            for (const auto& enumValue : rule.enumValues) {
                if (strValue == enumValue) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                Serial.printf("[CONFIG] Invalid enum value for %s: %s\n", rule.path.c_str(), strValue.c_str());
                return false;
            }
        }
    }
    return true;
}

String ConfigManager::getValidationErrorString(ConfigValidationResult result) {
    switch (result) {
        case CONFIG_VALID: return "Configuration is valid";
        case CONFIG_INVALID_VERSION: return "Invalid or unsupported configuration version";
        case CONFIG_INVALID_SCHEMA: return "Configuration does not match required schema";
        case CONFIG_MISSING_REQUIRED: return "Missing required configuration fields";
        case CONFIG_INVALID_VALUE: return "Invalid configuration values";
        case CONFIG_FILE_NOT_FOUND: return "Configuration file not found";
        case CONFIG_PARSE_ERROR: return "Configuration file parse error";
        default: return "Unknown validation error";
    }
}

bool ConfigManager::getConfigValue(const String& path, JsonVariant& value) const {
    if (!currentConfig) {
        return false;
    }
    std::vector<String> parts;
    splitPath(path, parts);
    if (parts.empty()) return false;
    JsonVariant current = currentConfig->as<JsonVariant>();
    for (size_t i = 0; i < parts.size(); i++) {
        if (current.is<JsonObject>()) {
            JsonObject obj = current.as<JsonObject>();
            if (obj.containsKey(parts[i])) {
                current = obj[parts[i]];
            } else {
                return false;
            }
        } else {
            return false;
        }
    }
    value = current;
    return !value.isNull();
}

bool ConfigManager::setConfigValue(const String& path, const JsonVariant& value) {
    if (!currentConfig) {
        return false;
    }
    
    return setNestedValue(*currentConfig, path, value);
}

bool ConfigManager::removeConfigValue(const String& path) {
    if (!currentConfig) {
        return false;
    }
    
    return removeNestedValue(*currentConfig, path);
}

String ConfigManager::getConfigVersion(const DynamicJsonDocument& doc) const {
    if (!doc.containsKey("version")) {
        return "1.0.0"; // Default version
    }
    return doc["version"].as<String>();
}

bool ConfigManager::setConfigVersion(DynamicJsonDocument& doc, const String& version) {
    doc["version"] = version;
    return true;
}

bool ConfigManager::isVersionCompatible(const String& version) const {
    // Simple version comparison - in production, use semantic versioning library
    return version >= CONFIG_VERSION_MIN_SUPPORTED && version <= CONFIG_VERSION_CURRENT;
}

bool ConfigManager::createBackup(const String& description) {
    if (!isInitialized() || !currentConfig) {
        return false;
    }
    
    String backupFile = generateBackupFilename();
    if (!description.isEmpty()) {
        backupFile = backupFile.substring(0, backupFile.length() - 5) + "_" + description + ".json";
    }
    
    return createBackupInternal(backupFile, *currentConfig);
}

String ConfigManager::generateBackupFilename() {
    unsigned long timestamp = millis();
    char filename[64];
    snprintf(filename, sizeof(filename), "backup_%lu_%s.json", timestamp, currentVersion.c_str());
    return String(filename);
}

bool ConfigManager::createBackupInternal(const String& backupFile, const DynamicJsonDocument& doc) {
    String fullPath = backupPath + "/" + backupFile;
    
    // Add backup metadata
    DynamicJsonDocument backupDoc(doc.capacity());
    backupDoc["backup_info"] = JsonObject();
    backupDoc["backup_info"]["timestamp"] = String(millis());
    backupDoc["backup_info"]["version"] = getConfigVersion(doc);
    backupDoc["backup_info"]["description"] = "Automatic backup";
    backupDoc["config"] = doc;
    
    if (!saveConfigToFile(fullPath, backupDoc)) {
        Serial.printf("[CONFIG] Failed to create backup: %s\n", backupFile.c_str());
        return false;
    }
    
    Serial.printf("[CONFIG] Backup created: %s\n", backupFile.c_str());
    return true;
}

std::vector<ConfigBackupInfo> ConfigManager::listBackups() {
    std::vector<ConfigBackupInfo> backups;
    
    if (!filesystem->exists(backupPath)) {
        return backups;
    }
    
    File backupDir = filesystem->open(backupPath);
    if (!backupDir || !backupDir.isDirectory()) {
        return backups;
    }
    
    File file = backupDir.openNextFile();
    while (file) {
        if (file.name()[0] != '.' && strstr(file.name(), ".json")) {
            ConfigBackupInfo info;
            info.filename = String(file.name());
            info.size = file.size();
            info.valid = true;
            
            // Try to load backup info
            String fullPath = backupPath + "/" + info.filename;
            DynamicJsonDocument backupDoc(16384);
            if (loadConfigFromFile(fullPath, backupDoc)) {
                if (backupDoc.containsKey("backup_info")) {
                    JsonObject backupInfo = backupDoc["backup_info"];
                    info.timestamp = backupInfo["timestamp"] | "unknown";
                    info.version = backupInfo["version"] | "unknown";
                }
            }
            
            backups.push_back(info);
        }
        file = backupDir.openNextFile();
    }
    
    return backups;
}

bool ConfigManager::restoreFromBackup(const String& backupFile) {
    String fullPath = backupPath + "/" + backupFile;
    
    DynamicJsonDocument backupDoc(16384);
    if (!loadConfigFromFile(fullPath, backupDoc)) {
        return false;
    }
    
    // Extract configuration from backup
    DynamicJsonDocument configDoc(16384);
    if (backupDoc.containsKey("config")) {
        configDoc = backupDoc["config"];
    } else {
        configDoc = backupDoc; // Old backup format
    }
    
    // Validate restored configuration
    ConfigValidationResult result = validateConfiguration(configDoc);
    if (result != CONFIG_VALID) {
        Serial.printf("[CONFIG] Restored configuration validation failed: %s\n", getValidationErrorString(result).c_str());
        return false;
    }
    
    // Apply restored configuration
    *currentConfig = configDoc;
    currentVersion = getConfigVersion(*currentConfig);
    
    Serial.printf("[CONFIG] Configuration restored from backup: %s\n", backupFile.c_str());
    return true;
}

bool ConfigManager::restoreFromBackup(const ConfigBackupInfo& backupInfo) {
    return restoreFromBackup(backupInfo.filename);
}

bool ConfigManager::deleteBackup(const String& backupFile) {
    String fullPath = backupPath + "/" + backupFile;
    return filesystem->remove(fullPath);
}

bool ConfigManager::deleteAllBackups() {
    std::vector<ConfigBackupInfo> backups = listBackups();
    bool success = true;
    
    for (const auto& backup : backups) {
        if (!deleteBackup(backup.filename)) {
            success = false;
        }
    }
    
    return success;
}

bool ConfigManager::loadDefaultConfiguration() {
    if (!isInitialized()) {
        return false;
    }
    
    // Create default configuration based on FS_DEFAULTS
    currentConfig->clear();
    
    // Find the global config in FS_DEFAULTS
    for (size_t i = 0; i < FS_DEFAULTS_COUNT; i++) {
        if (String(FS_DEFAULTS[i].path) == "/config.json") {
            DeserializationError error = deserializeJson(*currentConfig, FS_DEFAULTS[i].content);
            if (error) {
                Serial.printf("[CONFIG] Failed to parse default configuration: %s\n", error.c_str());
                return false;
            }
            break;
        }
    }
    
    currentVersion = getConfigVersion(*currentConfig);
    Serial.println("[CONFIG] Default configuration loaded");
    return true;
}

bool ConfigManager::createDefaultConfiguration() {
    if (!loadDefaultConfiguration()) {
        return false;
    }
    
    return saveConfiguration();
}

bool ConfigManager::addValidationRule(const ConfigValidationRule& rule) {
    validationRules.push_back(rule);
    return true;
}

void ConfigManager::addDefaultValidationRules() {
    // Add basic validation rules
    ConfigValidationRule r1; r1.path = "version"; r1.type = "string"; r1.required = true; validationRules.push_back(r1);
    ConfigValidationRule r2; r2.path = "modules.CONTROL_FS.priority"; r2.type = "int"; r2.required = true; validationRules.push_back(r2);
    ConfigValidationRule r3; r3.path = "modules.CONTROL_WIFI.priority"; r3.type = "int"; r3.required = true; validationRules.push_back(r3);
    ConfigValidationRule r4; r4.path = "modules.CONTROL_LCD.priority"; r4.type = "int"; r4.required = true; validationRules.push_back(r4);
    ConfigValidationRule r5; r5.path = "modules.CONTROL_SERIAL.priority"; r5.type = "int"; r5.required = true; validationRules.push_back(r5);
    ConfigValidationRule r6; r6.path = "modules.CONTROL_WEB.priority"; r6.type = "int"; r6.required = true; validationRules.push_back(r6);
    ConfigValidationRule r7; r7.path = "modules.CONTROL_RADAR.priority"; r7.type = "int"; r7.required = true; validationRules.push_back(r7);
    
    // Module state validation
    std::vector<String> validStates = {"enabled", "disabled", "error"};
    ConfigValidationRule s1; s1.path = "modules.CONTROL_FS.state"; s1.type = "string"; s1.required = true; s1.enumValues = validStates; validationRules.push_back(s1);
    ConfigValidationRule s2; s2.path = "modules.CONTROL_WIFI.state"; s2.type = "string"; s2.required = true; s2.enumValues = validStates; validationRules.push_back(s2);
    ConfigValidationRule s3; s3.path = "modules.CONTROL_LCD.state"; s3.type = "string"; s3.required = true; s3.enumValues = validStates; validationRules.push_back(s3);
    ConfigValidationRule s4; s4.path = "modules.CONTROL_SERIAL.state"; s4.type = "string"; s4.required = true; s4.enumValues = validStates; validationRules.push_back(s4);
    ConfigValidationRule s5; s5.path = "modules.CONTROL_WEB.state"; s5.type = "string"; s5.required = true; s5.enumValues = validStates; validationRules.push_back(s5);
    ConfigValidationRule s6; s6.path = "modules.CONTROL_RADAR.state"; s6.type = "string"; s6.required = true; s6.enumValues = validStates; validationRules.push_back(s6);
}

bool ConfigManager::migrateToLatestVersion() {
    if (!currentConfig) {
        return false;
    }
    
    return migrateConfiguration(*currentConfig, currentVersion);
}

bool ConfigManager::migrateConfiguration(DynamicJsonDocument& doc, const String& targetVersion) {
    String currentVersion = getConfigVersion(doc);
    
    if (currentVersion == targetVersion) {
        return true; // Already at target version
    }
    
    // Apply migrations in sequence
    if (currentVersion == "1.0.0" && targetVersion >= "1.1.0") {
        if (!migrateFrom1_0_0_to1_1_0(doc)) {
            return false;
        }
        currentVersion = "1.1.0";
    }
    
    if (currentVersion == "1.1.0" && targetVersion >= "1.2.0") {
        if (!migrateFrom1_1_0_to1_2_0(doc)) {
            return false;
        }
        currentVersion = "1.2.0";
    }
    
    if (currentVersion == "1.2.0" && targetVersion >= "2.0.0") {
        if (!migrateFrom1_2_0_to2_0_0(doc)) {
            return false;
        }
        currentVersion = "2.0.0";
    }
    
    // Update version in document
    setConfigVersion(doc, targetVersion);
    
    Serial.printf("[CONFIG] Migrated configuration from %s to %s\n", 
                  getConfigVersion(doc).c_str(), targetVersion.c_str());
    return true;
}

bool ConfigManager::migrateFrom1_0_0_to1_1_0(DynamicJsonDocument& doc) {
    // Add new fields introduced in 1.1.0
    if (!doc.containsKey("backup_settings")) {
        doc["backup_settings"]["auto_backup"] = true;
        doc["backup_settings"]["backup_count"] = 10;
        doc["backup_settings"]["backup_interval_hours"] = 24;
    }
    
    Serial.println("[CONFIG] Migrated from 1.0.0 to 1.1.0");
    return true;
}

bool ConfigManager::migrateFrom1_1_0_to1_2_0(DynamicJsonDocument& doc) {
    // Add new fields introduced in 1.2.0
    if (!doc.containsKey("monitoring")) {
        doc["monitoring"]["enabled"] = true;
        doc["monitoring"]["health_check_interval"] = 30000; // 30 seconds
        doc["monitoring"]["performance_tracking"] = true;
    }
    
    Serial.println("[CONFIG] Migrated from 1.1.0 to 1.2.0");
    return true;
}

bool ConfigManager::migrateFrom1_2_0_to2_0_0(DynamicJsonDocument& doc) {
    // Major restructuring for 2.0.0
    if (doc.containsKey("modules")) {
        JsonObject modules = doc["modules"];
        
        // Add watchdog configuration to all modules
        for (JsonPair kv : modules) {
            JsonObject moduleConfig = kv.value();
            if (!moduleConfig.containsKey("watchdog")) {
                moduleConfig["watchdog"]["enabled"] = true;
                moduleConfig["watchdog"]["timeout_ms"] = 5000;
                moduleConfig["watchdog"]["auto_restart"] = true;
            }
        }
    }
    
    // Add system-wide watchdog configuration
    if (!doc.containsKey("system")) {
        doc["system"]["watchdog"]["enabled"] = true;
        doc["system"]["watchdog"]["timeout_ms"] = 10000;
        doc["system"]["watchdog"]["reset_on_timeout"] = true;
    }
    
    Serial.println("[CONFIG] Migrated from 1.2.0 to 2.0.0");
    return true;
}

void ConfigManager::clearConfiguration() {
    if (currentConfig) {
        currentConfig->clear();
        currentVersion = CONFIG_VERSION_CURRENT;
    }
}

size_t ConfigManager::getConfigurationSize() const {
    if (!currentConfig) {
        return 0;
    }
    
    String output;
    serializeJson(*currentConfig, output);
    return output.length();
}

String ConfigManager::getConfigurationHash() const {
    if (!currentConfig) {
        return "";
    }
    
    String output;
    serializeJson(*currentConfig, output);
    
    MD5Builder md5;
    md5.begin();
    md5.add(output);
    md5.calculate();
    return md5.toString();
}

bool ConfigManager::isConfigurationDirty() const {
    // This is a simplified implementation - in production, track actual changes
    return false;
}

void ConfigManager::markConfigurationClean() {
    // Mark configuration as clean (no unsaved changes)
}

bool ConfigManager::loadModuleConfig(const String& moduleName, DynamicJsonDocument& moduleConfig) {
    if (!currentConfig || !currentConfig->containsKey("modules")) {
        return false;
    }
    
    JsonObject modules = (*currentConfig)["modules"];
    if (!modules.containsKey(moduleName)) {
        return false;
    }
    
    moduleConfig = modules[moduleName];
    return true;
}

bool ConfigManager::saveModuleConfig(const String& moduleName, const DynamicJsonDocument& moduleConfig) {
    if (!currentConfig) {
        return false;
    }
    
    if (!currentConfig->containsKey("modules")) {
        (*currentConfig)["modules"] = JsonObject();
    }
    
    JsonObject modules = (*currentConfig)["modules"];
    modules[moduleName] = moduleConfig;
    
    return true;
}

bool ConfigManager::validateModuleConfig(const String& moduleName, const DynamicJsonDocument& moduleConfig) {
    // Basic module configuration validation
    if (!moduleConfig.containsKey("state") || !moduleConfig["state"].is<String>()) {
        return false;
    }
    
    if (!moduleConfig.containsKey("priority") || !moduleConfig["priority"].is<int>()) {
        return false;
    }
    
    if (!moduleConfig.containsKey("version") || !moduleConfig["version"].is<String>()) {
        return false;
    }
    
    return true;
}

ConfigManager::ConfigStats ConfigManager::getStatistics() {
    ConfigStats stats = {};
    
    if (!isInitialized()) {
        return stats;
    }
    
    // Count total configurations
    stats.totalConfigs = 1; // Global config
    
    // Count valid configurations
    stats.validConfigs = (validateConfiguration() == CONFIG_VALID) ? 1 : 0;
    
    // Count backups
    std::vector<ConfigBackupInfo> backups = listBackups();
    stats.backupCount = backups.size();
    
    // Calculate total backup size
    stats.totalBackupSize = 0;
    for (const auto& backup : backups) {
        stats.totalBackupSize += backup.size;
    }
    
    // Get last backup time
    if (!backups.empty()) {
        stats.lastBackupTime = backups.back().timestamp;
    }
    
    // Configuration info
    stats.configSize = getConfigurationSize();
    stats.configVersion = currentVersion;
    
    return stats;
}

bool ConfigManager::loadSchemaFromFile(const String& schemaFile) {
    if (!filesystem) {
        return false;
    }
    if (!filesystem->exists(schemaFile)) {
        return false;
    }
    File file = filesystem->open(schemaFile, "r");
    if (!file) {
        return false;
    }
    String content = file.readString();
    file.close();
    DynamicJsonDocument doc(8192);
    DeserializationError err = deserializeJson(doc, content);
    if (err) {
        return false;
    }
    schemaPath = schemaFile;
    validationRules.clear();
    addDefaultValidationRules();
    return true;
}

bool ConfigManager::saveSchemaToFile(const String& schemaFile) const {
    if (!filesystem) {
        return false;
    }
    DynamicJsonDocument doc(2048);
    doc["$schema"] = "http://json-schema.org/draft-07/schema#";
    doc["type"] = "object";
    JsonObject props = doc.createNestedObject("properties");
    JsonObject ver = props.createNestedObject("version");
    ver["type"] = "string";
    JsonObject modules = props.createNestedObject("modules");
    modules["type"] = "object";
    String output;
    serializeJsonPretty(doc, output);
    File file = filesystem->open(schemaFile, "w");
    if (!file) {
        return false;
    }
    size_t written = file.print(output);
    file.close();
    return written > 0;
}

JsonVariantConst ConfigManager::getNestedValueConst(const DynamicJsonDocument& doc, const String& path) const {
    std::vector<String> parts;
    splitPath(path, parts);
    
    JsonVariantConst current = doc.as<JsonVariantConst>();
    for (const auto& part : parts) {
        if (current.is<JsonObjectConst>()) {
            JsonObjectConst obj = current.as<JsonObjectConst>();
            if (obj.containsKey(part)) {
                current = obj[part];
            } else {
                return JsonVariantConst(); // Not found
            }
        } else {
            return JsonVariantConst(); // Not an object
        }
    }
    
    return current;
}

bool ConfigManager::setNestedValue(DynamicJsonDocument& doc, const String& path, const JsonVariant& value) {
    std::vector<String> parts;
    splitPath(path, parts);
    
    if (parts.empty()) {
        return false;
    }
    
    JsonVariant current = doc.as<JsonVariant>();
    for (size_t i = 0; i < parts.size() - 1; i++) {
        if (current.is<JsonObject>()) {
            JsonObject obj = current.as<JsonObject>();
            if (!obj.containsKey(parts[i])) {
                obj[parts[i]] = JsonObject();
            }
            current = obj[parts[i]];
        } else {
            return false; // Cannot create nested structure
        }
    }
    
    // Set the final value
    if (current.is<JsonObject>()) {
        JsonObject obj = current.as<JsonObject>();
        obj[parts.back()] = value;
        return true;
    }
    
    return false;
}

bool ConfigManager::removeNestedValue(DynamicJsonDocument& doc, const String& path) {
    std::vector<String> parts;
    splitPath(path, parts);
    
    if (parts.empty()) {
        return false;
    }
    
    JsonVariant current = doc.as<JsonVariant>();
    for (size_t i = 0; i < parts.size() - 1; i++) {
        if (current.is<JsonObject>()) {
            JsonObject obj = current.as<JsonObject>();
            if (obj.containsKey(parts[i])) {
                current = obj[parts[i]];
            } else {
                return false; // Path doesn't exist
            }
        } else {
            return false; // Not an object
        }
    }
    
    // Remove the final value
    if (current.is<JsonObject>()) {
        JsonObject obj = current.as<JsonObject>();
        obj.remove(parts.back());
        return true;
    }
    
    return false;
}

void ConfigManager::splitPath(const String& path, std::vector<String>& parts) const {
    parts.clear();
    int start = 0;
    int end = path.indexOf('.');
    
    while (end != -1) {
        parts.push_back(path.substring(start, end));
        start = end + 1;
        end = path.indexOf('.', start);
    }
    
    if (start < path.length()) {
        parts.push_back(path.substring(start));
    }
}