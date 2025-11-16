#include "ModuleRegistry.h"
#include "ModuleManager.h"


ModuleRegistry* ModuleRegistry::getInstance() {
  static ModuleRegistry inst;
  return &inst;
}

ModuleRegistry::ModuleRegistry() {}

void ModuleRegistry::registerTask(const String& moduleName, TaskHandle_t handle) { tasks_[moduleName] = handle; }
void ModuleRegistry::registerQueue(const String& moduleName, QueueHandle_t handle) { queues_[moduleName] = handle; }
TaskHandle_t ModuleRegistry::findTask(const String& moduleName) { return tasks_.count(moduleName) ? tasks_[moduleName] : nullptr; }
QueueHandle_t ModuleRegistry::findQueue(const String& moduleName) { return queues_.count(moduleName) ? queues_[moduleName] : nullptr; }
void ModuleRegistry::setVar(const String& cls, const String& var, const JsonVarTemplate& tmpl) { vars_[cls][var] = tmpl; }
bool ModuleRegistry::getVar(const String& cls, const String& var, JsonVarTemplate& out) {
  auto itc = vars_.find(cls);
  if (itc == vars_.end()) return false;
  auto itv = itc->second.find(var);
  if (itv == itc->second.end()) return false;
  out = itv->second;
  return true;
}
void ModuleRegistry::toJson(DynamicJsonDocument& doc) {
  for (auto& kv : tasks_) doc["tasks"][kv.first] = (uint32_t)kv.second;
  for (auto& kv : queues_) doc["queues"][kv.first] = (uint32_t)kv.second;
}
String ModuleRegistry::exportJson() { DynamicJsonDocument d(4096); toJson(d); String s; serializeJson(d, s); return s; }
bool ModuleRegistry::importJson(const String& json) { DynamicJsonDocument d(4096); auto e = deserializeJson(d, json); return e == DeserializationError::Ok; }

String ModuleRegistry::Functions::keyOf(const String& moduleName, const String& functionName) { return moduleName + String(":") + functionName; }

bool ModuleRegistry::Functions::has(const String& key) const { return entries_.count(key) > 0; }
ModuleRegistry::FunctionEntry ModuleRegistry::Functions::get(const String& key) const { auto it = entries_.find(key); return it != entries_.end() ? it->second : FunctionEntry{}; }
std::vector<String> ModuleRegistry::Functions::listModuleFunctions(const String& moduleName) const {
  std::vector<String> names;
  String prefix = moduleName + String(":");
  for (auto& kv : entries_) {
    if (kv.first.startsWith(prefix)) names.push_back(kv.second.functionName);
  }
  return names;
}
bool ModuleRegistry::Functions::remove(const String& moduleName, const String& functionName) {
  String k = keyOf(moduleName, functionName);
  auto it = entries_.find(k);
  if (it == entries_.end()) return false;
  entries_.erase(it);
  #if (MODULE_REGISTRY_NO_DEBUG!=1)
  Serial.println(String("[ModuleRegistry][UNREGISTER] ") + moduleName + ":" + functionName);
  #endif

  return true;
}
bool ModuleRegistry::Functions::contains(const String& moduleName, const String& functionName) const {
  return entries_.count(keyOf(moduleName, functionName)) > 0;
}

bool ModuleRegistry::Functions::moduleNameFunctionRegister(const String& moduleName, const String& functionName, const String& handleName, FunctionsCallType type, std::function<bool(void*, DynamicJsonDocument*, String&)> fn, const String& evalCode) {
  FunctionEntry fe; fe.moduleName = moduleName; fe.functionName = functionName; fe.handleName = handleName; fe.callType = type; fe.func = fn; fe.evalCode = evalCode;
  String k = keyOf(moduleName, functionName);
  entries_[k] = fe;
  #if (MODULE_REGISTRY_NO_DEBUG!=1)
  Serial.println(String("[ModuleRegistry][REGISTER] ") + moduleName + ":" + functionName + String(" handle ") + handleName + String(" type ") + String((int)type));
  #endif
  return true;
}

bool ModuleRegistry::Functions::moduleNameFunctionCall(const String& moduleName, const String& functionName, DynamicJsonDocument* params, String& result) {
  String k = keyOf(moduleName, functionName);
  #if (MODULE_REGISTRY_NO_DEBUG!=1)
  if (!has(k)) { Serial.println(String("[ModuleRegistry][CALL][MISS] ") + moduleName + ":" + functionName); return false; }
  #endif
  FunctionEntry fe = get(k);
  #if (MODULE_REGISTRY_NO_DEBUG!=1)
  Serial.println(String("[ModuleRegistry][CALL] ") + moduleName + ":" + functionName + String(" type ") + String((int)fe.callType));
  #endif
  void* ctx = nullptr;
  Module* mod = ModuleManager::getInstance()->getModule(moduleName);
  ctx = (void*)mod;
  bool ok = false;
  if (fe.callType == NAME) {
    if (mod) ok = mod->callFunctionByName(fe.handleName.length() ? fe.handleName : functionName, params, result);
  } else if (fe.callType == POINTER || fe.callType == DYNAMIC) {
    if (fe.func) ok = fe.func(ctx, params, result);
  } else if (fe.callType == EVAL) {
    #if (MODULE_REGISTRY_NO_DEBUG!=1)
    Serial.println(String("[ModuleRegistry][EVAL][UNSUPPORTED] ") + moduleName + ":" + functionName);
    #endif
    ok = false;
  }
  #if (MODULE_REGISTRY_NO_DEBUG!=1)
  Serial.println(String("[ModuleRegistry][CALL][RESULT] ") + (ok ? String("OK") : String("FAIL")));
  #endif
  return ok;
}

bool ModuleRegistry::registerFunctionName(const String& moduleName, const String& functionName, const String& handleName) {
  return functions_.moduleNameFunctionRegister(moduleName, functionName, handleName, NAME, nullptr, String());
}
bool ModuleRegistry::registerFunctionPointer(const String& moduleName, const String& functionName, std::function<bool(void*, DynamicJsonDocument*, String&)> fn) {
  return functions_.moduleNameFunctionRegister(moduleName, functionName, String(), POINTER, fn, String());
}
bool ModuleRegistry::registerFunctionDynamic(const String& moduleName, const String& functionName, std::function<bool(void*, DynamicJsonDocument*, String&)> fn) {
  return functions_.moduleNameFunctionRegister(moduleName, functionName, String(), DYNAMIC, fn, String());
}
bool ModuleRegistry::registerFunctionEval(const String& moduleName, const String& functionName, const String& code) {
  return functions_.moduleNameFunctionRegister(moduleName, functionName, String(), EVAL, nullptr, code);
}
bool ModuleRegistry::callFunction(const String& moduleName, const String& functionName, DynamicJsonDocument* params, String& result) {
  return functions_.moduleNameFunctionCall(moduleName, functionName, params, result);
}

std::vector<String> ModuleRegistry::getFunctionsForModule(const String& moduleName) {
  std::vector<String> out;
  out = functions_.listModuleFunctions(moduleName);
  return out;
}

bool ModuleRegistry::unregisterFunction(const String& moduleName, const String& functionName) {
  return functions_.remove(moduleName, functionName);
}

bool ModuleRegistry::isFunctionRegistered(const String& moduleName, const String& functionName) {
  return functions_.contains(moduleName, functionName);
}