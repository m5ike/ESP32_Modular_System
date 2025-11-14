#ifndef CONTROL_RADAR_H
#define CONTROL_RADAR_H

#include "../ModuleManager.h"

// Radar types
#define RADAR_TYPE_MBT1 1
#define RADAR_TYPE_DIYW1 2
#define RADAR_TYPE_NONE 0

// Radar component structure
struct RadarComponent {
    uint8_t type;
    uint8_t trigPin;
    uint8_t echoPin;
    uint8_t ledPin;
    uint16_t speed;
    uint16_t step;
    uint16_t blinkSpeed;
    bool enabled;
};

class CONTROL_RADAR : public Module {
private:
    RadarComponent component;
    bool radarInitialized;
    unsigned long lastUpdate;
    unsigned long lastBlink;
    bool ledState;
    
    void setupPins();
    void updateBlink();
    long measureDistance();
    
public:
    CONTROL_RADAR();
    ~CONTROL_RADAR();
    
    // Module interface
    bool init() override;
    bool start() override;
    bool stop() override;
    bool update() override;
    bool test() override;
    JsonDocument getStatus() override;
    
    // Configuration
    bool loadConfig(JsonDocument& doc) override;
    bool setComponent(uint8_t type, uint8_t trig, uint8_t echo, uint8_t led);
    bool setSpeed(uint16_t speed);
    bool setStep(uint16_t step);
    bool setBlinkSpeed(uint16_t speed);
    
    // Radar control
    long getDistance(); // Returns distance in cm
    bool isObjectDetected(long threshold = 100); // threshold in cm
};

#endif
