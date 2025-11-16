#ifndef FREERTOS_TYPES_H
#define FREERTOS_TYPES_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

enum EventType { EVENT_NONE = 0, EVENT_DATA_READY, EVENT_PROCESS_DONE, EVENT_ACK };
enum CallType { CALL_NONE = 0, CALL_FUNCTION_SYNC, CALL_FUNCTION_ASYNC, CALL_VARIABLE_GET, CALL_VARIABLE_SET, CALL_RECEIVE_RETURN };

struct TaskConfig {
  String name;
  uint32_t stackSize;
  UBaseType_t priority;
  void* params;
  int8_t core;
};

struct QueueConfig {
  size_t length;
  size_t itemSize;
  TickType_t sendTimeoutTicks;
  TickType_t recvTimeoutTicks;
  bool allowISR;
};

struct QueueMessage {
  String eventUUID;
  String toQueue;
  String fromQueue;
  EventType eventType;
  CallType callType;
  String callName;
  DynamicJsonDocument* callVariables;
};

static inline String genUUID4() {
  uint32_t r1 = esp_random();
  uint32_t r2 = esp_random();
  uint32_t r3 = esp_random();
  uint32_t r4 = esp_random();
  char buf[37];
  snprintf(buf, sizeof(buf), "%08lx-%04lx-%04lx-%04lx-%08lx", (unsigned long)r1, (unsigned long)(r2 & 0xFFFF), (unsigned long)((r2 >> 16) & 0xFFFF), (unsigned long)(r3 & 0xFFFF), (unsigned long)r4);
  return String(buf);
}

#endif