/**
 * ConfigManager Test Suite
 * 
 * Comprehensive tests for the ConfigManager functionality
 */

#include "ModuleManager.h"
#include "modules/CONTROL_FS.h"
#include "ConfigManager.h"

struct TestResult {
    String name;
    bool passed;
    String message;
};

class ConfigManagerTest {
private:
    CONTROL_FS* fsModule;
    ConfigManager* configManager;
    std::vector<TestResult> results;
    
    void addResult(const String& name, bool passed, const String& message = "") {
        results.push_back({name, passed, message});
    }
    
    bool testInitialization() {
        Serial.println("Testing ConfigManager initialization...");
        
        if (!configManager) {
            addResult("Initialization", false, "ConfigManager is null");
            return false;
        }
        
        String version = configManager->getVersion();
        if (version.length() == 0) {
            addResult("Initialization", false, "Version is empty");
            return false;
        }
        
        addResult("Initialization", true, "Version: " + version);
        return true;
    }
    
    bool testConfigurationLoading() {
        Serial.println("Testing configuration loading...");
        
        // Test loading valid configuration
        String validConfig = R"({
            "version": "2.0.0",
            "system": {
                "name": "TestSystem",
                "debug": true,
                "timezone": "UTC"
            },
            "filesystem": {
                "max_size": 2097152,
                "log_max_size": 1048576
            }
        })";
        
        ConfigValidationResult result = configManager->loadConfiguration(validConfig);
        if (!result.isValid) {
            addResult("Load Valid Config", false, result.errorMessage);
            return false;
        }
        
        addResult("Load Valid Config", true, "Configuration loaded successfully");
        return true;
    }
    
    bool testValueAccess() {
        Serial.println("Testing value access...");
        
        // Test getting values
        String systemName = configManager->getValue("system.name");
        if (systemName != "TestSystem") {
            addResult("Get Value", false, "Expected 'TestSystem', got '" + systemName + "'");
            return false;
        }
        
        // Test setting values
        if (!configManager->setValue("system.debug", "false")) {
            addResult("Set Value", false, "Failed to set debug value");
            return false;
        }
        
        String debugValue = configManager->getValue("system.debug");
        if (debugValue != "false") {
            addResult("Set Value", false, "Expected 'false', got '" + debugValue + "'");
            return false;
        }
        
        addResult("Value Access", true, "All value operations successful");
        return true;
    }
    
    bool testNestedPaths() {
        Serial.println("Testing nested path access...");
        
        // Test deeply nested paths
        if (!configManager->setValue("modules.CONTROL_WIFI.wifi.ssid", "TestWiFi")) {
            addResult("Nested Path", false, "Failed to set nested value");
            return false;
        }
        
        String wifiSSID = configManager->getValue("modules.CONTROL_WIFI.wifi.ssid");
        if (wifiSSID != "TestWiFi") {
            addResult("Nested Path", false, "Expected 'TestWiFi', got '" + wifiSSID + "'");
            return false;
        }
        
        addResult("Nested Path", true, "Nested path access successful");
        return true;
    }
    
    bool testValidation() {
        Serial.println("Testing configuration validation...");
        
        // Test invalid configuration
        String invalidConfig = R"({
            "version": "invalid",
            "system": {
                "debug": "not_boolean"
            }
        })";
        
        ConfigValidationResult result = configManager->validateConfiguration(invalidConfig);
        if (result.isValid) {
            addResult("Validation", false, "Invalid configuration passed validation");
            return false;
        }
        
        addResult("Validation", true, "Invalid configuration correctly rejected");
        return true;
    }
    
    bool testBackupSystem() {
        Serial.println("Testing backup system...");
        
        int initialBackupCount = configManager->getBackupCount();
        
        if (!configManager->createBackup("test_backup")) {
            addResult("Backup", false, "Failed to create backup");
            return false;
        }
        
        int newBackupCount = configManager->getBackupCount();
        if (newBackupCount != initialBackupCount + 1) {
            addResult("Backup", false, "Backup count not incremented");
            return false;
        }
        
        addResult("Backup", true, "Backup created successfully");
        return true;
    }
    
    bool testStatistics() {
        Serial.println("Testing statistics...");
        
        // Perform some operations to generate statistics
        configManager->getValue("system.name");
        configManager->setValue("system.debug", "true");
        configManager->saveConfiguration();
        
        int loadCount = configManager->getLoadCount();
        int saveCount = configManager->getSaveCount();
        
        if (loadCount < 1 || saveCount < 1) {
            addResult("Statistics", false, "Statistics not properly tracked");
            return false;
        }
        
        addResult("Statistics", true, "Load: " + String(loadCount) + ", Save: " + String(saveCount));
        return true;
    }
    
    bool testConfigurationExport() {
        Serial.println("Testing configuration export...");
        
        String configStr = configManager->getConfigurationAsString();
        if (configStr.length() == 0) {
            addResult("Export", false, "Exported configuration is empty");
            return false;
        }
        
        // Verify it's valid JSON
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, configStr);
        if (error) {
            addResult("Export", false, "Exported configuration is invalid JSON");
            return false;
        }
        
        addResult("Export", true, "Configuration exported successfully");
        return true;
    }
    
public:
    ConfigManagerTest() : fsModule(nullptr), configManager(nullptr) {}
    
    void runTests() {
        Serial.println("\n========================================");
        Serial.println("ConfigManager Test Suite");
        Serial.println("========================================\n");
        
        // Initialize file system
        Serial.println("Initializing file system...");
        fsModule = new CONTROL_FS();
        
        if (!fsModule->init()) {
            Serial.println("Failed to initialize file system!");
            addResult("File System Init", false, "Failed to initialize file system");
            return;
        }
        
        configManager = fsModule->getConfigManager();
        
        // Run all tests
        testInitialization();
        testConfigurationLoading();
        testValueAccess();
        testNestedPaths();
        testValidation();
        testBackupSystem();
        testStatistics();
        testConfigurationExport();
        
        // Print results
        Serial.println("\n========================================");
        Serial.println("Test Results");
        Serial.println("========================================\n");
        
        int passed = 0;
        int total = results.size();
        
        for (const auto& result : results) {
            Serial.print(result.name + ": ");
            if (result.passed) {
                Serial.print("PASS");
                passed++;
            } else {
                Serial.print("FAIL");
            }
            if (result.message.length() > 0) {
                Serial.print(" - " + result.message);
            }
            Serial.println();
        }
        
        Serial.println("\n========================================");
        Serial.println("Summary: " + String(passed) + "/" + String(total) + " tests passed");
        Serial.println("========================================\n");
    }
};

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    ConfigManagerTest testSuite;
    testSuite.runTests();
}

void loop() {
    // Nothing to do
    delay(1000);
}