/**
 * @file CONTROL_RADAR.h
 * @brief Radar control module interface.
 * @author Michael Kojdl
 * @email michael@kojdl.com
 * @date 2025-11-16
 * @version 1.0.1
 */
#ifndef CONTROL_RADAR_H
#define CONTROL_RADAR_H

#include "../ModuleManager.h"

// Radar types
#define RADAR_TYPE_MBT1 1
#define RADAR_TYPE_DIYW1 2
#define RADAR_TYPE_NONE 0

/**
 * @struct RadarComponent
 * @brief Configuration and runtime parameters for radar hardware.
 * @details Pin mapping, stepping, speeds, blink behavior, and enable flags.
 */
struct RadarComponent {
    uint8_t type;
    uint8_t trigPin;
    uint8_t echoPin;
    uint8_t ledPin;
    uint8_t mbt1Pin;
    uint8_t mbt2Pin;
    uint8_t motorStepPin;
    uint8_t motorDirPin;
    uint8_t motorPins[4];
    bool useULN2003;
    float stepDegrees;
    uint16_t speed;
    uint16_t step;
    uint16_t blinkSpeed;
    bool enabled;
};

/**
 * @class CONTROL_RADAR
 * @brief Radar sensor and motor control for object detection and measurement.
 */
/**
 * @class CONTROL_RADAR
 * @brief Manages ultrasonic radar sensor and optional stepper-driven scanning.
 * @details Handles pin setup, measurement, blinking, button input, and rotation/measure modes.
 */
class CONTROL_RADAR : public Module {
private:
    RadarComponent component;
    bool radarInitialized;
    unsigned long lastUpdate;
    unsigned long lastBlink;
    bool ledState;
    long lastDistance;
    unsigned long lastMeasureMs;
    float lastSpeed;
    int movementDir;
    int rotationMode;
    int measureMode;
    float angleDeg;
    unsigned long lastStepMs;
    bool motorDirFwd;
    int lastBtn1;
    int lastBtn2;
    unsigned long lastBtn1Ms;
    unsigned long lastBtn2Ms;
    int stepPhase;
    bool sensorPresent;
    bool stepperPresent;
    bool buttonsPresent;
    static const int SAMPLE_WINDOW = 32;
    long distSamples[SAMPLE_WINDOW];
    unsigned long timeSamples[SAMPLE_WINDOW];
    int sampleIndex;
    int sampleCount;
    float vectorVx;
    float vectorVy;
    float movementSpeedAbs;
    float avgRPS;
    float sizeEstimate;
    String shapeClass;
    void probeHardware();
    
    void setupPins();
    void updateBlink();
    long measureDistance();
    void handleButtons();
    void setRotationMode(int mode);
    void setMeasureMode(int mode);
    void blinkSignal(int count);
    void stepMotorOnce();
    void releaseStepper();
    bool isObjectDetected(long threshold = 100);
    
public:
    CONTROL_RADAR();
    ~CONTROL_RADAR();
    
    // Module interface
    bool init() override;
    bool start() override;
    bool stop() override;
    bool update() override;
    bool test() override;
    DynamicJsonDocument getStatus() override;
    
    // Configuration
    bool loadConfig(DynamicJsonDocument& doc) override;
    bool setComponent(uint8_t type, uint8_t trig, uint8_t echo, uint8_t led);
    bool setSpeed(uint16_t speed);
    bool setStep(uint16_t step);
    bool setBlinkSpeed(uint16_t speed);
    
    // Radar control
    long getDistance(); // Returns distance in cm
    bool isObjectDetected(); // Uses default threshold
    bool setStepperULN2003(uint8_t in1, uint8_t in2, uint8_t in3, uint8_t in4);
    void setRotationModePublic(int mode) { setRotationMode(mode); }
    void setMeasureModePublic(int mode) { setMeasureMode(mode); }
};

#endif
