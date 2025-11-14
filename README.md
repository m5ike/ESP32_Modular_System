# ESP32 Modular System

Modulární systém pro ESP32-WROOM-32 s 1.9" LCD displejem (ST7789)

## 📋 Obsah

- [Hardware Specifikace](#hardware-specifikace)
- [Instalace](#instalace)
- [Architektura Systému](#architektura-systému)
- [Moduly](#moduly)
- [Použití](#použití)
- [API](#api)
- [Konfigurace](#konfigurace)

## 🔧 Hardware Specifikace

### Board ESP32 1.9'' LCD

- **MCU**: ESP32-WROOM-32
- **Flash**: 16MB
- **Display**: 1.9" LCD 170x320 pixels
- **Driver**: ST7789
- **Konektivita**: Wi-Fi 2.4GHz + Bluetooth BLE
- **USB**: Type-C (CH340 driver)

### LCD Pin Connection (SPI)

| Signal | GPIO | Popis |
|--------|------|-------|
| MOSI | GPIO23 | Data Out |
| SCLK | GPIO18 | Clock |
| CS | GPIO15 | Chip Select |
| DC | GPIO2 | Data/Command |
| RST | GPIO4 | Reset |
| BLK | GPIO32 | Backlight |

## 📦 Instalace

### 1. Požadavky

- [PlatformIO](https://platformio.org/)
- USB Type-C kabel
- CH340 driver (pro Windows)

### 2. Klonování projektu

```bash
git clone <repository-url>
cd ESP32_Modular_System
```

### 3. Konfigurace WiFi

Upravte v `src/main.cpp`:

```cpp
wifiModule->setSSID("VaseSSID");
wifiModule->setPassword("VaseHeslo");
```

### 4. Nahrání

```bash
pio run --target upload
pio device monitor
```

## 🏗️ Architektura Systému

### Modulární Design

Systém je postaven na modulární architektuře, kde každý modul je nezávislý a může být:

- **Zapnut/Vypnut** dynamicky
- **Konfigurován** individuálně
- **Testován** samostatně
- **Laděn** nezávisle

### Module Manager

Centrální správce všech modulů s funkcemi:

- Registrace modulů
- Inicializace podle priority
- Automatické spuštění (autostart)
- Správa životního cyklu
- Globální konfigurace

### Priority Systém

Moduly jsou inicializovány podle priority (vyšší číslo = vyšší priorita):

| Modul | Priorita | Popis |
|-------|----------|-------|
| CONTROL_FS | 100 | File system - nejvyšší priorita |
| CONTROL_LCD | 90 | LCD displej |
| CONTROL_WIFI | 85 | WiFi konektivita |
| CONTROL_SERIAL | 80 | Sériová konzole |
| CONTROL_WEB | 70 | Web server |

## 📦 Moduly

### 1. CONTROL_FS - File System

Správa souborového systému a logování.

**Funkce:**
- SPIFFS file system
- Globální konfigurace (JSON)
- Logging systém
- Správa modulových konfigurací

**Konfigurace:**
```json
{
  "fileSystem": {
    "maxSize": 2097152
  },
  "logSystem": {
    "maxSize": 1048576
  }
}
```

**API:**
```cpp
bool writeFile(const String& path, const String& content);
String readFile(const String& path);
bool writeLog(const String& message, const char* level = "INFO");
```

### 2. CONTROL_LCD - LCD Display

Ovládání 1.9" ST7789 LCD displeje.

**Funkce:**
- Inicializace displeje
- Kreslení textu, tvarů
- Status obrazovky
- Kontrola jasu

**Příklady:**
```cpp
lcdModule->clear();
lcdModule->drawText(10, 10, "Hello ESP32", TFT_WHITE);
lcdModule->setBrightness(128);
lcdModule->displayStatus("Status", lines);
```

### 3. CONTROL_WIFI - WiFi Management

Správa WiFi připojení (AP/Client/APSTA).

**Režimy:**
- **AP Mode**: Access Point
- **Client Mode**: Připojení k WiFi
- **APSTA Mode**: Kombinace obou

**Konfigurace:**
```json
{
  "CONTROL_WIFI": {
    "ssid": "ESP32-AP",
    "password": "12345678",
    "mode": 2,
    "client_dhcp": true
  }
}
```

**API:**
```cpp
bool setMode(WiFiMode_t mode);
bool connectToNetwork();
String getIP();
int scanNetworks();
```

### 4. CONTROL_WEB - Web Server

HTTP web server s API a webovým rozhraním.

**Endpointy:**
- `/` - Domovská stránka
- `/logs` - System logy
- `/display` - Zrcadlení displeje
- `/controls` - Ovládání modulů
- `/config` - Konfigurace

**API Endpointy:**
- `GET /api/status` - Status systému
- `GET /api/modules` - Seznam modulů
- `POST /api/module/control` - Ovládání modulu
- `GET /api/logs` - Logy

### 5. CONTROL_SERIAL - Serial Console

Textová konzole pro ovládání přes sériový port.

**Příkazy:**
```
help              - Nápověda
status            - Status systému
modules           - Seznam modulů
start <module>    - Spustit modul
stop <module>     - Zastavit modul
test <module>     - Testovat modul
logs [n]          - Zobrazit logy
restart           - Restart systému
```

### 6. CONTROL_RADAR (Template)

Modul pro radarové komponenty (MBT1, DIYW1).

**Funkce:**
- Měření vzdálenosti
- LED indikace
- Nastavitelná rychlost
- GPIO konfigurace

### Další moduly (Templates)

- **CONTROL_MEASURE** - Měřící komponenty (MBT2, DIBL1)
- **CONTROL_STEPMOTOR** - Krokový motor (STB2)
- **CONTROL_ULTRASOUND** - Ultrazvuk (UVC1)

## 🚀 Použití

### Základní použití

```cpp
#include "ModuleManager.h"
#include "modules/CONTROL_LCD.h"

// Vytvoření modulu
CONTROL_LCD* lcd = new CONTROL_LCD();

// Registrace
ModuleManager::getInstance()->registerModule(lcd);

// Inicializace a start
lcd->init();
lcd->start();

// Použití
lcd->drawText(10, 10, "Hello!", TFT_WHITE);
```

### Práce s konfigurací

```cpp
// Načtení konfigurace
JsonDocument config;
fsModule->loadGlobalConfig(config);

// Aplikace na moduly
for (Module* mod : moduleManager->getModules()) {
    mod->loadConfig(config);
}

// Uložení
fsModule->saveGlobalConfig(config);
```

### Web API příklad

```javascript
// Status systému
fetch('/api/status')
  .then(r => r.json())
  .then(data => console.log(data));

// Ovládání modulu
fetch('/api/module/control', {
  method: 'POST',
  headers: {'Content-Type': 'application/json'},
  body: JSON.stringify({
    module: 'CONTROL_LCD',
    action: 'restart'
  })
});
```

## 📡 API Reference

### Module Base Class

Každý modul implementuje tyto metody:

```cpp
virtual bool init();          // Inicializace
virtual bool start();         // Spuštění
virtual bool stop();          // Zastavení
virtual bool update();        // Update loop
virtual bool test();          // Test funkčnosti
virtual JsonDocument getStatus();  // Status modulu
```

### Module States

```cpp
enum ModuleState {
    MODULE_DISABLED = 0,
    MODULE_ENABLED = 1,
    MODULE_ERROR = 2,
    MODULE_TESTING = 3
};
```

## ⚙️ Konfigurace

### Globální konfigurace

<<<<<<< HEAD
Umístění: `/config/global.json`
=======
Umístění: `/cfg/global.json`
>>>>>>> de1429e (commit)

```json
{
  "version": "1.0.0",
  "fileSystem": {
    "maxSize": 2097152
  },
  "logSystem": {
    "maxSize": 1048576
  },
  "modules": {
    "CONTROL_FS": {
      "state": "enabled",
      "priority": 100,
      "autoStart": true,
      "debug": false
    },
    "CONTROL_LCD": {
      "state": "enabled",
      "priority": 90,
      "autoStart": true,
      "debug": false,
      "brightness": 255,
      "rotation": 0
    },
    "CONTROL_WIFI": {
      "state": "enabled",
      "priority": 85,
      "autoStart": true,
      "ssid": "YourSSID",
      "password": "YourPassword",
      "mode": 2,
      "client_dhcp": true
    }
  }
}
```

### Modulová konfigurace

Každý modul může mít vlastní konfiguraci:
- `/config/CONTROL_LCD.json`
- `/config/CONTROL_WIFI.json`
- atd.

## 🔍 Debugging

### Sériová konzole

```bash
pio device monitor --baud 115200
```

### Logs

```cpp
// Zápis do logu
module->log("Message", "INFO");  // INFO, WARN, ERROR

// Čtení logů
String logs = fsModule->readLogs(100); // last 100 lines

// Clear logs
fsModule->clearLogs();
```

### Web interface

Přístup k logům přes web:
```
http://<ESP32-IP>/logs
```

## 📝 Vytvoření vlastního modulu

### 1. Vytvoření header souboru

```cpp
// modules/MY_MODULE.h
#ifndef MY_MODULE_H
#define MY_MODULE_H

#include "../ModuleManager.h"

class MY_MODULE : public Module {
private:
    bool myInitialized;
    
public:
    MY_MODULE();
    ~MY_MODULE();
    
    bool init() override;
    bool start() override;
    bool stop() override;
    bool update() override;
    bool test() override;
    JsonDocument getStatus() override;
};

#endif
```

### 2. Implementace

```cpp
// modules/MY_MODULE.cpp
#include "MY_MODULE.h"

MY_MODULE::MY_MODULE() : Module("MY_MODULE") {
    myInitialized = false;
    priority = 50;
    autoStart = true;
}

bool MY_MODULE::init() {
    log("Initializing...");
    myInitialized = true;
    setState(MODULE_ENABLED);
    return true;
}

// ... další metody
```

### 3. Registrace

```cpp
// main.cpp
MY_MODULE* myModule = new MY_MODULE();
moduleManager->registerModule(myModule);
```

## 🤝 Přispívání

1. Fork projektu
2. Vytvořte feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit změny (`git commit -m 'Add AmazingFeature'`)
4. Push to branch (`git push origin feature/AmazingFeature`)
5. Otevřete Pull Request

## 📄 Licence

Tento projekt je licencován pod MIT licencí.

## 🙏 Poděkování

- ESP32 komunita
- TFT_eSPI library
- ArduinoJson library
- ESPAsyncWebServer library

## 📞 Kontakt

Pro otázky a podporu otevřete issue na GitHubu.

---

**Verze:** 1.0.0  
**Poslední aktualizace:** 2024
