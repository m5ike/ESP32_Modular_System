#include "QueueBase.h"
#include "ModuleManager.h"
#include "ModuleRegistry.h"

QueueBase::QueueBase(Module* owner, const QueueConfig& cfg) : owner_(owner), cfg_(cfg), queue_(nullptr) {}

QueueBase::~QueueBase() { destroy(); }

bool QueueBase::create() {
  if (queue_) return true;
  queue_ = xQueueCreate(cfg_.length, sizeof(QueueMessage*));
  if (queue_) ModuleRegistry::getInstance()->registerQueue(owner_->getName(), queue_);
  return queue_ != nullptr;
}

bool QueueBase::destroy() {
  if (!queue_) return true;
  vQueueDelete(queue_);
  queue_ = nullptr;
  return true;
}

bool QueueBase::send(QueueMessage* msg) {
  if (!queue_) return false;
  QueueMessage* tmp = msg;
  TickType_t to = cfg_.sendTimeoutTicks;
  return xQueueSend(queue_, &tmp, to) == pdPASS;
}

bool QueueBase::receive(QueueMessage*& out) {
  if (!queue_) return false;
  TickType_t to = cfg_.recvTimeoutTicks;
  return xQueueReceive(queue_, &out, to) == pdPASS;
}

String QueueBase::id() const { return owner_->getName(); }

void QueueBase::RECEIVE_RETURN_CALL_FUNC(QueueMessage* incoming) {
  if (!incoming) return;
  String toId = incoming->fromQueue;
  QueueHandle_t toH = ModuleRegistry::getInstance()->findQueue(toId);
  if (!toH) return;
  DynamicJsonDocument* vars = new DynamicJsonDocument(512);
  JsonArray arr = vars->createNestedArray("v");
  arr.add("RESULT");
  arr.add((*incoming->callVariables)["v"]);
  QueueMessage* resp = new QueueMessage{genUUID4(), toId, id(), EVENT_PROCESS_DONE, CALL_FUNCTION_ASYNC, String("RECEIVE_RETURN_CALL_FUNC"), vars};
  QueueMessage* tmp = resp;
  xQueueSend(toH, &tmp, cfg_.sendTimeoutTicks);
}