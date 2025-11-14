#include "ModuleRegistry.h"

ModuleRegistry* ModuleRegistry::instance_ = nullptr;

ModuleRegistry::ModuleRegistry() {}

ModuleRegistry* ModuleRegistry::getInstance() {
  if (!instance_) instance_ = new ModuleRegistry();
  return instance_;
}

void ModuleRegistry::registerTask(const String& moduleName, TaskHandle_t handle) { tasks_[moduleName] = handle; }
void ModuleRegistry::registerQueue(const String& moduleName, QueueHandle_t handle) { queues_[moduleName] = handle; }

TaskHandle_t ModuleRegistry::findTask(const String& moduleName) {
  auto it = tasks_.find(moduleName);
  if (it == tasks_.end()) return nullptr;
  return it->second;
}

QueueHandle_t ModuleRegistry::findQueue(const String& moduleName) {
  auto it = queues_.find(moduleName);
  if (it == queues_.end()) return nullptr;
  return it->second;
}

void ModuleRegistry::setVar(const String& cls, const String& var, const JsonVarTemplate& tmpl) { vars_[cls][var] = tmpl; }

bool ModuleRegistry::getVar(const String& cls, const String& var, JsonVarTemplate& out) {
  auto ic = vars_.find(cls);
  if (ic == vars_.end()) return false;
  auto iv = ic->second.find(var);
  if (iv == ic->second.end()) return false;
  out = iv->second;
  return true;
}

void ModuleRegistry::toJson(DynamicJsonDocument& doc) {
  JsonObject root = doc.to<JsonObject>();
  JsonObject t = root.createNestedObject("t");
  for (auto& kv : tasks_) t[kv.first] = (uint32_t)kv.second;
  JsonObject q = root.createNestedObject("q");
  for (auto& kv : queues_) q[kv.first] = (uint32_t)kv.second;
  JsonObject v = root.createNestedObject("v");
  for (auto& cls : vars_) {
    JsonObject c = v.createNestedObject(cls.first);
    for (auto& item : cls.second) {
      JsonObject o = c.createNestedObject(item.first);
      o["n"] = item.second.n;
      o["t"] = item.second.t;
      o["s"] = item.second.s;
      o["c"] = item.second.c;
      String tmp; serializeJson(*(item.second.v), tmp); o["v"] = tmp;
    }
  }
}

String ModuleRegistry::exportJson() {
  DynamicJsonDocument doc(4096);
  toJson(doc);
  String s; serializeJson(doc, s);
  return s;
}

bool ModuleRegistry::importJson(const String& json) {
  DynamicJsonDocument doc(4096);
  auto err = deserializeJson(doc, json);
  if (err) return false;
  JsonObject v = doc["v"].as<JsonObject>();
  for (JsonPair cls : v) {
    String cn = cls.key().c_str();
    JsonObject ov = cls.value().as<JsonObject>();
    for (JsonPair pr : ov) {
      String vn = pr.key().c_str();
      JsonObject o = pr.value().as<JsonObject>();
      JsonVarTemplate tmpl;
      tmpl.n = o["n"].as<String>();
      tmpl.t = o["t"].as<String>();
      tmpl.s = o["s"].as<size_t>();
      tmpl.c = o["c"].as<uint32_t>();
      String vs = o["v"].as<String>();
      tmpl.v = new DynamicJsonDocument(512);
      deserializeJson(*tmpl.v, vs);
      setVar(cn, vn, tmpl);
    }
  }
  return true;
}