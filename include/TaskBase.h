#ifndef TASK_BASE_H
#define TASK_BASE_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "FreeRTOSTypes.h"
#include "FreeRTOSWatchdog.h"

class Module;

/**
 * Enhanced TaskBase with integrated watchdog support
 * Provides standardized task management with health monitoring
 */
class TaskBase {
 public:
  TaskBase(Module* owner, const TaskConfig& cfg);
  ~TaskBase();
  
  bool start(TaskFunction_t fn);
  bool stop();
  bool suspend();
  bool resume();
  
  TaskHandle_t handle() const { return handle_; }
  TaskHandle_t getHandle() const { return handle_; }  // Alias for consistency
  TaskConfig config() const { return cfg_; }
  bool isRunning() const { return running_; }
  
  // Watchdog integration
  bool enableWatchdog(uint32_t timeoutMs = 2000);
  bool disableWatchdog();
  void feedWatchdog();
  bool isWatchdogEnabled() const { return watchdogEnabled_; }
  
  // Task health monitoring
  uint32_t getLastActivityTime() const { return lastActivityTime_; }
  void updateActivityTime() { lastActivityTime_ = millis(); }
  bool isHealthy() const;
  String getStatusJson() const;
  
  // Stack usage monitoring
  uint32_t getStackHighWaterMark() const;
  uint32_t getStackSize() const { return cfg_.stackSize; }
  uint32_t getStackUsed() const;
  float getStackUsagePercent() const;
  
 private:
  Module* owner_;
  TaskConfig cfg_;
  TaskHandle_t handle_;
  bool running_;
  bool watchdogEnabled_;
  uint32_t lastActivityTime_;
  uint32_t creationTime_;
  
  // Static task function wrapper for watchdog integration
  static void taskFunctionWrapper(void* parameters);
};

#endif