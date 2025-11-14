#ifndef TASK_BASE_H
#define TASK_BASE_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "FreeRTOSTypes.h"

class Module;

class TaskBase {
 public:
  TaskBase(Module* owner, const TaskConfig& cfg);
  ~TaskBase();
  bool start(TaskFunction_t fn);
  bool stop();
  bool suspend();
  bool resume();
  TaskHandle_t handle() const { return handle_; }
  TaskConfig config() const { return cfg_; }
  bool isRunning() const { return running_; }
 private:
  Module* owner_;
  TaskConfig cfg_;
  TaskHandle_t handle_;
  bool running_;
};

#endif