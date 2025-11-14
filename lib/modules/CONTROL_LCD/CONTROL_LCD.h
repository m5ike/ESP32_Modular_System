#ifndef CONTROL_LCD_H
#define CONTROL_LCD_H

#include "ModuleBase.h"
#include <TFT_eSPI.h>
#include <SPI.h>

class CONTROL_LCD : public ModuleBase {
private:
    TFT_eSPI* tft;
    uint8_t brightness;
    bool backlightOn;
    
    void setupBacklight() {
        pinMode(LCD_BLK, OUTPUT);
        setBacklight(brightness);
    }
    
public:
    CONTROL_LCD() : ModuleBase("CONTROL_LCD"), brightness(255), backlightOn(true) {
        priority = 85;
        autostart = true;
        tft = nullptr;
    }
    
    ~CONTROL_LCD() {
        if (tft) {
            delete tft;
        }
    }
    
    bool init() override {
        log("INFO", "Initializing LCD module...");
        
        // Create TFT instance
        tft = new TFT_eSPI();
        
        // Initialize TFT
        tft->init();
        tft->setRotation(0); // Portrait mode
        tft->fillScreen(TFT_BLACK);
        
        // Setup backlight
        setupBacklight();
        
        // Display welcome screen
        displayWelcome();
        
        log("INFO", "LCD initialized successfully");
        return true;
    }
    
    bool start() override {
        log("INFO", "Starting LCD module...");
        return true;
    }
    
    bool stop() override {
        log("INFO", "Stopping LCD module...");
        setBacklight(0);
        if (tft) {
            tft->fillScreen(TFT_BLACK);
        }
        return true;
    }
    
    bool test() override {
        log("INFO", "Testing LCD module...");
        
        if (!tft) {
            log("ERROR", "TFT not initialized");
            return false;
        }
        
        // Test colors
        tft->fillScreen(TFT_RED);
        delay(200);
        tft->fillScreen(TFT_GREEN);
        delay(200);
        tft->fillScreen(TFT_BLUE);
        delay(200);
        tft->fillScreen(TFT_BLACK);
        
        log("INFO", "LCD test passed");
        return true;
    }
    
    void loop() override {
        // Nothing to do in loop for now
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
        
        brightness = (*config)["brightness"] | 255;
        backlightOn = (*config)["backlight_on"] | true;
        
        log("INFO", "Config loaded successfully");
        return true;
    }
    
    bool saveConfig() override {
        (*config)["brightness"] = brightness;
        (*config)["backlight_on"] = backlightOn;
        (*config)["width"] = LCD_WIDTH;
        (*config)["height"] = LCD_HEIGHT;
        (*config)["rotation"] = 0;
        
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
    
    // Public LCD functions
    TFT_eSPI* getTFT() { return tft; }
    
    void setBacklight(uint8_t value) {
        brightness = value;
        analogWrite(LCD_BLK, brightness);
        backlightOn = (brightness > 0);
        
        if (debugMode) {
            logf("INFO", "Backlight set to: %d", brightness);
        }
    }
    
    void toggleBacklight() {
        if (backlightOn) {
            setBacklight(0);
        } else {
            setBacklight(brightness > 0 ? brightness : 255);
        }
    }
    
    void clear() {
        if (tft) {
            tft->fillScreen(TFT_BLACK);
        }
    }
    
    void displayWelcome() {
        if (!tft) return;
        
        tft->fillScreen(TFT_BLACK);
        tft->setTextColor(TFT_WHITE);
        tft->setTextSize(2);
        
        tft->setCursor(10, 50);
        tft->println("ESP32");
        
        tft->setCursor(10, 80);
        tft->println("Modular System");
        
        tft->setTextSize(1);
        tft->setCursor(10, 120);
        tft->println("v1.0.0");
        
        tft->setCursor(10, 140);
        tft->println("Initializing...");
    }
    
    void displayStatus(const String& module, const String& status) {
        if (!tft) return;
        
        static int yPos = 170;
        
        tft->setTextSize(1);
        tft->setTextColor(TFT_GREEN, TFT_BLACK);
        tft->setCursor(10, yPos);
        tft->printf("%s: %s", module.c_str(), status.c_str());
        
        yPos += 15;
        if (yPos > 300) {
            yPos = 170;
        }
    }
    
    void displayText(int x, int y, const String& text, uint16_t color = TFT_WHITE, uint8_t size = 1) {
        if (!tft) return;
        
        tft->setTextSize(size);
        tft->setTextColor(color, TFT_BLACK);
        tft->setCursor(x, y);
        tft->print(text);
    }
    
    void drawRect(int x, int y, int w, int h, uint16_t color) {
        if (!tft) return;
        tft->drawRect(x, y, w, h, color);
    }
    
    void fillRect(int x, int y, int w, int h, uint16_t color) {
        if (!tft) return;
        tft->fillRect(x, y, w, h, color);
    }
    
    void drawLine(int x0, int y0, int x1, int y1, uint16_t color) {
        if (!tft) return;
        tft->drawLine(x0, y0, x1, y1, color);
    }
    
    String getStatusJson() override {
        DynamicJsonDocument doc(512);
        doc["name"] = moduleName;
        doc["state"] = (int)state;
        doc["brightness"] = brightness;
        doc["backlight_on"] = backlightOn;
        doc["width"] = LCD_WIDTH;
        doc["height"] = LCD_HEIGHT;
        
        String output;
        serializeJson(doc, output);
        return output;
    }
};

#endif // CONTROL_LCD_H
