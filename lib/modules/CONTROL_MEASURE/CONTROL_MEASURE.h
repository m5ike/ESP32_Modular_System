#ifndef CONTROL_MEASURE_H
#define CONTROL_MEASURE_H

#include "ModuleBase.h"
#include <Arduino.h>
#include <queue>
#include "../../src/ModuleManager.h"
#include "../../src/modules/CONTROL_LCD.h"

// Measure component types
enum MeasureType {
    MEASURE_NONE = 0,
    MEASURE_MBT2 = 1,
    MEASURE_DIBL1 = 2
};

struct MeasureData {
    unsigned long timestamp;
    float value;
    String unit;
};

class CONTROL_MEASURE : public ModuleBase {
private:
    MeasureType measureType;
    uint8_t pinSensor;
    uint8_t pinLED;
    uint8_t pinTrig;
    uint8_t pinEcho;
    uint32_t queueSpeed;
    uint32_t ledBlinkInterval;
    std::queue<MeasureData> measureQueue;
    size_t maxQueueSize;
    
public:
    CONTROL_MEASURE() : ModuleBase("CONTROL_MEASURE"),
                        measureType(MEASURE_NONE),
                        pinSensor(0),
                        pinLED(0),
                        queueSpeed(1000),
                        ledBlinkInterval(500),
                        maxQueueSize(100) {
        priority = 50;
        autostart = true;
    }
    
    bool init() override {
        log("INFO", "Initializing Measure module...");
        
        if (measureType == MEASURE_NONE) {
            log("WARN", "No measure type configured");
            return true;
        }
        
        // Configure pins
        if (pinSensor > 0) pinMode(pinSensor, INPUT);
        if (pinLED > 0) pinMode(pinLED, OUTPUT);
        if (pinTrig > 0) pinMode(pinTrig, OUTPUT);
        if (pinEcho > 0) pinMode(pinEcho, INPUT);
        
        return true;
    }
    
    bool start() override {
        log("INFO", "Starting Measure module...");
        return true;
    }
    
    bool stop() override {
        log("INFO", "Stopping Measure module...");
        if (pinLED > 0) digitalWrite(pinLED, LOW);
        
        // Clear queue
        while (!measureQueue.empty()) {
            measureQueue.pop();
        }
        
        return true;
    }
    
    bool test() override {
        log("INFO", "Testing Measure module...");
        
        if (measureType == MEASURE_NONE) {
            log("WARN", "No measure configured");
            return true;
        }
        
        // Test LED blink
        if (pinLED > 0) {
            digitalWrite(pinLED, HIGH);
            delay(200);
            digitalWrite(pinLED, LOW);
        }
        
        log("INFO", "Measure test passed");
        return true;
    }
    
    void loop() override {
        if (measureType == MEASURE_NONE) return;
        
        unsigned long now = millis();
        
        // Take measurement at specified queue speed
        static unsigned long lastMeasure = 0;
        if (now - lastMeasure >= queueSpeed) {
            takeMeasurement();
            lastMeasure = now;
        }
        
        // Blink LED
        static unsigned long lastBlink = 0;
        static bool ledState = false;
        
        if (pinLED > 0 && now - lastBlink >= ledBlinkInterval) {
            ledState = !ledState;
            digitalWrite(pinLED, ledState ? HIGH : LOW);
            lastBlink = now;
        }

        if (measureType == MEASURE_DIBL1) {
            Module* lcdMod = ModuleManager::getInstance()->getModule("CONTROL_LCD");
            CONTROL_LCD* lcd = lcdMod ? static_cast<CONTROL_LCD*>(lcdMod) : nullptr;
            if (lcd && lcd->getState() == MODULE_ENABLED) {
                float dist = 0;
                if (!measureQueue.empty()) { dist = measureQueue.back().value; }
                TFT_eSPI* tft = lcd->getDisplay();
                if (tft) {
                    int16_t top = 60;
                    int16_t h = 180;
                    tft->fillRect(0, top, LCD_WIDTH, h, TFT_BLACK);
                    lcd->drawCenteredText(top + 20, String("Distance ") + String(dist,1) + " cm", TFT_WHITE, 2);
                }
            }
        }
    }
    
    bool loadConfig() override {
        if (!SPIFFS.exists(configPath.c_str())) {
            log("WARN", "Config file not found, creating default");
            return saveConfig();
        }
        
        File file = SPIFFS.open(configPath.c_str(), "r");
        if (!file) {
            log("ERROR", "Failed to open config file");
            return false;
        }
        
        DeserializationError error = deserializeJson(*config, file);
        file.close();
        
        if (error) {
            logf("ERROR", "Failed to parse config: %s", error.c_str());
            return false;
        }
        
        measureType = (MeasureType)((*config)["type"] | 0);
        pinSensor = (*config)["pin_sensor"] | 0;
        pinLED = (*config)["pin_led"] | 0;
        pinTrig = (*config)["pin_trig"] | 0;
        pinEcho = (*config)["pin_echo"] | 0;
        queueSpeed = (*config)["queue_speed"] | 1000;
        ledBlinkInterval = (*config)["led_blink_interval"] | 500;
        maxQueueSize = (*config)["max_queue_size"] | 100;
        
        log("INFO", "Config loaded successfully");
        return true;
    }
    
    bool saveConfig() override {
        (*config)["type"] = (int)measureType;
        (*config)["pin_sensor"] = pinSensor;
        (*config)["pin_led"] = pinLED;
        (*config)["queue_speed"] = queueSpeed;
        (*config)["led_blink_interval"] = ledBlinkInterval;
        (*config)["max_queue_size"] = maxQueueSize;
        
        File file = SPIFFS.open(configPath.c_str(), "w");
        if (!file) {
            log("ERROR", "Failed to create config file");
            return false;
        }
        
        serializeJson(*config, file);
        file.close();
        
        log("INFO", "Config saved successfully");
        return true;
    }
    
    // Measure functions
    void takeMeasurement() {
        if (pinSensor == 0) return;
        
        MeasureData data;
        data.timestamp = millis();
        
        if (measureType == MEASURE_DIBL1 && pinTrig > 0 && pinEcho > 0) {
            digitalWrite(pinTrig, LOW);
            delayMicroseconds(2);
            digitalWrite(pinTrig, HIGH);
            delayMicroseconds(10);
            digitalWrite(pinTrig, LOW);
            long duration = pulseIn(pinEcho, HIGH, 30000);
            data.value = duration / 58.0;
            data.unit = "cm";
        } else {
            int rawValue = analogRead(pinSensor);
            data.value = (rawValue / 4095.0) * 3.3;
            data.unit = "V";
        }
        
        // Add to queue
        measureQueue.push(data);
        
        // Limit queue size
        while (measureQueue.size() > maxQueueSize) {
            measureQueue.pop();
        }
        
        if (debugMode) {
            logf("INFO", "Measurement: %.3f %s", data.value, data.unit.c_str());
        }
    }
    
    bool getLatestMeasurement(MeasureData& data) {
        if (measureQueue.empty()) return false;
        
        data = measureQueue.back();
        return true;
    }
    
    size_t getQueueSize() const { return measureQueue.size(); }
    
    String getStatusJson() override {
        DynamicJsonDocument doc(1024);
        doc["name"] = moduleName;
        doc["state"] = (int)state;
        doc["type"] = (int)measureType;
        doc["queue_size"] = measureQueue.size();
        doc["queue_speed"] = queueSpeed;
        
        // Add latest measurement if available
        if (!measureQueue.empty()) {
            MeasureData latest = measureQueue.back();
            JsonObject latestObj = doc.createNestedObject("latest");
            latestObj["timestamp"] = latest.timestamp;
            latestObj["value"] = latest.value;
            latestObj["unit"] = latest.unit;
        }
        
        String output;
        serializeJson(doc, output);
        return output;
    }
};

#endif // CONTROL_MEASURE_H
