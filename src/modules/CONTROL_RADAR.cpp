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
    component.mbt1Pin = 26;
    component.mbt2Pin = 27;
    component.motorStepPin = 0;
    component.motorDirPin = 0;
    component.motorPins[0] = 2;
    component.motorPins[1] = 4;
    component.motorPins[2] = 5;
    component.motorPins[3] = 18;
    component.useULN2003 = false;
    component.stepDegrees = 0.0879f;
    component.speed = 100;
    component.step = 1;
    component.blinkSpeed = 500;
    priority = 50;
    autoStart = true;
    lastDistance = -1;
    lastMeasureMs = 0;
    lastSpeed = 0;
    movementDir = 0;
    rotationMode = 0;
    measureMode = 0;
    angleDeg = 0.0f;
    lastStepMs = 0;
    motorDirFwd = true;
    lastBtn1 = 1;
    lastBtn2 = 1;
    lastBtn1Ms = 0;
    lastBtn2Ms = 0;
    stepPhase = 0;
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
    sensorPresent = false;
    stepperPresent = false;
    buttonsPresent = false;
    sampleIndex = 0;
    sampleCount = 0;
    vectorVx = 0.0f;
    vectorVy = 0.0f;
    movementSpeedAbs = 0.0f;
    avgRPS = 0.0f;
    sizeEstimate = 0.0f;
    shapeClass = String("unknown");
}

CONTROL_RADAR::~CONTROL_RADAR() {
    stop();
}

bool CONTROL_RADAR::init() {
    setupPins();
    probeHardware();
    // Log and display pre-start checks
    Module* lcdMod = ModuleManager::getInstance()->getModule("CONTROL_LCD");
    if (lcdMod && lcdMod->getState() == MODULE_ENABLED) {
        QueueBase* qb = lcdMod->getQueue();
        if (qb) {
            DynamicJsonDocument* vars = new DynamicJsonDocument(128);
            (*vars)["msg"] = String("RADAR probe: sensor=") + (sensorPresent?"yes":"no") + ", stepper=" + (stepperPresent?"yes":"no");
            QueueMessage* msg = new QueueMessage{genUUID4(), lcdMod->getName(), getName(), EVENT_DATA_READY, CALL_FUNCTION_ASYNC, String("lcd_log_append"), vars};
            qb->send(msg);
        }
    }
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
    if (buttonsPresent) handleButtons();
    if (stepperPresent && (rotationMode == 1 || rotationMode == 2 || rotationMode == 3)) {
        unsigned long stepInterval = rotationMode == 1 ? 10 * component.step : (rotationMode == 2 ? 3 * component.step : 6 * component.step);
        if (now - lastStepMs >= stepInterval) {
            stepMotorOnce();
            lastStepMs = now;
        }
    }
    if (sensorPresent && now - lastUpdate >= component.speed) {
        long d = measureDistance();
        if (d >= 0) {
            if (lastDistance >= 0) {
                unsigned long dt = now - lastMeasureMs;
                if (dt > 0) {
                    float v = (float)(d - lastDistance) / ((float)dt / 1000.0);
                    lastSpeed = v;
                    movementDir = (v > 0) ? 1 : (v < 0 ? -1 : 0);
                    float rad = angleDeg * 0.01745329252f;
                    vectorVx = v * cos(rad);
                    vectorVy = v * sin(rad);
                    movementSpeedAbs = v >= 0 ? v : -v;
                }
            }
            lastDistance = d;
            lastMeasureMs = now;
            distSamples[sampleIndex] = d;
            timeSamples[sampleIndex] = now;
            sampleIndex = (sampleIndex + 1) % SAMPLE_WINDOW;
            if (sampleCount < SAMPLE_WINDOW) sampleCount++;
            int cnt = 0;
            for (int i = 0; i < sampleCount; i++) {
                unsigned long t = timeSamples[i];
                if (now - t <= 1000) cnt++;
            }
            avgRPS = (float)cnt;
            float mean = 0.0f;
            for (int i = 0; i < sampleCount; i++) mean += (float)distSamples[i];
            if (sampleCount > 0) mean /= (float)sampleCount;
            float var = 0.0f;
            for (int i = 0; i < sampleCount; i++) {
                float dd = (float)distSamples[i] - mean;
                var += dd * dd;
            }
            if (sampleCount > 1) var /= (float)(sampleCount - 1);
            sizeEstimate = sqrtf(var);
            if (sizeEstimate < 2.0f) shapeClass = String("point");
            else if (sizeEstimate < 5.0f) shapeClass = String("round");
            else shapeClass = String("flat");
            Module* lcdMod = ModuleManager::getInstance()->getModule("CONTROL_LCD");
            if (lcdMod && lcdMod->getState() == MODULE_ENABLED) {
                QueueBase* qb = lcdMod->getQueue();
                if (qb) {
                    DynamicJsonDocument* vars = new DynamicJsonDocument(256);
                    (*vars)["d"] = (int)d;
                    (*vars)["v"] = measureMode == 1 ? lastSpeed : 0.0f;
                    (*vars)["dir"] = measureMode == 1 ? movementDir : 0;
                    (*vars)["type"] = (int)component.type;
                    (*vars)["ang"] = (int)angleDeg;
                    (*vars)["vx"] = vectorVx;
                    (*vars)["vy"] = vectorVy;
                    (*vars)["ms"] = movementSpeedAbs;
                    (*vars)["size"] = sizeEstimate;
                    (*vars)["shape"] = shapeClass;
                    (*vars)["avg_rps"] = avgRPS;
                    QueueMessage* msg = new QueueMessage{genUUID4(), lcdMod->getName(), getName(), EVENT_DATA_READY, CALL_FUNCTION_ASYNC, String("lcd_radar_update"), vars};
                    qb->send(msg);
                }
            }
            if (rotationMode == 3) {
                if (movementDir > 0) motorDirFwd = true; else if (movementDir < 0) motorDirFwd = false;
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
    doc["speed_cms"] = lastSpeed;
    doc["direction"] = movementDir;
    doc["angle_deg"] = (int)angleDeg;
    doc["type"] = component.type;
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
        if (cfg.containsKey("rotation_mode")) setRotationMode(cfg["rotation_mode"]);
        if (cfg.containsKey("measure_mode")) setMeasureMode(cfg["measure_mode"]);
        if (cfg.containsKey("uln")) {
            JsonObject uln = cfg["uln"];
            uint8_t in1 = uln["in1"] | 0;
            uint8_t in2 = uln["in2"] | 0;
            uint8_t in3 = uln["in3"] | 0;
            uint8_t in4 = uln["in4"] | 0;
            if (in1 && in2 && in3 && in4) setStepperULN2003(in1, in2, in3, in4);
        }
        if (cfg.containsKey("step_degrees")) component.stepDegrees = cfg["step_degrees"] | component.stepDegrees;
    }
    return true;
}

void CONTROL_RADAR::setupPins() {
    if (component.trigPin) pinMode(component.trigPin, OUTPUT);
    if (component.echoPin) pinMode(component.echoPin, INPUT);
    if (component.ledPin) pinMode(component.ledPin, OUTPUT);
    if (component.mbt1Pin) pinMode(component.mbt1Pin, INPUT_PULLUP);
    if (component.mbt2Pin) pinMode(component.mbt2Pin, INPUT_PULLUP);
    if (component.motorStepPin) pinMode(component.motorStepPin, OUTPUT);
    if (component.motorDirPin) pinMode(component.motorDirPin, OUTPUT);
    if (component.useULN2003) {
        for (int i = 0; i < 4; i++) {
            if (component.motorPins[i]) pinMode(component.motorPins[i], OUTPUT);
        }
    }
}

void CONTROL_RADAR::probeHardware() {
    sensorPresent = (component.trigPin && component.echoPin);
    // Try a quick pulse to detect echo activity
    if (sensorPresent) {
        digitalWrite(component.trigPin, LOW);
        delayMicroseconds(5);
        digitalWrite(component.trigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(component.trigPin, LOW);
        long duration = pulseIn(component.echoPin, HIGH, 15000);
        if (duration <= 0) {
            // Could be no target or sensor missing; keep sensorPresent based on pins only
        }
    }
    stepperPresent = (component.useULN2003 && component.motorPins[0] && component.motorPins[1] && component.motorPins[2] && component.motorPins[3]) ||
                     (component.motorStepPin && component.motorDirPin);
    buttonsPresent = (component.mbt1Pin || component.mbt2Pin);
    // If not present, disable respective modes to prevent operations
    if (!sensorPresent) measureMode = 0;
    if (!stepperPresent) rotationMode = 0;
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

void CONTROL_RADAR::handleButtons() {
    int b1 = component.mbt1Pin ? digitalRead(component.mbt1Pin) : 1;
    int b2 = component.mbt2Pin ? digitalRead(component.mbt2Pin) : 1;
    unsigned long now = millis();
    if (b1 == LOW && lastBtn1 == HIGH && now - lastBtn1Ms > 250) {
        int next = (rotationMode + 1) % 4;
        setRotationMode(next);
        lastBtn1Ms = now;
    }
    if (b2 == LOW && lastBtn2 == HIGH && now - lastBtn2Ms > 250) {
        int nextM = (measureMode + 1) % 2;
        setMeasureMode(nextM);
        lastBtn2Ms = now;
    }
    lastBtn1 = b1;
    lastBtn2 = b2;
}

void CONTROL_RADAR::setRotationMode(int mode) {
    rotationMode = mode;
    blinkSignal(mode == 0 ? 4 : mode);
}

void CONTROL_RADAR::setMeasureMode(int mode) {
    measureMode = mode;
    blinkSignal(mode + 1);
}

void CONTROL_RADAR::blinkSignal(int count) {
    if (!component.ledPin) return;
    for (int i = 0; i < count; i++) {
        digitalWrite(component.ledPin, HIGH);
        delay(120);
        digitalWrite(component.ledPin, LOW);
        delay(120);
    }
}

void CONTROL_RADAR::stepMotorOnce() {
    if (component.useULN2003 && component.motorPins[0] && component.motorPins[1] && component.motorPins[2] && component.motorPins[3]) {
        static const uint8_t seq[8] = {1,3,2,6,4,12,8,9};
        stepPhase = (stepPhase + (motorDirFwd ? 1 : -1) + 8) % 8;
        uint8_t pattern = seq[stepPhase];
        for (int i = 0; i < 4; i++) {
            uint8_t on = (pattern >> i) & 0x1;
            digitalWrite(component.motorPins[i], on ? HIGH : LOW);
        }
        angleDeg += motorDirFwd ? component.stepDegrees : -component.stepDegrees;
        if (angleDeg >= 360.0f) angleDeg -= 360.0f;
        if (angleDeg < 0.0f) angleDeg += 360.0f;
    } else if (component.motorStepPin && component.motorDirPin) {
        digitalWrite(component.motorDirPin, motorDirFwd ? HIGH : LOW);
        digitalWrite(component.motorStepPin, HIGH);
        delayMicroseconds(400);
        digitalWrite(component.motorStepPin, LOW);
        delayMicroseconds(400);
        angleDeg += motorDirFwd ? component.step : -component.step;
        if (angleDeg >= 360.0f) angleDeg -= 360.0f;
        if (angleDeg < 0.0f) angleDeg += 360.0f;
    }
}

void CONTROL_RADAR::releaseStepper() {
    if (component.useULN2003) {
        for (int i = 0; i < 4; i++) {
            if (component.motorPins[i]) digitalWrite(component.motorPins[i], LOW);
        }
    }
}

bool CONTROL_RADAR::isObjectDetected(long threshold) {
    long d = measureDistance();
    if (d < 0) return false;
    return d <= threshold;
}

bool CONTROL_RADAR::isObjectDetected() { return isObjectDetected(100); }

bool CONTROL_RADAR::setStepperULN2003(uint8_t in1, uint8_t in2, uint8_t in3, uint8_t in4) {
    component.motorPins[0] = in1;
    component.motorPins[1] = in2;
    component.motorPins[2] = in3;
    component.motorPins[3] = in4;
    component.useULN2003 = true;
    setupPins();
    return true;
}