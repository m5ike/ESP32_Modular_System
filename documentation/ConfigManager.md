# ConfigManager Documentation

## Overview

The ConfigManager is a comprehensive configuration management system for the ESP32 Modular System. It provides robust configuration loading/saving, versioning, validation, backup/recovery, and migration capabilities.

## Features

- **Schema-based validation**: Validates configurations against JSON Schema
- **Version management**: Semantic versioning with migration support
- **Backup and recovery**: Automatic backups before saves
- **Nested path access**: Access nested configuration values using dot notation
- **Statistics tracking**: Monitor configuration usage and validation failures
- **Thread-safe operations**: Mutex-protected operations for FreeRTOS environments
- **Legacy migration**: Migrate from legacy module-specific configurations

## Architecture

### Core Components

1. **ConfigManager Class**: Main configuration management interface
2. **JSON Schema**: Validation rules for configuration structure
3. **Configuration Storage**: JSON-based configuration files
4. **Backup System**: Automatic backup creation and management
5. **Migration System**: Version upgrade support

### File Structure

```
/config.json              # Main configuration file
/schema.json              # JSON Schema for validation
/backups/                 # Backup directory
  ├── config_backup_YYYYMMDD_HHMMSS.json
  └── config_backup_manual_NAME.json
/cfg/                     # Legacy module configurations (for migration)
/config/                  # Module-specific configurations
```

## Configuration Schema

The configuration follows a hierarchical structure:

```json
{
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
  },
  "modules": {
    "CONTROL_FS": {
      "state": "enabled",
      "priority": 100,
      "autostart": true,
      "test": true,
      "debug": false,
      "version": "1.0.1",
      "critical": true,
      "freertos": {
        "task": {
          "name": "CONTROL_FS_TASK",
          "stack": 4096,
          "priority": 3,
          "core": 0,
          "enabled": true
        },
        "queue": {
          "enabled": false,
          "length": 8,
          "send_timeout_ms": 1000,
          "recv_timeout_ms": 100
        }
      }
    }
    // ... other modules
  }
}
```

## API Reference

### Initialization

```cpp
#include "ConfigManager.h"

ConfigManager configManager;
String schema = readFile("/schema.json");

if (configManager.begin(schema)) {
    Serial.println("ConfigManager initialized successfully");
}
```

### Loading Configuration

```cpp
String configContent = readFile("/config.json");
ConfigValidationResult result = configManager.loadConfiguration(configContent);

if (result.isValid) {
    Serial.println("Configuration loaded successfully");
} else {
    Serial.println("Validation failed: " + result.errorMessage);
}
```

### Accessing Values

```cpp
// Get values using nested paths
String systemName = configManager.getValue("system.name");
bool debugMode = configManager.getValue("system.debug") == "true";
String wifiSSID = configManager.getValue("modules.CONTROL_WIFI.wifi.ssid");

// Set values
configManager.setValue("system.debug", "false");
configManager.setValue("modules.CONTROL_WIFI.wifi.ssid", "MyWiFi");
```

### Saving Configuration

```cpp
if (configManager.saveConfiguration()) {
    Serial.println("Configuration saved successfully");
}
```

### Backup Operations

```cpp
// Create manual backup
configManager.createBackup("manual_backup");

// Get backup information
int backupCount = configManager.getBackupCount();
String lastBackup = configManager.getLastBackupTime();
```

### Validation

```cpp
String testConfig = "{\"version\": \"2.0.0\", ...}";
ConfigValidationResult result = configManager.validateConfiguration(testConfig);

if (result.isValid) {
    Serial.println("Configuration is valid");
} else {
    Serial.println("Validation error: " + result.errorMessage);
}
```

### Statistics

```cpp
int loadCount = configManager.getLoadCount();
int saveCount = configManager.getSaveCount();
int validationFailures = configManager.getValidationFailureCount();
```

## Integration with CONTROL_FS

The ConfigManager is integrated into the CONTROL_FS module:

```cpp
#include "modules/CONTROL_FS.h"

CONTROL_FS* fsModule = new CONTROL_FS();
fsModule->init(); // Initializes ConfigManager automatically

ConfigManager* configMgr = fsModule->getConfigManager();
```

### Configuration Methods

The CONTROL_FS module provides configuration methods that use ConfigManager internally:

```cpp
// Load global configuration
DynamicJsonDocument doc(1024);
fsModule->loadGlobalConfig(doc);

// Save global configuration
fsModule->saveGlobalConfig(doc);

// Load module-specific configuration
fsModule->loadModuleConfig("CONTROL_WIFI", doc);

// Save module-specific configuration
fsModule->saveModuleConfig("CONTROL_WIFI", doc);
```

## Migration from Legacy Configurations

The system supports migrating from legacy module-specific configurations:

```cpp
// Automatically migrate legacy configs from /cfg/ directory
fsModule->migrateLegacyConfigs();
```

Legacy configurations in `/cfg/CONTROL_WIFI.json` will be migrated to the new format:

```json
// Legacy format
{
  "ssid": "MyWiFi",
  "password": "MyPassword"
}

// Migrated to new format
{
  "modules": {
    "CONTROL_WIFI": {
      "wifi": {
        "ssid": "MyWiFi",
        "password": "MyPassword"
      }
    }
  }
}
```

## Error Handling

The ConfigManager provides detailed error information:

```cpp
ConfigValidationResult result = configManager.loadConfiguration(config);

if (!result.isValid) {
    Serial.print("Error: ");
    Serial.println(result.errorMessage);
    Serial.print("Error path: ");
    Serial.println(result.errorPath);
}
```

Common error types:
- `INVALID_JSON`: Malformed JSON syntax
- `SCHEMA_VIOLATION`: Configuration doesn't match schema
- `INVALID_VERSION`: Version format is incorrect
- `REQUIRED_FIELD_MISSING`: Required field is missing
- `TYPE_MISMATCH`: Field type doesn't match schema

## Best Practices

1. **Always validate configurations**: Use schema validation before loading
2. **Create backups**: Enable automatic backups for critical configurations
3. **Use semantic versioning**: Follow semantic versioning for configuration changes
4. **Handle errors gracefully**: Check validation results and handle errors appropriately
5. **Use nested paths**: Access nested values using dot notation for clarity
6. **Monitor statistics**: Track configuration usage and validation failures

## Performance Considerations

- Configuration loading/saving is optimized for embedded systems
- Schema validation is performed only when loading configurations
- Backups are created automatically before saves (can be disabled)
- Statistics tracking has minimal overhead
- Thread-safe operations use mutex protection

## Example Usage

See the following examples:
- `/examples/ConfigManager_Example.cpp` - Basic usage examples
- `/tests/ConfigManager_Test.cpp` - Comprehensive test suite
- Integration in `main.cpp` - Real-world usage

## Future Enhancements

- Configuration encryption for sensitive data
- Remote configuration management
- Configuration templates
- Advanced migration strategies
- Configuration diff and merge
- Web-based configuration editor