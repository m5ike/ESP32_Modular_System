/**
 * @file TaskBase.cpp
 * @brief Task management with watchdog integration and health monitoring.
 * @author Michael Kojdl
 * @email michael@kojdl.com
 * @date 2025-11-16
 * @version 1.0.0
 */
#include "TaskBase.h"
#include "ModuleManager.h"
#include "esp_task_wdt.h"

// Task wrapper structure for enhanced functionality
struct TaskWrapper {
    TaskFunction_t originalFn;
    void* originalParams;
    TaskBase* taskBase;
    Module* owner;
};

TaskBase::TaskBase(Module* owner, const TaskConfig& cfg) 
    : owner_(owner), cfg_(cfg), handle_(nullptr), running_(false), 
      watchdogEnabled_(false), lastActivityTime_(0), creationTime_(0) {}

TaskBase::~TaskBase() { 
    disableWatchdog();
    stop(); 
}

bool TaskBase::start(TaskFunction_t fn) {
    if (handle_) return true;
    
    // Create wrapper to inject watchdog feeding
    TaskWrapper* wrapper = new TaskWrapper{fn, owner_, this, owner_};
    
    BaseType_t ok;
    if (cfg_.core >= 0) {
        ok = xTaskCreatePinnedToCore(
            taskFunctionWrapper, 
            cfg_.name.c_str(), 
            cfg_.stackSize + 1024, // Add extra stack for wrapper overhead
            wrapper, 
            cfg_.priority, 
            &handle_, 
            cfg_.core
        );
    } else {
        ok = xTaskCreate(
            taskFunctionWrapper, 
            cfg_.name.c_str(), 
            cfg_.stackSize + 1024, // Add extra stack for wrapper overhead
            wrapper, 
            cfg_.priority, 
            &handle_
        );
    }
    
    if (ok == pdPASS) {
        running_ = true;
        creationTime_ = millis();
        lastActivityTime_ = creationTime_;
        
        // Automatically enable watchdog if configured
        if (cfg_.priority >= 2) { // Only for medium+ priority tasks
            enableWatchdog();
        }
        
        ESP_LOGI("TASK", "Task started: %s (stack: %lu, prio: %u, core: %d)", 
                 cfg_.name.c_str(), cfg_.stackSize, cfg_.priority, cfg_.core);
    } else {
        delete wrapper;
        ESP_LOGE("TASK", "Failed to start task: %s", cfg_.name.c_str());
    }
    
    return running_;
}

bool TaskBase::stop() {
    if (!handle_) return true;
    
    disableWatchdog();
    
    // Get task wrapper before deletion
    TaskWrapper* wrapper = nullptr;
    vTaskSetThreadLocalStoragePointer(handle_, 0, &wrapper);
    
    vTaskDelete(handle_);
    handle_ = nullptr;
    running_ = false;
    
    // Clean up wrapper
    if (wrapper) {
        delete wrapper;
    }
    
    ESP_LOGI("TASK", "Task stopped: %s", cfg_.name.c_str());
    return true;
}

bool TaskBase::suspend() {
    if (!handle_) return false;
    vTaskSuspend(handle_);
    ESP_LOGW("TASK", "Task suspended: %s", cfg_.name.c_str());
    return true;
}

bool TaskBase::resume() {
    if (!handle_) return false;
    vTaskResume(handle_);
    ESP_LOGI("TASK", "Task resumed: %s", cfg_.name.c_str());
    return true;
}

// Watchdog integration
bool TaskBase::enableWatchdog(uint32_t timeoutMs) {
    if (!handle_ || watchdogEnabled_) return false;
    
    if (globalWatchdog && globalWatchdog->addTaskToWatchdog(handle_)) {
        watchdogEnabled_ = true;
        ESP_LOGI("TASK", "Watchdog enabled for task: %s (timeout: %lu ms)", 
                 cfg_.name.c_str(), timeoutMs);
        return true;
    }
    
    return false;
}

bool TaskBase::disableWatchdog() {
    if (!handle_ || !watchdogEnabled_) return false;
    
    if (globalWatchdog) globalWatchdog->removeTaskFromWatchdog(handle_);
    watchdogEnabled_ = false;
    ESP_LOGI("TASK", "Watchdog disabled for task: %s", cfg_.name.c_str());
    return true;
}

void TaskBase::feedWatchdog() {
    if (!handle_ || !watchdogEnabled_) return;
    
    if (globalWatchdog) globalWatchdog->feedTaskWatchdog(handle_);
    updateActivityTime();
}

bool TaskBase::isHealthy() const {
    if (!running_) return false;
    
    uint32_t currentTime = millis();
    uint32_t timeSinceActivity = currentTime - lastActivityTime_;
    
    // Consider task unhealthy if no activity for 10x the expected loop time
    // This is a heuristic - tasks should call updateActivityTime() regularly
    const uint32_t HEALTHY_TIMEOUT_MS = 30000; // 30 seconds
    
    return timeSinceActivity < HEALTHY_TIMEOUT_MS;
}

String TaskBase::getStatusJson() const {
    DynamicJsonDocument doc(256);
    doc["name"] = cfg_.name;
    doc["running"] = running_;
    doc["watchdog_enabled"] = watchdogEnabled_;
    doc["healthy"] = isHealthy();
    doc["stack_size"] = cfg_.stackSize;
    doc["priority"] = cfg_.priority;
    doc["core"] = cfg_.core;
    doc["last_activity_ms"] = millis() - lastActivityTime_;
    doc["uptime_ms"] = running_ ? millis() - creationTime_ : 0;
    doc["stack_usage_percent"] = getStackUsagePercent();
    doc["stack_high_water_mark"] = getStackHighWaterMark();
    
    String output;
    serializeJson(doc, output);
    return output;
}

uint32_t TaskBase::getStackHighWaterMark() const {
    if (!handle_) return 0;
    return uxTaskGetStackHighWaterMark(handle_);
}

uint32_t TaskBase::getStackUsed() const {
    uint32_t highWaterMark = getStackHighWaterMark();
    return cfg_.stackSize - highWaterMark;
}

float TaskBase::getStackUsagePercent() const {
    if (cfg_.stackSize == 0) return 0.0f;
    uint32_t used = getStackUsed();
    return (float)used / (float)cfg_.stackSize * 100.0f;
}

// Task wrapper function
void TaskBase::taskFunctionWrapper(void* parameters) {
    TaskWrapper* wrapper = static_cast<TaskWrapper*>(parameters);
    if (!wrapper || !wrapper->originalFn || !wrapper->taskBase) {
        ESP_LOGE("TASK", "Invalid task wrapper");
        vTaskDelete(nullptr);
        return;
    }
    
    // Store wrapper in thread-local storage for cleanup
    vTaskSetThreadLocalStoragePointer(nullptr, 0, wrapper);
    
    ESP_LOGI("TASK", "Task wrapper started: %s", wrapper->taskBase->cfg_.name.c_str());
    
    // Call original task function
    wrapper->originalFn(wrapper->originalParams);
    
    ESP_LOGI("TASK", "Task wrapper completed: %s", wrapper->taskBase->cfg_.name.c_str());
    
    // Cleanup - task should not return, but handle it gracefully
    wrapper->taskBase->running_ = false;
    vTaskDelete(nullptr);
}