#include "TaskBase.h"
#include "ModuleManager.h"

TaskBase::TaskBase(Module* owner, const TaskConfig& cfg) : owner_(owner), cfg_(cfg), handle_(nullptr), running_(false) {}

TaskBase::~TaskBase() { stop(); }

bool TaskBase::start(TaskFunction_t fn) {
  if (handle_) return true;
  BaseType_t ok;
  if (cfg_.core >= 0) {
    ok = xTaskCreatePinnedToCore(fn, cfg_.name.c_str(), cfg_.stackSize, owner_, cfg_.priority, &handle_, cfg_.core);
  } else {
    ok = xTaskCreate(fn, cfg_.name.c_str(), cfg_.stackSize, owner_, cfg_.priority, &handle_);
  }
  running_ = ok == pdPASS;
  return running_;
}

bool TaskBase::stop() {
  if (!handle_) return true;
  vTaskDelete(handle_);
  handle_ = nullptr;
  running_ = false;
  return true;
}

bool TaskBase::suspend() {
  if (!handle_) return false;
  vTaskSuspend(handle_);
  return true;
}

bool TaskBase::resume() {
  if (!handle_) return false;
  vTaskResume(handle_);
  return true;
}