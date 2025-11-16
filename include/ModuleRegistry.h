#ifndef MODULE_REGISTRY_H
#define MODULE_REGISTRY_H

#define MODULE_REGISTRY_NO_DEBUG 1

#include <Arduino.h>
#include <ArduinoJson.h>
#include <map>
#include <vector>
#include <functional>
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
  enum FunctionsCallType { NAME = 0, POINTER = 1, DYNAMIC = 2, EVAL = 3 };
  std::vector<String> getFunctionsForModule(const String& moduleName);
  bool registerFunctionName(const String& moduleName, const String& functionName, const String& handleName);
  bool registerFunctionPointer(const String& moduleName, const String& functionName, std::function<bool(void*, DynamicJsonDocument*, String&)> fn);
  bool registerFunctionDynamic(const String& moduleName, const String& functionName, std::function<bool(void*, DynamicJsonDocument*, String&)> fn);
  bool registerFunctionEval(const String& moduleName, const String& functionName, const String& code);
  bool callFunction(const String& moduleName, const String& functionName, DynamicJsonDocument* params, String& result);
  bool unregisterFunction(const String& moduleName, const String& functionName);
  bool isFunctionRegistered(const String& moduleName, const String& functionName);
 private:
  ModuleRegistry();
  static ModuleRegistry* instance_;
  std::map<String, TaskHandle_t> tasks_;
  std::map<String, QueueHandle_t> queues_;
  std::map<String, std::map<String, JsonVarTemplate>> vars_;
  struct FunctionEntry {
    String moduleName;
    String functionName;
    String handleName;
    FunctionsCallType callType;
    std::function<bool(void*, DynamicJsonDocument*, String&)> func;
    String evalCode;
  };
 public:
  class Functions {
   public:
    bool moduleNameFunctionRegister(const String& moduleName, const String& functionName, const String& handleName, FunctionsCallType type, std::function<bool(void*, DynamicJsonDocument*, String&)> fn, const String& evalCode);
    bool moduleNameFunctionCall(const String& moduleName, const String& functionName, DynamicJsonDocument* params, String& result);
    bool has(const String& key) const;
    FunctionEntry get(const String& key) const;
    std::vector<String> listModuleFunctions(const String& moduleName) const;
    bool remove(const String& moduleName, const String& functionName);
    bool contains(const String& moduleName, const String& functionName) const;
   private:
    std::map<String, FunctionEntry> entries_;
    static String keyOf(const String& moduleName, const String& functionName);
  };
  Functions functions_;
};

#endif