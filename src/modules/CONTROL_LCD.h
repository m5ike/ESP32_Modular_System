/**
 * @file CONTROL_LCD.h
 * @brief LCD control module interface for ST7789 display.
 * @author Michael Kojdl
 * @email michael@kojdl.com
 * @date 2025-11-16
 * @version 1.0.1
 */
#ifndef CONTROL_LCD_H
#define CONTROL_LCD_H

#include "../ModuleManager.h"
#include "../../include/Config.h"
#include <vector>
#include <TFT_eSPI.h>

class CONTROL_LCD : public Module {
private:
    TFT_eSPI* tft;
    bool lcdInitialized;
    uint8_t brightness;
    uint8_t rotation;
    std::vector<String> logLines;
    int lastRadarDistance;
    float lastRadarSpeed;
    int lastRadarDir;
    int lastRadarType;
    int lastRadarAngle;
    bool firstRadarDraw;
    
    void setupBacklight();
    void drawRadarBox(int d, float v, int dir, int type, int ang);
    void drawFooterURL(const String& url);
    
public:
    CONTROL_LCD();
    ~CONTROL_LCD();
    
    // Module interface
    bool init() override;
    bool start() override;
    bool stop() override;
    bool update() override;
    bool test() override;
    DynamicJsonDocument getStatus() override;
    bool loadConfig(DynamicJsonDocument& doc) override;
    
    // LCD control
    TFT_eSPI* getDisplay() { return tft; }
    void setBrightness(uint8_t level);
    uint8_t getBrightness() { return brightness; }
    void setRotation(uint8_t rot);
    void appendLogLine(const String& line);
    
    // Drawing functions
    void clear(uint16_t color = TFT_BLACK);
    void drawText(int16_t x, int16_t y, const String& text, uint16_t color = TFT_WHITE, uint8_t font = 2);
    void drawCenteredText(int16_t y, const String& text, uint16_t color = TFT_WHITE, uint8_t font = 2);
    void drawRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color, bool filled = false);
    void drawCircle(int16_t x, int16_t y, int16_t r, uint16_t color, bool filled = false);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    
    // Status display
    void displayStatus(const String& title, const std::vector<String>& lines);
    void displayError(const String& error);
    void displayWelcome();
    
    // Graphics
    void drawProgressBar(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t percent, uint16_t color = TFT_GREEN);
};

#endif
