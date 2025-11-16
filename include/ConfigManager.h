/**
 * @file ConfigManager.h
 * @brief Configuration manager API: loading, saving, validation, backups, and statistics.
 * @author Michael Kojdl
 * @email michael@kojdl.com
 * @date 2025-11-16
 * @version 2.0.0
 */
#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include <memory>
#include "FS.h"

// Configuration versioning
#define CONFIG_VERSION_CURRENT "2.0.0"
#define CONFIG_VERSION_MIN_SUPPORTED "1.0.0"

// Configuration validation result codes
enum ConfigValidationResult {
    CONFIG_VALID,
    CONFIG_INVALID_VERSION,
    CONFIG_INVALID_SCHEMA,
    CONFIG_MISSING_REQUIRED,
    CONFIG_INVALID_VALUE,
    CONFIG_FILE_NOT_FOUND,
    CONFIG_PARSE_ERROR
};

// Configuration backup information
struct ConfigBackupInfo {
    String filename;
    String timestamp;
    String version;
    size_t size;
    bool valid;
};

// Configuration schema validation rule
struct ConfigValidationRule {
    String path;           // JSON path (e.g., "modules.CONTROL_FS.priority")
    String type;           // "int", "bool", "string", "float", "array", "object"
    bool required;         // Whether field is required
    JsonVariant defaultValue; // Default value if missing
    JsonVariant minValue;     // Minimum value (for numeric types)
    JsonVariant maxValue;     // Maximum value (for numeric types)
    std::vector<String> enumValues; // Allowed values (for strings)
};

class ConfigManager {
private:
    fs::FS* filesystem;
    String configPath;
    String backupPath;
    String schemaPath;
    DynamicJsonDocument* currentConfig;
    String currentVersion;
    std::vector<ConfigValidationRule> validationRules;
    
    // Internal methods
    bool loadConfigFromFile(const String& path, DynamicJsonDocument& doc);
    bool saveConfigToFile(const String& path, const DynamicJsonDocument& doc);
    ConfigValidationResult validateConfigInternal(const DynamicJsonDocument& doc);
    bool validateSchema(const DynamicJsonDocument& doc);
    bool validateRequiredFields(const DynamicJsonDocument& doc);
    bool validateValueRanges(const DynamicJsonDocument& doc);
    bool applyDefaults(DynamicJsonDocument& doc);
    bool migrateConfig(DynamicJsonDocument& doc, const String& fromVersion);
    String generateBackupFilename();
    bool createBackupInternal(const String& backupFile, const DynamicJsonDocument& doc);
    
public:
    ConfigManager(fs::FS* fs = nullptr);
    ~ConfigManager();
    
    // Initialization
    bool initialize(const String& basePath = "/config");
    bool isInitialized() const { return filesystem != nullptr && currentConfig != nullptr; }
    
    // Configuration loading and saving
    bool loadConfiguration();
    bool saveConfiguration();
    bool loadConfiguration(const String& path);
    bool saveConfiguration(const String& path);
    
    // Configuration validation
    ConfigValidationResult validateConfiguration();
    ConfigValidationResult validateConfiguration(const DynamicJsonDocument& doc);
    String getValidationErrorString(ConfigValidationResult result);
    
    // Configuration access
    DynamicJsonDocument* getConfiguration() { return currentConfig; }
    const DynamicJsonDocument* getConfiguration() const { return currentConfig; }
    bool getConfigValue(const String& path, JsonVariant& value) const;
    bool setConfigValue(const String& path, const JsonVariant& value);
    bool removeConfigValue(const String& path);
    
    // Configuration versioning
    String getCurrentVersion() const { return currentVersion; }
    String getConfigVersion(const DynamicJsonDocument& doc) const;
    bool setConfigVersion(DynamicJsonDocument& doc, const String& version);
    bool isVersionCompatible(const String& version) const;
    
    // Configuration backup and recovery
    bool createBackup(const String& description = "");
    std::vector<ConfigBackupInfo> listBackups();
    bool restoreFromBackup(const String& backupFile);
    bool restoreFromBackup(const ConfigBackupInfo& backupInfo);
    bool deleteBackup(const String& backupFile);
    bool deleteAllBackups();
    
    // Configuration defaults and schema
    bool loadDefaultConfiguration();
    bool createDefaultConfiguration();
    bool addValidationRule(const ConfigValidationRule& rule);
    bool loadSchemaFromFile(const String& schemaFile);
    bool saveSchemaToFile(const String& schemaFile) const;
    
    // Configuration migration
    bool migrateToLatestVersion();
    bool migrateConfiguration(DynamicJsonDocument& doc, const String& targetVersion);
    
    // Utility methods
    void clearConfiguration();
    size_t getConfigurationSize() const;
    String getConfigurationHash() const;
    bool isConfigurationDirty() const;
    void markConfigurationClean();
    
    // Module-specific configuration
    bool loadModuleConfig(const String& moduleName, DynamicJsonDocument& moduleConfig);
    bool saveModuleConfig(const String& moduleName, const DynamicJsonDocument& moduleConfig);
    bool validateModuleConfig(const String& moduleName, const DynamicJsonDocument& moduleConfig);
    
    // Statistics and monitoring
    struct ConfigStats {
        size_t totalConfigs;
        size_t validConfigs;
        size_t backupCount;
        size_t totalBackupSize;
        String lastBackupTime;
        String lastConfigChange;
        size_t configSize;
        String configVersion;
    };
    ConfigStats getStatistics();
    
private:
    void addDefaultValidationRules();
    
private:
    // Migration functions for different versions
    bool migrateFrom1_0_0_to1_1_0(DynamicJsonDocument& doc);
    bool migrateFrom1_1_0_to1_2_0(DynamicJsonDocument& doc);
    bool migrateFrom1_2_0_to2_0_0(DynamicJsonDocument& doc);
    
    // Helper functions
    JsonVariantConst getNestedValueConst(const DynamicJsonDocument& doc, const String& path) const;
    bool setNestedValue(DynamicJsonDocument& doc, const String& path, const JsonVariant& value);
    bool removeNestedValue(DynamicJsonDocument& doc, const String& path);
    void splitPath(const String& path, std::vector<String>& parts) const;
};

// Global configuration manager instance
extern ConfigManager* configManager;

// Convenience macros for configuration access
#define CONFIG_GET(path, defaultValue) \
    ({ JsonVariant _v; configManager->getConfigValue(path, _v) ? _v : JsonVariant(defaultValue); })

#define CONFIG_SET(path, value) \
    configManager->setConfigValue(path, value)

#define CONFIG_VALIDATE(doc) \
    configManager->validateConfiguration(doc)

#endif // CONFIG_MANAGER_H