#ifndef CONTROL_RADAR_H
#define CONTROL_RADAR_H

#include "ModuleBase.h"

// Radar component types
enum RadarType {
    RADAR_NONE = 0,
    RADAR_MBT1 = 1,
    RADAR_DIYW1 = 2
};

class CONTROL_RADAR : public ModuleBase {
private:
    RadarType radarType;
    uint8_t pinTrig;
    uint8_t pinEcho;
    uint8_t pinLED;
    uint32_t speed;
    uint32_t step;
    uint32_t ledBlinkInterval;
    float distance;
    unsigned long lastMeasurement;
    
public:
    CONTROL_RADAR() : ModuleBase("CONTROL_RADAR"),
                      radarType(RADAR_NONE),
                      pinTrig(0),
                      pinEcho(0),
                      pinLED(0),
                      speed(100),
                      step(1),
                      ledBlinkInterval(500),
                      distance(0.0f),
                      lastMeasurement(0) {
        priority = 50;
        autostart = false;
    }
    
    bool init() override {
        log("INFO", "Initializing Radar module...");
        
        if (radarType == RADAR_NONE) {
            log("WARN", "No radar type configured");
            return true;
        }
        
        // Configure pins
        if (pinTrig > 0) pinMode(pinTrig, OUTPUT);
        if (pinEcho > 0) pinMode(pinEcho, INPUT);
        if (pinLED > 0) pinMode(pinLED, OUTPUT);
        
        return true;
    }
    
    bool start() override {
        log("INFO", "Starting Radar module...");
        return true;
    }
    
    bool stop() override {
        log("INFO", "Stopping Radar module...");
        if (pinLED > 0) digitalWrite(pinLED, LOW);
        return true;
    }
    
    bool test() override {
        log("INFO", "Testing Radar module...");
        
        if (radarType == RADAR_NONE) {
            log("WARN", "No radar configured");
            return true;
        }
        
        // Test LED blink
        if (pinLED > 0) {
            digitalWrite(pinLED, HIGH);
            delay(200);
            digitalWrite(pinLED, LOW);
        }
        
        log("INFO", "Radar test passed");
        return true;
    }
    
    void loop() override {
        if (radarType == RADAR_NONE) return;
        
        unsigned long now = millis();
        
        // Measure distance at specified speed interval
        if (now - lastMeasurement >= speed) {
            measure();
            lastMeasurement = now;
        }
        
        // Blink LED
        static unsigned long lastBlink = 0;
        static bool ledState = false;
        
        if (pinLED > 0 && now - lastBlink >= ledBlinkInterval) {
            ledState = !ledState;
            digitalWrite(pinLED, ledState ? HIGH : LOW);
            lastBlink = now;
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
        
        radarType = (RadarType)((*config)["type"] | 0);
        pinTrig = (*config)["pin_trig"] | 0;
        pinEcho = (*config)["pin_echo"] | 0;
        pinLED = (*config)["pin_led"] | 0;
        speed = (*config)["speed"] | 100;
        step = (*config)["step"] | 1;
        ledBlinkInterval = (*config)["led_blink_interval"] | 500;
        
        log("INFO", "Config loaded successfully");
        return true;
    }
    
    bool saveConfig() override {
        (*config)["type"] = (int)radarType;
        (*config)["pin_trig"] = pinTrig;
        (*config)["pin_echo"] = pinEcho;
        (*config)["pin_led"] = pinLED;
        (*config)["speed"] = speed;
        (*config)["step"] = step;
        (*config)["led_blink_interval"] = ledBlinkInterval;
        
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
    
    // Radar functions
    void measure() {
        if (pinTrig == 0 || pinEcho == 0) return;
        
        // Send trigger pulse
        digitalWrite(pinTrig, LOW);
        delayMicroseconds(2);
        digitalWrite(pinTrig, HIGH);
        delayMicroseconds(10);
        digitalWrite(pinTrig, LOW);
        
        // Read echo
        long duration = pulseIn(pinEcho, HIGH, 30000); // 30ms timeout
        
        if (duration > 0) {
            distance = duration * 0.034 / 2; // Calculate distance in cm
            
            if (debugMode) {
                logf("INFO", "Distance: %.2f cm", distance);
            }
        }
    }
    
    float getDistance() const { return distance; }
    
    String getStatusJson() override {
        DynamicJsonDocument doc(512);
        doc["name"] = moduleName;
        doc["state"] = (int)state;
        doc["type"] = (int)radarType;
        doc["distance"] = distance;
        doc["speed"] = speed;
        
        String output;
        serializeJson(doc, output);
        return output;
    }
};

#endif // CONTROL_RADAR_H
