/**
 * @file FreeRTOSWatchdog.cpp
 * @brief System and task-level watchdog manager implementation.
 * @author Michael Kojdl
 * @email michael@kojdl.com
 * @date 2025-11-16
 * @version 1.0.0
 */
#include "FreeRTOSWatchdog.h"
#include "TaskBase.h"
#include "ModuleBase.h"
#include <ArduinoJson.h>
#include "esp_system.h"
#include "esp_log.h"

// Global watchdog instance
WatchdogManager* globalWatchdog = nullptr;

WatchdogManager::WatchdogManager(uint32_t timeoutMs) 
    : systemTimer(nullptr), systemTimeout(timeoutMs), initialized(false) {
}

WatchdogManager::~WatchdogManager() {
    deinitialize();
}

bool WatchdogManager::initialize() {
    if (initialized) return true;
    
    // Initialize ESP-IDF task watchdog
    esp_err_t err = esp_task_wdt_init(systemTimeout / 1000, true); // true = panic on timeout
    if (err != ESP_OK) {
        ESP_LOGE("WATCHDOG", "Failed to initialize task watchdog: %s", esp_err_to_name(err));
        return false;
    }
    
    // Create system-level software timer
    systemTimer = xTimerCreate(
        "SystemWatchdog",
        pdMS_TO_TICKS(systemTimeout),
        pdTRUE,  // Auto-reload
        this,    // Timer ID
        systemTimerCallback
    );
    
    if (!systemTimer) {
        ESP_LOGE("WATCHDOG", "Failed to create system timer");
        esp_task_wdt_deinit();
        return false;
    }
    
    initialized = true;
    ESP_LOGI("WATCHDOG", "Watchdog manager initialized with %lu ms timeout", systemTimeout);
    return true;
}

bool WatchdogManager::deinitialize() {
    if (!initialized) return true;
    
    stopSystemWatchdog();
    
    if (systemTimer) {
        xTimerDelete(systemTimer, portMAX_DELAY);
        systemTimer = nullptr;
    }
    
    esp_task_wdt_deinit();
    initialized = false;
    
    return true;
}

bool WatchdogManager::startSystemWatchdog() {
    if (!initialized || !systemTimer) return false;
    
    if (xTimerStart(systemTimer, portMAX_DELAY) != pdPASS) {
        ESP_LOGE("WATCHDOG", "Failed to start system timer");
        return false;
    }
    
    ESP_LOGI("WATCHDOG", "System watchdog started");
    return true;
}

bool WatchdogManager::stopSystemWatchdog() {
    if (!systemTimer) return false;
    
    xTimerStop(systemTimer, portMAX_DELAY);
    ESP_LOGI("WATCHDOG", "System watchdog stopped");
    return true;
}

void WatchdogManager::feedSystemWatchdog() {
    if (!initialized) return;
    
    // Reset the software timer
    if (systemTimer) {
        xTimerReset(systemTimer, 0); // Don't block
    }
    
    // Feed the ESP-IDF task watchdog for the current task
    esp_task_wdt_reset();
}

bool WatchdogManager::addTaskToWatchdog(TaskHandle_t task, uint32_t timeoutMs) {
    if (!initialized || !task) return false;
    
    esp_err_t err = esp_task_wdt_add(task);
    if (err != ESP_OK) {
        ESP_LOGE("WATCHDOG", "Failed to add task to watchdog: %s", esp_err_to_name(err));
        return false;
    }
    
    // Configure task timeout if different from system default
    if (timeoutMs != systemTimeout) {
        err = esp_task_wdt_status(task); // Check if task is subscribed
        if (err == ESP_OK) {
            // Note: ESP-IDF doesn't support per-task timeout configuration
            // All tasks use the same timeout set during initialization
            ESP_LOGW("WATCHDOG", "Per-task timeout not supported, using system timeout");
        }
    }
    
    ESP_LOGI("WATCHDOG", "Task added to watchdog: %p", task);
    return true;
}

bool WatchdogManager::removeTaskFromWatchdog(TaskHandle_t task) {
    if (!initialized || !task) return false;
    
    esp_err_t err = esp_task_wdt_delete(task);
    if (err != ESP_OK) {
        ESP_LOGE("WATCHDOG", "Failed to remove task from watchdog: %s", esp_err_to_name(err));
        return false;
    }
    
    ESP_LOGI("WATCHDOG", "Task removed from watchdog: %p", task);
    return true;
}

void WatchdogManager::feedTaskWatchdog(TaskHandle_t task) {
    if (!initialized || !task) return;
    
    // Feed the watchdog for a specific task
    esp_err_t err = esp_task_wdt_reset();
    if (err != ESP_OK) {
        // Task might not be subscribed, try to add it
        if (err == ESP_ERR_NOT_FOUND) {
            addTaskToWatchdog(task);
        }
    }
}

bool WatchdogManager::addModuleTask(ModuleBase* module) {
    if (!module) return false;
    
    TaskBase* task = module->getTask();
    if (!task) return false;
    
    TaskHandle_t handle = task->getHandle();
    if (!handle) return false;
    
    return addTaskToWatchdog(handle);
}

bool WatchdogManager::removeModuleTask(ModuleBase* module) {
    if (!module) return false;
    
    TaskBase* task = module->getTask();
    if (!task) return false;
    
    TaskHandle_t handle = task->getHandle();
    if (!handle) return false;
    
    return removeTaskFromWatchdog(handle);
}

void WatchdogManager::feedModuleTask(ModuleBase* module) {
    if (!module) return;
    
    TaskBase* task = module->getTask();
    if (!task) return;
    
    TaskHandle_t handle = task->getHandle();
    if (!handle) return;
    
    feedTaskWatchdog(handle);
}

bool WatchdogManager::isHealthy() const {
    return initialized && systemTimer != nullptr;
}

uint32_t WatchdogManager::getLastFeedTime() const {
    // This would require tracking feed times - simplified for now
    return 0;
}

String WatchdogManager::getStatusJson() const {
    DynamicJsonDocument doc(256);
    doc["initialized"] = initialized;
    doc["healthy"] = isHealthy();
    doc["system_timeout_ms"] = systemTimeout;
    doc["timer_active"] = systemTimer != nullptr;
    
    String output;
    serializeJson(doc, output);
    return output;
}

void WatchdogManager::systemTimerCallback(TimerHandle_t xTimer) {
    WatchdogManager* wd = static_cast<WatchdogManager*>(pvTimerGetTimerID(xTimer));
    if (wd) {
        wd->onSystemTimeout();
    }
}

void WatchdogManager::onSystemTimeout() {
    ESP_LOGE("WATCHDOG", "SYSTEM WATCHDOG TIMEOUT - System appears unresponsive!");
    
    // Log system state before reset
    ESP_LOGE("WATCHDOG", "Free heap: %d bytes", ESP.getFreeHeap());
    ESP_LOGE("WATCHDOG", "Uptime: %lu ms", millis());
    
    // Optional: Perform graceful shutdown sequence
    // For now, we'll let the ESP-IDF watchdog handle the reset
    
    // Trigger system reset after logging
    esp_restart();
}