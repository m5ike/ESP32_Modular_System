#ifndef CONTROL_LCD_H
#define CONTROL_LCD_H

#include "../ModuleManager.h"
#include <TFT_eSPI.h>

// LCD GPIO Pins - according to board specification
#define LCD_MOSI 23
#define LCD_SCLK 18
#define LCD_CS 15
#define LCD_DC 2
#define LCD_RST 4
#define LCD_BL 32

// Display dimensions
#define LCD_WIDTH 170
#define LCD_HEIGHT 320

class CONTROL_LCD : public Module {
private:
    TFT_eSPI* tft;
    bool lcdInitialized;
    uint8_t brightness;
    uint8_t rotation;
    
    void setupBacklight();
    
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
