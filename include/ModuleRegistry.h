#ifndef MODULE_REGISTRY_H
#define MODULE_REGISTRY_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <map>
#include "freertos/queue.h"
#include "freertos/task.h"

struct JsonVarTemplate {
  String n;
  DynamicJsonDocument* v;
  String t;
  size_t s;
  uint32_t c;
};

class ModuleRegistry {
 public:
  static ModuleRegistry* getInstance();
  void registerTask(const String& moduleName, TaskHandle_t handle);
  void registerQueue(const String& moduleName, QueueHandle_t handle);
  TaskHandle_t findTask(const String& moduleName);
  QueueHandle_t findQueue(const String& moduleName);
  void setVar(const String& cls, const String& var, const JsonVarTemplate& tmpl);
  bool getVar(const String& cls, const String& var, JsonVarTemplate& out);
  void toJson(DynamicJsonDocument& doc);
  String exportJson();
  bool importJson(const String& json);
 private:
  ModuleRegistry();
  static ModuleRegistry* instance_;
  std::map<String, TaskHandle_t> tasks_;
  std::map<String, QueueHandle_t> queues_;
  std::map<String, std::map<String, JsonVarTemplate>> vars_;
};

#endif