#include "CONTROL_LCD.h"

CONTROL_LCD::CONTROL_LCD() : Module("CONTROL_LCD") {
    tft = nullptr;
    lcdInitialized = false;
    brightness = 255;
    rotation = 0;
    priority = 90; // High priority
    autoStart = true;
    version = "1.0.1";
    setUseQueue(true);
    TaskConfig tcfg = getTaskConfig();
    tcfg.name = "CONTROL_LCD_TASK";
    tcfg.stackSize = 4096;
    tcfg.priority = 3;
    tcfg.core = 1;
    setTaskConfig(tcfg);
    QueueConfig qcfg = getQueueConfig();
    qcfg.length = 16;
    setQueueConfig(qcfg);
}

CONTROL_LCD::~CONTROL_LCD() {
    stop();
    if (tft) {
        delete tft;
    }
}

bool CONTROL_LCD::init() {
    log("Initializing LCD...");
    
    // Setup backlight pin
    setupBacklight();
    
    // Create TFT instance
    tft = new TFT_eSPI();
    if (!tft) {
        log("Failed to create TFT instance", "ERROR");
        setState(MODULE_ERROR);
        return false;
    }
    
    // Initialize display
    tft->init();
    tft->setSwapBytes(true);
    tft->setRotation(rotation);
    tft->fillScreen(TFT_WHITE);
    
    // Set default brightness
    setBrightness(brightness);
    
    lcdInitialized = true;
    setState(MODULE_ENABLED);
    
    log("LCD initialized successfully");
    displayWelcome();
    
    return true;
}

bool CONTROL_LCD::start() {
    if (!lcdInitialized) {
        return init();
    }
    
    setState(MODULE_ENABLED);
    setBrightness(brightness);
    log("LCD started");
    return true;
}

bool CONTROL_LCD::stop() {
    if (lcdInitialized) {
        clear();
        setBrightness(0);
    }
    
    setState(MODULE_DISABLED);
    log("LCD stopped");
    return true;
}

bool CONTROL_LCD::update() {
    // Update display if needed
    return true;
}

bool CONTROL_LCD::test() {
    log("Testing LCD...");
    
    if (!lcdInitialized) {
        log("LCD not initialized", "ERROR");
        return false;
    }
    
    // Test colors
    clear(TFT_RED);
    delay(500);
    clear(TFT_GREEN);
    delay(500);
    clear(TFT_BLUE);
    delay(500);
    clear(TFT_BLACK);
    
    // Test text
    drawCenteredText(LCD_HEIGHT / 2, "LCD TEST OK", TFT_WHITE, 4);
    delay(1000);
    
    clear();
    log("LCD test passed");
    return true;
}

DynamicJsonDocument CONTROL_LCD::getStatus() {
    DynamicJsonDocument doc(1024);
    
    doc["module"] = moduleName;
    doc["state"] = state == MODULE_ENABLED ? "enabled" : "disabled";
    doc["version"] = version;
    doc["priority"] = priority;
    doc["autoStart"] = autoStart;
    doc["debug"] = debugEnabled;
    
    doc["width"] = LCD_WIDTH;
    doc["height"] = LCD_HEIGHT;
    doc["brightness"] = brightness;
    doc["rotation"] = rotation;
    doc["initialized"] = lcdInitialized;
    
    return doc;
}

bool CONTROL_LCD::loadConfig(DynamicJsonDocument& doc) {
    Module::loadConfig(doc);
    if (doc.containsKey("CONTROL_LCD")) {
        JsonObject lcd = doc["CONTROL_LCD"];
        if (lcd.containsKey("brightness")) brightness = lcd["brightness"];
        if (lcd.containsKey("rotation")) { rotation = lcd["rotation"]; if (tft) tft->setRotation(rotation); }
    }
    return true;
}

void CONTROL_LCD::setupBacklight() {
    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, HIGH);
}

void CONTROL_LCD::setBrightness(uint8_t level) {
    brightness = level;
    
    if (brightness == 0) {
        digitalWrite(LCD_BL, LOW);
    } else if (brightness == 255) {
        digitalWrite(LCD_BL, HIGH);
    } else {
        // Use PWM for brightness control
        ledcSetup(0, 5000, 8);
        ledcAttachPin(LCD_BL, 0);
        ledcWrite(0, brightness);
    }
    
    if (debugEnabled) {
        log("Brightness set to: " + String(brightness));
    }
}

void CONTROL_LCD::setRotation(uint8_t rot) {
    if (tft && rot <= 3) {
        rotation = rot;
        tft->setRotation(rotation);
        log("Rotation set to: " + String(rotation));
    }
}

void CONTROL_LCD::clear(uint16_t color) {
    if (tft) {
        tft->fillScreen(color);
    }
}

void CONTROL_LCD::drawText(int16_t x, int16_t y, const String& text, uint16_t color, uint8_t font) {
    if (!tft) return;
    
    tft->setTextColor(color);
    tft->setTextSize(1);
    tft->setCursor(x, y);
    tft->print(text);
}

void CONTROL_LCD::drawCenteredText(int16_t y, const String& text, uint16_t color, uint8_t font) {
    if (!tft) return;
    
    tft->setTextColor(color);
    tft->setTextSize(2);
    tft->setTextDatum(MC_DATUM); // Middle center
    tft->drawString(text, LCD_WIDTH / 2, y);
    tft->setTextDatum(TL_DATUM); // Reset to top left
}

void CONTROL_LCD::drawRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color, bool filled) {
    if (!tft) return;
    
    if (filled) {
        tft->fillRect(x, y, w, h, color);
    } else {
        tft->drawRect(x, y, w, h, color);
    }
}

void CONTROL_LCD::drawCircle(int16_t x, int16_t y, int16_t r, uint16_t color, bool filled) {
    if (!tft) return;
    
    if (filled) {
        tft->fillCircle(x, y, r, color);
    } else {
        tft->drawCircle(x, y, r, color);
    }
}

void CONTROL_LCD::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    if (!tft) return;
    tft->drawLine(x0, y0, x1, y1, color);
}

void CONTROL_LCD::displayStatus(const String& title, const std::vector<String>& lines) {
    if (!tft) return;
    
    clear();
    
    // Draw title
    drawCenteredText(20, title, TFT_CYAN, 4);
    
    // Draw separator
    drawLine(10, 40, LCD_WIDTH - 10, 40, TFT_WHITE);
    
    // Draw status lines
    int16_t y = 60;
    for (const String& line : lines) {
        drawText(10, y, line, TFT_WHITE, 2);
        y += 20;
    }
}

void CONTROL_LCD::displayError(const String& error) {
    if (!tft) return;
    
    clear(TFT_RED);
    drawCenteredText(LCD_HEIGHT / 2 - 20, "ERROR", TFT_WHITE, 4);
    drawCenteredText(LCD_HEIGHT / 2 + 10, error, TFT_WHITE, 2);
}

void CONTROL_LCD::displayWelcome() {
    if (!tft) return;
    
    clear();
    
    drawCenteredText(LCD_HEIGHT / 2 - 40, "ESP32", TFT_CYAN, 4);
    drawCenteredText(LCD_HEIGHT / 2 - 10, "Modular System", TFT_WHITE, 4);
    drawCenteredText(LCD_HEIGHT / 2 + 20, "v1.0.0", TFT_GREEN, 2);
    
    drawProgressBar(35, LCD_HEIGHT - 40, LCD_WIDTH - 70, 20, 100, TFT_GREEN);
    
    delay(2000);
    clear();
}

void CONTROL_LCD::drawProgressBar(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t percent, uint16_t color) {
    if (!tft) return;
    
    // Draw border
    tft->drawRect(x, y, w, h, TFT_WHITE);
    
    // Draw filled part
    int16_t fillWidth = (w - 4) * percent / 100;
    if (fillWidth > 0) {
        tft->fillRect(x + 2, y + 2, fillWidth, h - 4, color);
    }
    
    // Draw percentage text
    String percentText = String(percent) + "%";
    tft->setTextColor(TFT_WHITE);
    tft->setTextFont(2);
    tft->setTextDatum(MC_DATUM);
    tft->drawString(percentText, x + w / 2, y + h / 2);
    tft->setTextDatum(TL_DATUM);
}
