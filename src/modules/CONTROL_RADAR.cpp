#include "CONTROL_RADAR.h"
#include "CONTROL_LCD.h"
#include <Arduino.h>

CONTROL_RADAR::CONTROL_RADAR() : Module("CONTROL_RADAR") {
    radarInitialized = false;
    lastUpdate = 0;
    lastBlink = 0;
    ledState = false;
    component.type = RADAR_TYPE_NONE;
    component.enabled = false;
    component.trigPin = 13;
    component.echoPin = 12;
    component.ledPin = 14;
    component.speed = 100;
    component.step = 1;
    component.blinkSpeed = 500;
    priority = 50;
    autoStart = false;
    lastDistance = -1;
    lastMeasureMs = 0;
    lastSpeed = 0;
    movementDir = 0;
    setUseQueue(true);
    TaskConfig tcfg = getTaskConfig();
    tcfg.name = "CONTROL_RADAR_TASK";
    tcfg.stackSize = 4096;
    tcfg.priority = 2;
    tcfg.core = 1;
    setTaskConfig(tcfg);
    QueueConfig qcfg = getQueueConfig();
    qcfg.length = 16;
    setQueueConfig(qcfg);
}

CONTROL_RADAR::~CONTROL_RADAR() {
    stop();
}

bool CONTROL_RADAR::init() {
    setupPins();
    radarInitialized = true;
    setState(MODULE_ENABLED);
    return true;
}

bool CONTROL_RADAR::start() {
    if (!radarInitialized) return init();
    setState(MODULE_ENABLED);
    return true;
}

bool CONTROL_RADAR::stop() {
    if (component.ledPin) digitalWrite(component.ledPin, LOW);
    setState(MODULE_DISABLED);
    return true;
}

bool CONTROL_RADAR::update() {
    if (state != MODULE_ENABLED) return true;
    unsigned long now = millis();
    if (component.ledPin && now - lastBlink >= component.blinkSpeed) {
        ledState = !ledState;
        digitalWrite(component.ledPin, ledState ? HIGH : LOW);
        lastBlink = now;
    }
    if (now - lastUpdate >= component.speed) {
        long d = measureDistance();
        if (d >= 0) {
            if (lastDistance >= 0) {
                unsigned long dt = now - lastMeasureMs;
                if (dt > 0) {
                    float v = (float)(d - lastDistance) / ((float)dt / 1000.0);
                    lastSpeed = v;
                    movementDir = (v > 0) ? 1 : (v < 0 ? -1 : 0);
                }
            }
            lastDistance = d;
            lastMeasureMs = now;
            Module* lcdMod = ModuleManager::getInstance()->getModule("CONTROL_LCD");
            if (lcdMod && lcdMod->getState() == MODULE_ENABLED) {
                QueueBase* qb = lcdMod->getQueue();
                if (qb) {
                    DynamicJsonDocument* vars = new DynamicJsonDocument(256);
                    (*vars)["d"] = (int)d;
                    (*vars)["v"] = lastSpeed;
                    (*vars)["dir"] = movementDir;
                    (*vars)["type"] = (int)component.type;
                    QueueMessage* msg = new QueueMessage{genUUID4(), lcdMod->getName(), getName(), EVENT_DATA_READY, CALL_FUNCTION_ASYNC, String("lcd_radar_update"), vars};
                    qb->send(msg);
                }
            }
        }
        lastUpdate = now;
    }
    return true;
}

bool CONTROL_RADAR::test() {
    long d = measureDistance();
    return d >= 0;
}

DynamicJsonDocument CONTROL_RADAR::getStatus() {
    DynamicJsonDocument doc(512);
    doc["module"] = moduleName;
    doc["state"] = state == MODULE_ENABLED ? "enabled" : "disabled";
    doc["distance_cm"] = getDistance();
    return doc;
}

bool CONTROL_RADAR::loadConfig(DynamicJsonDocument& doc) {
    if (doc.containsKey("CONTROL_RADAR")) {
        JsonObject cfg = doc["CONTROL_RADAR"];
        component.enabled = cfg["enabled"] | false;
        component.trigPin = cfg["pin_trig"] | component.trigPin;
        component.echoPin = cfg["pin_echo"] | component.echoPin;
        component.ledPin = cfg["pin_led"] | component.ledPin;
        component.blinkSpeed = cfg["led_blink_interval"] | component.blinkSpeed;
    }
    return true;
}

void CONTROL_RADAR::setupPins() {
    if (component.trigPin) pinMode(component.trigPin, OUTPUT);
    if (component.echoPin) pinMode(component.echoPin, INPUT);
    if (component.ledPin) pinMode(component.ledPin, OUTPUT);
}

void CONTROL_RADAR::updateBlink() {
}

long CONTROL_RADAR::measureDistance() {
    if (!component.trigPin || !component.echoPin) return -1;
    digitalWrite(component.trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(component.trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(component.trigPin, LOW);
    long duration = pulseIn(component.echoPin, HIGH, 30000);
    long distance = duration / 58;
    return distance;
}

long CONTROL_RADAR::getDistance() {
    return measureDistance();
}

bool CONTROL_RADAR::setComponent(uint8_t type, uint8_t trig, uint8_t echo, uint8_t led) {
    component.type = type;
    component.trigPin = trig;
    component.echoPin = echo;
    component.ledPin = led;
    setupPins();
    return true;
}

bool CONTROL_RADAR::setSpeed(uint16_t speed) { component.speed = speed; return true; }
bool CONTROL_RADAR::setStep(uint16_t step) { component.step = step; return true; }
bool CONTROL_RADAR::setBlinkSpeed(uint16_t speed) { component.blinkSpeed = speed; return true; }