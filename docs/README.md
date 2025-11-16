# ESP32 Modular System - Complete Documentation

## 📋 Table of Contents

1. [Project Overview](#project-overview)
2. [System Architecture](#system-architecture)
3. [Getting Started](#getting-started)
4. [Module System](#module-system)
5. [API Reference](#api-reference)
6. [Developer Guide](#developer-guide)
7. [Troubleshooting](#troubleshooting)

---

## 🎯 Project Overview

### What is ESP32 Modular System?

ESP32 Modular System is a professional-grade, highly modular embedded framework for ESP32 microcontrollers. It provides a robust foundation for building complex IoT applications with:

- **True Modularity**: Each control module runs as an independent FreeRTOS task
- **Inter-Module Communication**: Thread-safe queue-based message passing
- **Hot Configuration**: Runtime configuration loading/saving via filesystem
- **Multi-Interface Control**: Web GUI, Serial Console, and API endpoints
- **Professional Architecture**: Following embedded systems best practices

### Key Features

✅ **Modular Architecture**
- Plugin-based module system
- Each module is self-contained and configurable
- Priority-based initialization and execution

✅ **FreeRTOS Integration**
- Each module runs in dedicated FreeRTOS task
- Configurable task priorities, stack sizes, and CPU core affinity
- Inter-task communication via message queues
- Thread-safe operations with mutex protection

✅ **Configuration Management**
- JSON-based configuration files
- Per-module configuration support
- Runtime configuration reload
- Factory defaults with auto-population

✅ **Multi-Interface Control**
- Web-based GUI for visual control
- Serial console for debugging and CLI control
- RESTful API for programmatic access
- Real-time status updates

✅ **Hardware Support**
- 1.9" ST7789 LCD Display (170x320)
- WiFi (AP/Client/AP+STA modes)
- Ultrasonic Radar Sensor
- Stepper Motor Control
- Distance Measurement
- Extensible for additional peripherals

### Hardware Specifications

**Microcontroller**: ESP32-WROOM-32
- Dual-core Xtensa 32-bit LX6 @ 240 MHz
- 520 KB SRAM
- 4 MB Flash (typical)

**Display**: ST7789 TFT LCD
- Size: 1.9 inches
- Resolution: 170x320 pixels
- Interface: SPI
- 16-bit color depth (RGB565)

**Connectivity**
- WiFi 802.11 b/g/n
- Bluetooth Classic + BLE (future support)

---

## 🏗️ System Architecture

### Core Components

```
┌─────────────────────────────────────────────────────────────────┐
│                         ESP32 Hardware                          │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐       │
│  │  Core 0  │  │  Core 1  │  │   WiFi   │  │   SPI    │       │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘       │
└─────────────────────────────────────────────────────────────────┘
                              ▲
                              │
┌─────────────────────────────────────────────────────────────────┐
│                      FreeRTOS Kernel                            │
│  ┌────────────────┐  ┌────────────────┐  ┌────────────────┐   │
│  │   Scheduler    │  │  Queue Manager │  │  Task Manager  │   │
│  └────────────────┘  └────────────────┘  └────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
                              ▲
                              │
┌─────────────────────────────────────────────────────────────────┐
│                      Module Manager                             │
│  • Module Registry                                              │
│  • Priority-based Initialization                                │
│  • Lifecycle Management                                         │
│  • Inter-module Communication                                   │
└─────────────────────────────────────────────────────────────────┘
                              ▲
                              │
┌─────────────────────────────────────────────────────────────────┐
│                      Control Modules                            │
│                                                                 │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐      │
│  │CTRL_FS   │  │CTRL_WIFI │  │CTRL_LCD  │  │CTRL_WEB  │      │
│  │Priority:│  │Priority:│  │Priority:│  │Priority:│      │
│  │  100     │  │  90      │  │  85      │  │  75      │      │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘      │
│                                                                 │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐      │
│  │CTRL_     │  │CTRL_     │  │CTRL_STEP │  │CTRL_     │      │
│  │SERIAL    │  │RADAR     │  │MOTOR     │  │MEASURE   │      │
│  │Priority:│  │Priority:│  │Priority:│  │Priority:│      │
│  │  80      │  │  50      │  │  50      │  │  50      │      │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘      │
└─────────────────────────────────────────────────────────────────┘
```

### Module Architecture

Each module follows a standardized architecture:

```
┌─────────────────────────────────────────────────────────────┐
│                     Module Instance                         │
│                                                             │
│  ┌───────────────────────────────────────────────────────┐ │
│  │              Module Configuration                     │ │
│  │  • Name, Version, Priority                           │ │
│  │  • State (Disabled/Enabled/Running/Error)            │ │
│  │  • Autostart, Debug, Test flags                      │ │
│  └───────────────────────────────────────────────────────┘ │
│                          ▼                                  │
│  ┌───────────────────────────────────────────────────────┐ │
│  │              FreeRTOS Task                            │ │
│  │  • Task Handle                                        │ │
│  │  • Stack Size (configurable)                         │ │
│  │  • Priority (0-31)                                   │ │
│  │  • Core Affinity (0, 1, or -1 for any)             │ │
│  └───────────────────────────────────────────────────────┘ │
│                          ▼                                  │
│  ┌───────────────────────────────────────────────────────┐ │
│  │              Message Queue                            │ │
│  │  • Queue Handle                                       │ │
│  │  • Message Buffer (configurable depth)               │ │
│  │  • Thread-safe Send/Receive                          │ │
│  │  • Timeout Configuration                             │ │
│  └───────────────────────────────────────────────────────┘ │
│                          ▼                                  │
│  ┌───────────────────────────────────────────────────────┐ │
│  │              Module Logic                             │ │
│  │  • init()   - Initialize hardware/resources          │ │
│  │  • start()  - Start module operation                 │ │
│  │  • loop()   - Main processing loop                   │ │
│  │  • stop()   - Cleanup and stop                       │ │
│  │  • test()   - Self-test routines                     │ │
│  └───────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### Data Flow Architecture

```
┌─────────────┐
│ User Input  │
│ (Web/Serial)│
└─────┬───────┘
      │
      ▼
┌─────────────────┐      ┌──────────────┐
│ Module Manager  │◄────►│  FileSystem  │
│  (Dispatcher)   │      │   (Config)   │
└────────┬────────┘      └──────────────┘
         │
         ▼
┌─────────────────────────────────────────┐
│      Module Queue System                │
│                                         │
│  ┌────────┐  ┌────────┐  ┌────────┐   │
│  │Queue 1 │  │Queue 2 │  │Queue N │   │
│  └───┬────┘  └───┬────┘  └───┬────┘   │
└──────┼───────────┼───────────┼─────────┘
       │           │           │
       ▼           ▼           ▼
  ┌────────┐  ┌────────┐  ┌────────┐
  │Module 1│  │Module 2│  │Module N│
  │ Task   │  │ Task   │  │ Task   │
  └───┬────┘  └───┬────┘  └───┬────┘
      │           │           │
      └───────────┴───────────┘
               │
               ▼
        ┌──────────────┐
        │   Hardware   │
        │   Drivers    │
        └──────────────┘
```

---

## 🚀 Getting Started

### Prerequisites

1. **Hardware**
   - ESP32 development board (ESP32-WROOM-32 or compatible)
   - 1.9" ST7789 LCD display (170x320)
   - USB cable for programming
   - Power supply (5V, minimum 500mA)

2. **Software**
   - Visual Studio Code
   - PlatformIO extension
   - Git (for version control)

### Installation

1. **Clone the repository**
   ```bash
   git clone <repository-url>
   cd ESP32_Modular_System
   ```

2. **Open in PlatformIO**
   - Open VS Code
   - Open the project folder
   - PlatformIO will automatically install dependencies

3. **Upload Filesystem (First Time)**
   ```bash
   pio run --target uploadfs
   ```

4. **Build and Upload**
   ```bash
   pio run --target upload
   ```

5. **Monitor Serial Output**
   ```bash
   pio device monitor
   ```

### First Boot

On first boot, the system will:

1. Initialize the filesystem (SPIFFS)
2. Create default configuration files
3. Initialize all enabled modules in priority order
4. Start all autostart modules
5. Display system status on LCD
6. Start WiFi in configured mode
7. Launch web server

Default WiFi credentials:
- **SSID**: ESP32-LCD
- **Password**: 12345678
- **IP Address**: 192.168.4.1

### Quick Configuration

1. **Via Web Interface**
   - Connect to ESP32-LCD WiFi network
   - Navigate to http://192.168.4.1
   - Access configuration panel

2. **Via Serial Console**
   ```
   > config wifi ssid YourNetwork
   > config wifi password YourPassword
   > module restart CONTROL_WIFI
   ```

3. **Via Configuration Files**
   - Edit `/data/config/CONTROL_WIFI.json`
   - Upload filesystem: `pio run --target uploadfs`

---

## 🧩 Module System

### Available Modules


#### 1. CONTROL_FS (Priority: 100) 🔴 Critical

**Purpose**: File system management and configuration storage

**Features**:
- SPIFFS initialization and management
- Configuration file loading/saving
- Log file management with rotation
- Directory operations
- Thread-safe file access via mutex

**FreeRTOS Configuration**:
- Task: `FS_TASK`
- Stack: 4096 bytes
- Priority: 5
- Core: 0 (Protocol CPU)

**Queue Messages**:
- `file_read`: Read file content
- `file_write`: Write file content
- `config_load`: Load module configuration
- `config_save`: Save module configuration
- `log_write`: Write to system log

**Configuration File**: `/config/CONTROL_FS.json`

---

#### 2. CONTROL_WIFI (Priority: 90) 🔴 Critical

**Purpose**: WiFi connectivity management

**Features**:
- Multiple operating modes (AP, STA, AP+STA)
- Auto-reconnection
- Network scanning
- DHCP and static IP configuration
- Connection monitoring

**FreeRTOS Configuration**:
- Task: `WIFI_TASK`
- Stack: 4096 bytes
- Priority: 4
- Core: 0 (Protocol CPU - WiFi library runs on Core 0)

**Queue Messages**:
- `wifi_connect`: Connect to network
- `wifi_disconnect`: Disconnect
- `wifi_scan`: Scan for networks
- `wifi_status`: Get connection status
- `wifi_config`: Update configuration

**Configuration File**: `/config/CONTROL_WIFI.json`

---

#### 3. CONTROL_LCD (Priority: 85)

**Purpose**: Display management and visual feedback

**Features**:
- TFT_eSPI driver integration
- Multi-page UI system
- Status display
- Graphics primitives (text, shapes, images)
- Backlight control
- Touch input (future)

**FreeRTOS Configuration**:
- Task: `LCD_TASK`
- Stack: 8192 bytes (larger for graphics buffer)
- Priority: 3
- Core: 1 (Application CPU)

**Queue Messages**:
- `lcd_clear`: Clear screen
- `lcd_text`: Draw text
- `lcd_status`: Display status page
- `lcd_menu`: Display menu
- `lcd_brightness`: Set backlight level
- `lcd_page`: Switch display page

**Configuration File**: `/config/CONTROL_LCD.json`

---

#### 4. CONTROL_SERIAL (Priority: 80)

**Purpose**: Serial console interface and debugging

**Features**:
- Command-line interface (CLI)
- Module control commands
- Configuration management
- Real-time logging
- Interactive debugging

**FreeRTOS Configuration**:
- Task: `SERIAL_TASK`
- Stack: 4096 bytes
- Priority: 3
- Core: 1 (Application CPU)

**Queue Messages**:
- `serial_print`: Print message to console
- `serial_command`: Execute CLI command
- `serial_log`: Log message

**Available Commands**:
```
help                    - Show available commands
status                  - Show system status
modules                 - List all modules
module start <name>     - Start module
module stop <name>      - Stop module
module status <name>    - Get module status
config list             - List configurations
config get <key>        - Get configuration value
config set <key> <val>  - Set configuration value
wifi scan               - Scan WiFi networks
wifi connect <ssid>     - Connect to network
system info             - Show system information
system restart          - Restart system
```

**Configuration File**: `/config/CONTROL_SERIAL.json`

---

#### 5. CONTROL_WEB (Priority: 75)

**Purpose**: Web server and HTTP API

**Features**:
- Async web server (ESPAsyncWebServer)
- RESTful API
- WebSocket support for real-time updates
- Static file serving
- CORS support
- Module control endpoints

**FreeRTOS Configuration**:
- Task: `WEB_TASK`
- Stack: 8192 bytes
- Priority: 2
- Core: 0 (Protocol CPU)

**API Endpoints**:
```
GET  /api/status             - System status
GET  /api/modules            - List modules
GET  /api/module/<name>      - Module status
POST /api/module/<name>/start - Start module
POST /api/module/<name>/stop  - Stop module
GET  /api/config             - Get all config
POST /api/config             - Update config
GET  /api/wifi/scan          - Scan networks
POST /api/wifi/connect       - Connect to network
```

**WebSocket Events**:
- `module_status`: Module state changes
- `log_message`: System log messages
- `sensor_data`: Real-time sensor readings

**Configuration File**: `/config/CONTROL_WEB.json`

---

#### 6. CONTROL_RADAR (Priority: 50)

**Purpose**: Ultrasonic distance measurement with servo sweep

**Features**:
- HC-SR04 ultrasonic sensor driver
- Servo motor control for sweeping
- Distance mapping
- Object detection
- Scan pattern configuration

**FreeRTOS Configuration**:
- Task: `RADAR_TASK`
- Stack: 4096 bytes
- Priority: 2
- Core: 1 (Application CPU)

**Queue Messages**:
- `radar_start`: Start scanning
- `radar_stop`: Stop scanning
- `radar_single`: Single measurement
- `radar_sweep`: Perform sweep scan
- `radar_data`: Distance data ready

**Hardware Connections**:
```
Ultrasonic Sensor (HC-SR04):
- TRIG: GPIO 13
- ECHO: GPIO 12
- VCC: 5V
- GND: GND

Servo Motor (SG90):
- Signal: GPIO 14
- VCC: 5V
- GND: GND
```

**Configuration File**: `/config/CONTROL_RADAR.json`

---

#### 7. CONTROL_MEASURE (Priority: 50)

**Purpose**: General-purpose measurement and sensor data acquisition

**Features**:
- Multiple sensor support
- Data averaging and filtering
- Threshold detection
- Event triggering
- Data logging

**FreeRTOS Configuration**:
- Task: `MEASURE_TASK`
- Stack: 4096 bytes
- Priority: 2
- Core: 1 (Application CPU)

**Queue Messages**:
- `measure_start`: Start measurement
- `measure_stop`: Stop measurement
- `measure_read`: Read current value
- `measure_calibrate`: Calibrate sensor
- `measure_data`: Measurement data ready

**Configuration File**: `/config/CONTROL_MEASURE.json`

---

#### 8. CONTROL_STEPMOTOR (Priority: 50) ⚠️ Disabled by default

**Purpose**: Stepper motor control

**Features**:
- A4988/DRV8825 driver support
- Microstepping configuration
- Position control
- Speed control
- Homing routines

**FreeRTOS Configuration**:
- Task: `STEPMOTOR_TASK`
- Stack: 4096 bytes
- Priority: 2
- Core: 1 (Application CPU)

**Queue Messages**:
- `motor_move`: Move to position
- `motor_stop`: Emergency stop
- `motor_home`: Home the motor
- `motor_speed`: Set speed
- `motor_enable`: Enable/disable motor

**Hardware Connections**:
```
Stepper Driver (A4988):
- STEP: GPIO 25
- DIR: GPIO 26
- ENABLE: GPIO 27
- MS1: GPIO 33 (optional)
- MS2: GPIO 32 (optional)
- MS3: GPIO 35 (optional)
```

**Configuration File**: `/config/CONTROL_STEPMOTOR.json`

---

### Module States

Every module operates in one of these states:

```
MODULE_DISABLED (0)     - Module not loaded
   ↓
MODULE_ENABLED (1)      - Module loaded, not running
   ↓
MODULE_INITIALIZING (3) - Initialization in progress
   ↓
MODULE_RUNNING (4)      - Module actively running
   ↓
MODULE_ERROR (2)        - Error state, requires attention
```

State transitions:
```
DISABLED → init() → ENABLED
ENABLED → start() → RUNNING
RUNNING → stop() → ENABLED
ANY → error → ERROR
ERROR → reset → DISABLED
```

---

## 📡 Inter-Module Communication

### Queue Message Structure

```cpp
struct QueueMessage {
    String eventUUID;           // Unique message identifier
    String toQueue;             // Destination module name
    String fromQueue;           // Source module name
    EventType eventType;        // Event type (see below)
    CallType callType;          // Call type (see below)
    String callName;            // Function/variable name
    DynamicJsonDocument* vars;  // Parameters/data
};
```

### Event Types

```cpp
enum EventType {
    EVENT_NONE = 0,          // No event
    EVENT_DATA_READY,        // Data available for processing
    EVENT_PROCESS_DONE,      // Processing completed
    EVENT_ACK                // Acknowledgment
};
```

### Call Types

```cpp
enum CallType {
    CALL_NONE = 0,           // No call
    CALL_FUNCTION_SYNC,      // Synchronous function call
    CALL_FUNCTION_ASYNC,     // Asynchronous function call
    CALL_VARIABLE_GET,       // Get variable value
    CALL_VARIABLE_SET        // Set variable value
};
```

### Message Flow Example

**Example**: Main loop sends LCD update request

```cpp
// Create message data
DynamicJsonDocument* vars = new DynamicJsonDocument(256);
(*vars)["title"] = "System";
JsonArray lines = (*vars)["lines"].to<JsonArray>();
lines.add("WiFi Connected");
lines.add("IP: 192.168.1.100");

// Create queue message
QueueMessage* msg = new QueueMessage{
    genUUID4(),                      // eventUUID
    "CONTROL_LCD",                   // toQueue
    "main",                          // fromQueue
    EVENT_DATA_READY,                // eventType
    CALL_FUNCTION_ASYNC,             // callType
    "lcd_status",                    // callName
    vars                             // callVariables
};

// Send to LCD module's queue
QueueBase* lcdQueue = lcdModule->getQueue();
lcdQueue->send(msg);
```

**Example**: Module-to-Module Communication

```cpp
// In CONTROL_RADAR, send distance data to LCD
DynamicJsonDocument* data = new DynamicJsonDocument(512);
(*data)["distance"] = measureDistance();
(*data)["angle"] = currentAngle;

QueueMessage* msg = new QueueMessage{
    genUUID4(),
    "CONTROL_LCD",
    "CONTROL_RADAR",
    EVENT_DATA_READY,
    CALL_FUNCTION_ASYNC,
    "lcd_draw_radar",
    data
};

// Get LCD module's queue from module manager
ModuleBase* lcdMod = moduleManager->findModule("CONTROL_LCD");
if (lcdMod) {
    QueueBase* queue = lcdMod->getQueue();
    queue->send(msg);
}
```

### Thread-Safe Queue Operations

**Sending Messages**:
```cpp
bool QueueBase::send(QueueMessage* msg) {
    if (!queue_) return false;
    
    // Send with timeout
    if (xQueueSend(queue_, &msg, cfg_.sendTimeoutTicks) == pdTRUE) {
        return true;
    }
    
    // Failed to send, cleanup
    if (msg->callVariables) {
        delete msg->callVariables;
    }
    delete msg;
    return false;
}
```

**Receiving Messages**:
```cpp
bool QueueBase::receive(QueueMessage*& out) {
    if (!queue_) return false;
    
    QueueMessage* msg = nullptr;
    if (xQueueReceive(queue_, &msg, cfg_.recvTimeoutTicks) == pdTRUE) {
        out = msg;
        return true;
    }
    return false;
}
```

### Processing Messages in Module Task

```cpp
void moduleTaskFunction(void* param) {
    ModuleBase* module = (ModuleBase*)param;
    QueueBase* queue = module->getQueue();
    
    while (true) {
        QueueMessage* msg = nullptr;
        
        if (queue->receive(msg)) {
            // Process message based on call type
            if (msg->callType == CALL_FUNCTION_ASYNC) {
                // Execute function
                module->handleQueueMessage(msg);
            }
            
            // Cleanup
            if (msg->callVariables) {
                delete msg->callVariables;
            }
            delete msg;
        }
        
        // Module-specific processing
        module->loop();
        
        // Yield to other tasks
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

---

