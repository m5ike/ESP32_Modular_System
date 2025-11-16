#ifndef QUEUE_BASE_H
#define QUEUE_BASE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "FreeRTOSTypes.h"

class Module;

class QueueBase {
 public:
  QueueBase(Module* owner, const QueueConfig& cfg);
  ~QueueBase();

  bool create();
  bool destroy();
  bool send(QueueMessage* msg);
  bool receive(QueueMessage*& out);
  String id() const;
  void RECEIVE_RETURN_CALL_FUNC(QueueMessage* incoming);

 private:
  Module* owner_;
  QueueConfig cfg_;
  QueueHandle_t queue_;
};

#endif
