# Developer Handbook - ESP32 Modular System

## Table of Contents

1. [Introduction](#introduction)
2. [Development Environment Setup](#development-environment-setup)
3. [Module Development Guide](#module-development-guide)
4. [Creating Your First Module](#creating-your-first-module)
5. [FreeRTOS Task Implementation](#freertos-task-implementation)
6. [Queue-Based Communication](#queue-based-communication)
7. [Configuration Management](#configuration-management)
8. [Testing and Debugging](#testing-and-debugging)
9. [Best Practices](#best-practices)
10. [Common Patterns](#common-patterns)

---

## Introduction

This handbook provides comprehensive guidance for developers who want to:
- Create new modules for the ESP32 Modular System
- Understand the modular architecture
- Implement proper FreeRTOS task management
- Use inter-module communication effectively
- Follow best practices for embedded systems

### Prerequisites

**Knowledge Requirements**:
- C++ programming (C++11 or later)
- ESP32 architecture basics
- FreeRTOS fundamentals
- JSON data structures
- Basic embedded systems concepts

**Tools Required**:
- Visual Studio Code
- PlatformIO
- Git
- Serial terminal application (PuTTY, minicom, or built-in)

---

## Development Environment Setup

### 1. Install Required Software

**Visual Studio Code**:

```bash
# Download from https://code.visualstudio.com/
# Install PlatformIO extension from Extensions marketplace
```

**Git**:
```bash
# macOS
brew install git

# Linux
sudo apt-get install git

# Windows
# Download from https://git-scm.com/
```

### 2. Clone and Setup Project

```bash
# Clone repository
git clone <repository-url>
cd ESP32_Modular_System

# Open in VS Code
code .

# PlatformIO will automatically install dependencies
# Wait for installation to complete
```

### 3. Verify Build

```bash
# Build project
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor
```

---

## Module Development Guide

### Module Anatomy

Every module in the system consists of:

```
MODULE_NAME/
├── MODULE_NAME.h          # Header file with class declaration
├── MODULE_NAME.cpp        # Implementation file
└── config/
    └── MODULE_NAME.json   # Default configuration
```

### Module Class Structure

```cpp
// MODULE_NAME.h
#ifndef MODULE_NAME_H
#define MODULE_NAME_H

#include "ModuleBase.h"

class MODULE_NAME : public ModuleBase {
private:
    // Module-specific private members
    bool initialized;
    // Hardware resources
    // State variables
    
    // Private helper functions
    void internalFunction();
    
public:
    // Constructor
    MODULE_NAME();
    
    // Destructor
    ~MODULE_NAME();
    
    // Required Module Interface (from ModuleBase)
    bool init() override;
    bool start() override;
    bool stop() override;
    bool test() override;
    void loop() override;
    bool loadConfig() override;
    bool saveConfig() override;
    
    // Module-specific public interface
    void moduleSpecificFunction();
    
    // Queue message handler
    void handleQueueMessage(QueueMessage* msg);
};

#endif
```

---

## Creating Your First Module

### Step-by-Step: Creating CONTROL_BUZZER Module

Let's create a complete module for controlling a piezo buzzer.

#### Step 1: Create Module Files

**Create header file**: `/src/modules/CONTROL_BUZZER.h`

```cpp
#ifndef CONTROL_BUZZER_H
#define CONTROL_BUZZER_H

#include "ModuleBase.h"

// Buzzer pin definition
#define BUZZER_PIN 16

// Tone frequencies (Hz)
#define TONE_C4  262
#define TONE_D4  294
#define TONE_E4  330
#define TONE_F4  349
#define TONE_G4  392
#define TONE_A4  440
#define TONE_B4  494

/**
 * @brief Buzzer Control Module
 * 
 * Controls a piezo buzzer for audio feedback
 * Features:
 * - Tone generation
 * - Melody playback
 * - Beep patterns
 * - Volume control (PWM)
 */
class CONTROL_BUZZER : public ModuleBase {
private:
    bool buzzerInitialized;
    uint8_t currentVolume;  // 0-255
    int currentChannel;      // PWM channel
    bool isPlaying;
    
    // Helper functions
    void playToneInternal(uint16_t frequency, uint32_t duration);
    void stopTone();
    
public:
    CONTROL_BUZZER();
    ~CONTROL_BUZZER();
    
    // Module interface implementation
    bool init() override;
    bool start() override;
    bool stop() override;
    bool test() override;
    void loop() override;
    bool loadConfig() override;
    bool saveConfig() override;
    
    // Buzzer-specific functions
    void beep(uint32_t duration = 100);
    void beepPattern(uint8_t count, uint32_t duration, uint32_t pause);
    void playTone(uint16_t frequency, uint32_t duration);
    void playMelody(uint16_t* frequencies, uint32_t* durations, uint8_t length);
    void setVolume(uint8_t volume);
    uint8_t getVolume() const { return currentVolume; }
    
    // Queue message handler
    void handleQueueMessage(QueueMessage* msg);
    
    // Status
    bool isActive() const { return isPlaying; }
};

#endif // CONTROL_BUZZER_H
```

#### Step 2: Implement Module

**Create implementation file**: `/src/modules/CONTROL_BUZZER.cpp`

```cpp
#include "CONTROL_BUZZER.h"

// Constructor
CONTROL_BUZZER::CONTROL_BUZZER() : ModuleBase("CONTROL_BUZZER") {
    buzzerInitialized = false;
    currentVolume = 128;  // 50% volume
    currentChannel = 0;   // Use PWM channel 0
    isPlaying = false;
    
    // Set module properties
    priority = 60;        // Medium priority
    autostart = true;     // Start automatically
    version = "1.0.0";
    
    // Configure FreeRTOS task
    taskCfg.name = "BUZZER_TASK";
    taskCfg.stackSize = 2048;  // 2KB stack (buzzer is simple)
    taskCfg.priority = 2;       // Low task priority
    taskCfg.core = 1;           // Application CPU
    
    // Configure queue
    useQueue = true;
    queueCfg.length = 8;
    queueCfg.itemSize = sizeof(QueueMessage*);
    queueCfg.sendTimeoutTicks = pdMS_TO_TICKS(100);
    queueCfg.recvTimeoutTicks = pdMS_TO_TICKS(100);
}

// Destructor
CONTROL_BUZZER::~CONTROL_BUZZER() {
    stop();
}

// Initialize module
bool CONTROL_BUZZER::init() {
    log("Initializing buzzer...");
    
    // Configure buzzer pin
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    
    // Configure PWM channel for buzzer
    // Channel, Frequency, Resolution
    ledcSetup(currentChannel, 5000, 8);  // 8-bit resolution
    ledcAttachPin(BUZZER_PIN, currentChannel);
    ledcWrite(currentChannel, 0);  // Start silent
    
    buzzerInitialized = true;
    setState(MODULE_ENABLED);
    
    log("Buzzer initialized");
    return true;
}

// Start module
bool CONTROL_BUZZER::start() {
    if (!buzzerInitialized) {
        if (!init()) return false;
    }
    
    log("Starting buzzer module...");
    setState(MODULE_RUNNING);
    
    // Play startup beep
    beep(50);
    
    log("Buzzer module started");
    return true;
}

// Stop module
bool CONTROL_BUZZER::stop() {
    log("Stopping buzzer...");
    
    stopTone();
    ledcWrite(currentChannel, 0);
    digitalWrite(BUZZER_PIN, LOW);
    
    setState(MODULE_ENABLED);
    log("Buzzer stopped");
    return true;
}

// Test module
bool CONTROL_BUZZER::test() {
    log("Testing buzzer...");
    
    if (!buzzerInitialized) {
        log("Buzzer not initialized", "ERROR");
        return false;
    }
    
    // Play test melody
    uint16_t testMelody[] = {TONE_C4, TONE_E4, TONE_G4};
    uint32_t testDurations[] = {200, 200, 200};
    playMelody(testMelody, testDurations, 3);
    
    log("Buzzer test completed");
    return true;
}

// Main loop (runs in FreeRTOS task)
void CONTROL_BUZZER::loop() {
    // Process queue messages
    if (queueBase && useQueue) {
        QueueMessage* msg = nullptr;
        if (queueBase->receive(msg)) {
            handleQueueMessage(msg);
            
            // Cleanup
            if (msg->callVariables) {
                delete msg->callVariables;
            }
            delete msg;
        }
    }
    
    // Nothing to do in continuous loop for buzzer
    // All operations are event-driven via queue
}

// Load configuration
bool CONTROL_BUZZER::loadConfig() {
    // Load from filesystem
    // Implementation depends on your config structure
    log("Loading buzzer configuration...");
    
    // Example: Load volume from config
    if (config && config->containsKey("volume")) {
        currentVolume = (*config)["volume"];
    }
    
    return true;
}

// Save configuration
bool CONTROL_BUZZER::saveConfig() {
    log("Saving buzzer configuration...");
    
    // Save current settings to config
    if (config) {
        (*config)["volume"] = currentVolume;
    }
    
    // Save to filesystem via FS module
    // Implementation depends on your setup
    
    return true;
}

// Buzzer-specific functions

void CONTROL_BUZZER::beep(uint32_t duration) {
    playTone(TONE_A4, duration);
}

void CONTROL_BUZZER::beepPattern(uint8_t count, uint32_t duration, uint32_t pause) {
    for (uint8_t i = 0; i < count; i++) {
        beep(duration);
        if (i < count - 1) {
            delay(pause);
        }
    }
}

void CONTROL_BUZZER::playTone(uint16_t frequency, uint32_t duration) {
    if (!buzzerInitialized) return;
    
    playToneInternal(frequency, duration);
}

void CONTROL_BUZZER::playMelody(uint16_t* frequencies, uint32_t* durations, uint8_t length) {
    if (!buzzerInitialized) return;
    
    for (uint8_t i = 0; i < length; i++) {
        playToneInternal(frequencies[i], durations[i]);
        delay(50);  // Small pause between notes
    }
}

void CONTROL_BUZZER::setVolume(uint8_t volume) {
    currentVolume = volume;
    log("Volume set to " + String(volume));
}

// Private helper functions

void CONTROL_BUZZER::playToneInternal(uint16_t frequency, uint32_t duration) {
    isPlaying = true;
    
    // Set frequency
    ledcWriteTone(currentChannel, frequency);
    
    // Set volume (duty cycle)
    ledcWrite(currentChannel, currentVolume);
    
    // Wait for duration
    delay(duration);
    
    // Stop tone
    stopTone();
}

void CONTROL_BUZZER::stopTone() {
    ledcWrite(currentChannel, 0);
    isPlaying = false;
}

// Queue message handler
void CONTROL_BUZZER::handleQueueMessage(QueueMessage* msg) {
    if (!msg) return;
    
    log("Received message: " + msg->callName);
    
    if (msg->callType == CALL_FUNCTION_ASYNC) {
        // Handle async function calls
        if (msg->callName == "beep") {
            uint32_t duration = 100;
            if (msg->callVariables && msg->callVariables->containsKey("duration")) {
                duration = (*msg->callVariables)["duration"];
            }
            beep(duration);
            
        } else if (msg->callName == "play_tone") {
            if (msg->callVariables) {
                uint16_t freq = (*msg->callVariables)["frequency"];
                uint32_t dur = (*msg->callVariables)["duration"];
                playTone(freq, dur);
            }
            
        } else if (msg->callName == "set_volume") {
            if (msg->callVariables) {
                uint8_t vol = (*msg->callVariables)["volume"];
                setVolume(vol);
            }
        }
        
    } else if (msg->callType == CALL_VARIABLE_SET) {
        // Handle variable set
        if (msg->callName == "volume" && msg->callVariables) {
            currentVolume = (*msg->callVariables)["value"];
        }
    }
}
```


#### Step 3: Create Configuration File

**Create**: `/data/config/CONTROL_BUZZER.json`

```json
{
  "enabled": true,
  "volume": 128,
  "default_tone": 440,
  "default_duration": 100,
  "startup_beep": true,
  "pins": {
    "buzzer": 16
  },
  "pwm": {
    "channel": 0,
    "frequency": 5000,
    "resolution": 8
  }
}
```

#### Step 4: Register Module in Main

**Edit**: `/src/main.cpp`

```cpp
#include "modules/CONTROL_BUZZER.h"

// Add module instance declaration
CONTROL_BUZZER* buzzerModule = nullptr;

void setup() {
    // ... existing setup code ...
    
    // Create and register buzzer module
    buzzerModule = new CONTROL_BUZZER();
    moduleManager->registerModule(buzzerModule);
    
    // ... rest of setup ...
}
```

#### Step 5: Add to Global Configuration

**Edit**: `/data/config/global.json`

```json
{
  "modules": {
    "CONTROL_BUZZER": {
      "state": "enabled",
      "priority": 60,
      "autostart": true,
      "test": true,
      "debug": false,
      "version": "1.0.0"
    }
  }
}
```

#### Step 6: Build and Test

```bash
# Build the project
pio run

# Upload filesystem (includes config)
pio run --target uploadfs

# Upload code
pio run --target upload

# Monitor output
pio device monitor
```

You should see:
```
[CONTROL_BUZZER][INFO] Initializing buzzer...
[CONTROL_BUZZER][INFO] Buzzer initialized
[CONTROL_BUZZER][INFO] Starting buzzer module...
[CONTROL_BUZZER][INFO] Buzzer module started
```

#### Step 7: Test via Serial

```
> module test CONTROL_BUZZER
[CONTROL_BUZZER][INFO] Testing buzzer...
[CONTROL_BUZZER][INFO] Buzzer test completed
```

---

## FreeRTOS Task Implementation

### Understanding FreeRTOS Tasks

Each module runs in its own FreeRTOS task, providing:
- **Concurrent execution**: Multiple modules run simultaneously
- **Priority-based scheduling**: Critical tasks get more CPU time
- **Core affinity**: Bind tasks to specific CPU cores
- **Stack isolation**: Each task has dedicated memory

### Task Configuration Structure

```cpp
struct TaskConfig {
    String name;           // Task name (for debugging)
    uint32_t stackSize;    // Stack size in bytes
    UBaseType_t priority;  // Task priority (0-31, higher = more important)
    void* params;          // Optional parameters
    int8_t core;           // CPU core (-1=any, 0=protocol, 1=application)
};
```

### Task Priority Guidelines

```
Priority 31-24: CRITICAL SYSTEM TASKS
  - Not recommended for user modules
  - Reserved for WiFi, Bluetooth drivers

Priority 23-16: HIGH PRIORITY
  - Real-time control (motor control, servo)
  - Time-critical operations

Priority 15-8: MEDIUM PRIORITY (RECOMMENDED)
  - Sensor reading
  - Display updates
  - Network communication

Priority 7-1: LOW PRIORITY
  - Background tasks
  - Logging
  - Non-critical operations

Priority 0: IDLE TASK
  - Never use - reserved for FreeRTOS idle task
```

### Stack Size Guidelines

```cpp
// Minimal task (simple flag checking)
taskCfg.stackSize = 1024;  // 1 KB

// Small task (basic I/O, simple processing)
taskCfg.stackSize = 2048;  // 2 KB

// Medium task (JSON parsing, moderate computation)
taskCfg.stackSize = 4096;  // 4 KB

// Large task (graphics, heavy processing)
taskCfg.stackSize = 8192;  // 8 KB

// Very large task (web server, complex operations)
taskCfg.stackSize = 16384; // 16 KB
```

### Core Affinity Best Practices

**Core 0 (Protocol CPU)**:
- WiFi operations
- Bluetooth operations
- Network stack
- Web server

**Core 1 (Application CPU)**:
- Display rendering
- Sensor processing
- Motor control
- User application logic

**Any Core (-1)**:
- Background tasks
- Low-priority operations
- Tasks without specific requirements

### Task Function Template

```cpp
// Static task function (runs the module loop)
static void moduleTaskFunction(void* parameter) {
    ModuleBase* module = static_cast<ModuleBase*>(parameter);
    
    // Task initialization
    module->log("Task started");
    
    // Main task loop
    while (true) {
        // Process queue messages
        QueueBase* queue = module->getQueue();
        if (queue) {
            QueueMessage* msg = nullptr;
            if (queue->receive(msg)) {
                module->handleQueueMessage(msg);
                
                // Cleanup
                if (msg->callVariables) {
                    delete msg->callVariables;
                }
                delete msg;
            }
        }
        
        // Module-specific processing
        module->loop();
        
        // Yield to other tasks (IMPORTANT!)
        vTaskDelay(pdMS_TO_TICKS(10));  // 10ms delay
    }
}
```

### Starting a Task

```cpp
bool CONTROL_BUZZER::start() {
    // Create task if enabled
    if (useTask && !taskBase) {
        taskBase = new TaskBase(this, taskCfg);
        
        if (!taskBase->start(moduleTaskFunction)) {
            log("Failed to start task", "ERROR");
            delete taskBase;
            taskBase = nullptr;
            return false;
        }
    }
    
    // Create queue if enabled
    if (useQueue && !queueBase) {
        queueBase = new QueueBase(this, queueCfg);
        
        if (!queueBase->create()) {
            log("Failed to create queue", "ERROR");
            delete queueBase;
            queueBase = nullptr;
            return false;
        }
    }
    
    setState(MODULE_RUNNING);
    return true;
}
```

### Stopping a Task

```cpp
bool CONTROL_BUZZER::stop() {
    // Stop task
    if (taskBase) {
        taskBase->stop();
        delete taskBase;
        taskBase = nullptr;
    }
    
    // Destroy queue
    if (queueBase) {
        queueBase->destroy();
        delete queueBase;
        queueBase = nullptr;
    }
    
    setState(MODULE_ENABLED);
    return true;
}
```

### Monitoring Task Health

```cpp
// Get task stack high water mark (minimum free stack)
UBaseType_t freeStack = uxTaskGetStackHighWaterMark(taskHandle);
log("Free stack: " + String(freeStack) + " bytes");

// If freeStack < 100 bytes, increase stack size!

// Get task state
eTaskState taskState = eTaskGetState(taskHandle);
// States: eRunning, eReady, eBlocked, eSuspended, eDeleted
```

---

## Queue-Based Communication

### Why Use Queues?

**Thread Safety**: 
- Safe communication between tasks
- No race conditions
- No need for manual mutex management

**Decoupling**:
- Modules don't need to know about each other's internals
- Easy to add/remove modules
- Clean architecture

**Asynchronous**:
- Non-blocking communication
- Tasks can work independently
- Better responsiveness

### Queue Message Design

```cpp
// Sending a simple command
DynamicJsonDocument* vars = new DynamicJsonDocument(128);
(*vars)["command"] = "start";

QueueMessage* msg = new QueueMessage{
    genUUID4(),
    "CONTROL_MOTOR",     // Destination
    "CONTROL_BUTTON",    // Source
    EVENT_DATA_READY,
    CALL_FUNCTION_ASYNC,
    "motor_start",
    vars
};

// Send to motor module
motorModule->getQueue()->send(msg);
```

```cpp
// Sending data
DynamicJsonDocument* vars = new DynamicJsonDocument(256);
(*vars)["temperature"] = 25.5;
(*vars)["humidity"] = 60;
(*vars)["timestamp"] = millis();

QueueMessage* msg = new QueueMessage{
    genUUID4(),
    "CONTROL_LCD",
    "CONTROL_SENSOR",
    EVENT_DATA_READY,
    CALL_FUNCTION_ASYNC,
    "display_sensor_data",
    vars
};

lcdModule->getQueue()->send(msg);
```

### Message Handler Pattern

```cpp
void CONTROL_BUZZER::handleQueueMessage(QueueMessage* msg) {
    if (!msg) return;
    
    // Log for debugging
    if (debugMode) {
        log("Message: " + msg->callName + " from " + msg->fromQueue);
    }
    
    // Async function calls
    if (msg->callType == CALL_FUNCTION_ASYNC) {
        if (msg->callName == "beep") {
            // Extract parameters
            uint32_t duration = 100;  // default
            if (msg->callVariables && msg->callVariables->containsKey("duration")) {
                duration = (*msg->callVariables)["duration"];
            }
            beep(duration);
            
            // Send acknowledgment if needed
            if (msg->eventType == EVENT_DATA_READY) {
                sendAck(msg->fromQueue, msg->eventUUID);
            }
        }
        
    // Synchronous function calls
    } else if (msg->callType == CALL_FUNCTION_SYNC) {
        // Process and send response
        DynamicJsonDocument* response = new DynamicJsonDocument(256);
        (*response)["status"] = "OK";
        (*response)["result"] = processRequest(msg);
        
        sendResponse(msg->fromQueue, msg->eventUUID, response);
        
    // Variable get
    } else if (msg->callType == CALL_VARIABLE_GET) {
        DynamicJsonDocument* response = new DynamicJsonDocument(128);
        (*response)["value"] = getVariable(msg->callName);
        
        sendResponse(msg->fromQueue, msg->eventUUID, response);
        
    // Variable set
    } else if (msg->callType == CALL_VARIABLE_SET) {
        if (msg->callVariables) {
            setVariable(msg->callName, (*msg->callVariables)["value"]);
        }
    }
}
```

### Helper Functions for Messaging

```cpp
// Send acknowledgment
void CONTROL_BUZZER::sendAck(const String& destination, const String& uuid) {
    DynamicJsonDocument* vars = new DynamicJsonDocument(64);
    (*vars)["status"] = "OK";
    
    QueueMessage* msg = new QueueMessage{
        uuid,           // Same UUID as original message
        destination,
        moduleName,
        EVENT_ACK,
        CALL_NONE,
        "ack",
        vars
    };
    
    // Get destination module's queue
    ModuleBase* dest = moduleManager->findModule(destination);
    if (dest && dest->getQueue()) {
        dest->getQueue()->send(msg);
    }
}

// Send response with data
void CONTROL_BUZZER::sendResponse(const String& destination, const String& uuid, DynamicJsonDocument* data) {
    QueueMessage* msg = new QueueMessage{
        uuid,
        destination,
        moduleName,
        EVENT_PROCESS_DONE,
        CALL_NONE,
        "response",
        data
    };
    
    ModuleBase* dest = moduleManager->findModule(destination);
    if (dest && dest->getQueue()) {
        dest->getQueue()->send(msg);
    }
}

// Broadcast to all modules
void CONTROL_BUZZER::broadcast(const String& functionName, DynamicJsonDocument* data) {
    const auto& modules = moduleManager->getModules();
    
    for (auto* module : modules) {
        if (module == this) continue;  // Don't send to self
        
        if (module->getQueue()) {
            // Create copy of data for each module
            DynamicJsonDocument* dataCopy = new DynamicJsonDocument(*data);
            
            QueueMessage* msg = new QueueMessage{
                genUUID4(),
                module->getName(),
                moduleName,
                EVENT_DATA_READY,
                CALL_FUNCTION_ASYNC,
                functionName,
                dataCopy
            };
            
            module->getQueue()->send(msg);
        }
    }
}
```

---

