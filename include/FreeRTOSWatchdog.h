#ifndef FREERTOS_WATCHDOG_H
#define FREERTOS_WATCHDOG_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_task_wdt.h"

class ModuleBase;

/**
 * FreeRTOS Watchdog Timer Manager
 * Provides task-level and system-level watchdog functionality
 */
class WatchdogManager {
private:
    static constexpr uint32_t DEFAULT_TIMEOUT_MS = 5000;  // 5 seconds default
    static constexpr uint32_t TASK_TIMEOUT_MS = 2000;     // 2 seconds for individual tasks
    
    TimerHandle_t systemTimer;
    uint32_t systemTimeout;
    bool initialized;
    
public:
    WatchdogManager(uint32_t timeoutMs = DEFAULT_TIMEOUT_MS);
    ~WatchdogManager();
    
    bool initialize();
    bool deinitialize();
    
    // System-level watchdog
    bool startSystemWatchdog();
    bool stopSystemWatchdog();
    void feedSystemWatchdog();
    
    // Task-level watchdog
    bool addTaskToWatchdog(TaskHandle_t task, uint32_t timeoutMs = TASK_TIMEOUT_MS);
    bool removeTaskFromWatchdog(TaskHandle_t task);
    void feedTaskWatchdog(TaskHandle_t task);
    
    // Module-level convenience functions
    bool addModuleTask(ModuleBase* module);
    bool removeModuleTask(ModuleBase* module);
    void feedModuleTask(ModuleBase* module);
    
    // Status and diagnostics
    bool isHealthy() const;
    uint32_t getLastFeedTime() const;
    String getStatusJson() const;
    
private:
    static void systemTimerCallback(TimerHandle_t xTimer);
    void onSystemTimeout();
};

// Global watchdog instance
extern WatchdogManager* globalWatchdog;

// Convenience macros
#define WATCHDOG_INIT() if(!globalWatchdog) globalWatchdog = new WatchdogManager()
#define WATCHDOG_START() if(globalWatchdog) globalWatchdog->startSystemWatchdog()
#define WATCHDOG_STOP() if(globalWatchdog) globalWatchdog->stopSystemWatchdog()
#define WATCHDOG_FEED() if(globalWatchdog) globalWatchdog->feedSystemWatchdog()
#define WATCHDOG_ADD_TASK(task) if(globalWatchdog) globalWatchdog->addTaskToWatchdog(task)
#define WATCHDOG_REMOVE_TASK(task) if(globalWatchdog) globalWatchdog->removeTaskFromWatchdog(task)
#define WATCHDOG_FEED_TASK(task) if(globalWatchdog) globalWatchdog->feedTaskWatchdog(task)
#define WATCHDOG_ADD_MODULE(module) if(globalWatchdog) globalWatchdog->addModuleTask(module)
#define WATCHDOG_REMOVE_MODULE(module) if(globalWatchdog) globalWatchdog->removeModuleTask(module)
#define WATCHDOG_FEED_MODULE(module) if(globalWatchdog) globalWatchdog->feedModuleTask(module)

#endif // FREERTOS_WATCHDOG_H