/**
 * ConfigManager Example
 * 
 * This example demonstrates how to use the ConfigManager for configuration management
 * in the ESP32 Modular System.
 */

#include "ModuleManager.h"
#include "modules/CONTROL_FS.h"
#include "ConfigManager.h"

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("ConfigManager Example Starting...");
    
    // Initialize the file system module first
    CONTROL_FS* fsModule = new CONTROL_FS();
    
    if (!fsModule->init()) {
        Serial.println("Failed to initialize file system!");
        return;
    }
    
    // Get the ConfigManager instance
    ConfigManager* configManager = fsModule->getConfigManager();
    
    if (!configManager) {
        Serial.println("ConfigManager not available!");
        return;
    }
    
    Serial.println("ConfigManager initialized successfully!");
    
    // Example 1: Get configuration values
    Serial.println("\n=== Example 1: Reading Configuration ===");
    String systemName = configManager->getValue("system.name");
    Serial.println("System Name: " + systemName);
    
    bool debugMode = configManager->getValue("system.debug") == "true";
    Serial.println("Debug Mode: " + String(debugMode ? "enabled" : "disabled"));
    
    // Example 2: Set configuration values
    Serial.println("\n=== Example 2: Updating Configuration ===");
    if (configManager->setValue("system.debug", "false")) {
        Serial.println("Successfully disabled debug mode");
    }
    
    if (configManager->setValue("system.name", "MyESP32System")) {
        Serial.println("Successfully updated system name");
    }
    
    // Example 3: Get module-specific configuration
    Serial.println("\n=== Example 3: Module Configuration ===");
    String wifiSSID = configManager->getValue("modules.CONTROL_WIFI.wifi.ssid");
    Serial.println("WiFi SSID: " + wifiSSID);
    
    // Example 4: Configuration validation
    Serial.println("\n=== Example 4: Configuration Validation ===");
    String testConfig = R"({
        "version": "2.0.0",
        "system": {
            "name": "TestSystem",
            "debug": true,
            "timezone": "UTC"
        }
    })";
    
    ConfigValidationResult result = configManager->validateConfiguration(testConfig);
    if (result.isValid) {
        Serial.println("Configuration is valid!");
    } else {
        Serial.println("Configuration validation failed: " + result.errorMessage);
    }
    
    // Example 5: Configuration backup
    Serial.println("\n=== Example 5: Configuration Backup ===");
    if (configManager->createBackup("manual_backup")) {
        Serial.println("Backup created successfully!");
        Serial.println("Backup count: " + String(configManager->getBackupCount()));
    }
    
    // Example 6: Configuration statistics
    Serial.println("\n=== Example 6: Configuration Statistics ===");
    Serial.println("Load count: " + String(configManager->getLoadCount()));
    Serial.println("Save count: " + String(configManager->getSaveCount()));
    Serial.println("Validation failures: " + String(configManager->getValidationFailureCount()));
    
    // Example 7: Get configuration as JSON string
    Serial.println("\n=== Example 7: Configuration Export ===");
    String fullConfig = configManager->getConfigurationAsString();
    Serial.println("Current configuration:");
    Serial.println(fullConfig);
    
    Serial.println("\nConfigManager Example Complete!");
}

void loop() {
    // Nothing to do in this example
    delay(1000);
}