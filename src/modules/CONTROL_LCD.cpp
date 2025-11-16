#include "CONTROL_LCD.h"
#include "CONTROL_WIFI.h"

CONTROL_LCD::CONTROL_LCD() : Module("CONTROL_LCD") {
    tft = nullptr;
    lcdInitialized = false;
    brightness = 255;
    rotation = 90;
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
    lastRadarDistance = -9999;
    lastRadarSpeed = -9999.0f;
    lastRadarDir = -99;
    lastRadarType = -99;
    lastRadarAngle = -9999;
    firstRadarDraw = true;
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
    {
        Module* wifiMod = ModuleManager::getInstance()->getModule("CONTROL_WIFI");
        String ip = wifiMod ? static_cast<CONTROL_WIFI*>(wifiMod)->getIP() : String("esp32.local");
        drawFooterURL(String("http://") + ip);
    }
    
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
    QueueBase* qb = getQueue();
    if (qb) {
        QueueMessage* incoming = nullptr;
        if (qb->receive(incoming)) {
            if (incoming && incoming->callVariables) {
                if (incoming->callName == "lcd_log_append") {
                    JsonArray arr = (*incoming->callVariables)["v"].as<JsonArray>();
                    for (JsonVariant v : arr) { appendLogLine(v.as<String>()); }
                    int16_t yStart = LCD_HEIGHT - 70;
                    tft->fillRect(0, yStart, LCD_WIDTH, 70, TFT_BLACK);
                    tft->setTextColor(TFT_WHITE);
                    tft->setTextSize(1);
                    int16_t y = yStart + 4;
                    for (const String& s : logLines) { tft->setCursor(4, y); tft->print(s); y += 12; }
                } else if (incoming->callName == "lcd_radar_update") {
                    int d = (*incoming->callVariables)["d"] | -1;
                    float v = (*incoming->callVariables)["v"] | 0.0f;
                    int dir = (*incoming->callVariables)["dir"] | 0;
                    int type = (*incoming->callVariables)["type"] | 0;
                    int ang = (*incoming->callVariables)["ang"] | 0;
                    if (firstRadarDraw || d != lastRadarDistance || ang != lastRadarAngle || dir != lastRadarDir || type != lastRadarType) {
                        drawRadarBox(d, v, dir, type, ang);
                        lastRadarDistance = d;
                        lastRadarSpeed = v;
                        lastRadarDir = dir;
                        lastRadarType = type;
                        lastRadarAngle = ang;
                        firstRadarDraw = false;
                    }
                } else if (incoming->callName == "lcd_status") {
                    String title = (*incoming->callVariables)["title"].as<String>();
                    JsonArray lines = (*incoming->callVariables)["lines"].as<JsonArray>();
                    std::vector<String> ls;
                    for (JsonVariant it : lines) { ls.push_back(it.as<String>()); }
                    displayStatus(title, ls);
                } else if (incoming->callName == "lcd_text") {
                    int16_t x = (*incoming->callVariables)["x"] | 0;
                    int16_t y = (*incoming->callVariables)["y"] | 0;
                    String text = (*incoming->callVariables)["text"].as<String>();
                    uint16_t color = (*incoming->callVariables)["color"] | TFT_WHITE;
                    drawText(x, y, text, color, 1);
                } else if (incoming->callName == "lcd_boot_step") {
                    String op = (*incoming->callVariables)["op"].as<String>();
                    int percent = (*incoming->callVariables)["percent"] | 0;
                    tft->fillRect(0, 0, LCD_WIDTH, 40, TFT_BLACK);
                    drawCenteredText(18, "ESP32 Modular System", TFT_CYAN, 2);
                    tft->fillRect(0, 60, LCD_WIDTH, 180, TFT_BLACK);
                    drawCenteredText(120, op, TFT_WHITE, 2);
                    drawProgressBar(20, LCD_HEIGHT - 90, LCD_WIDTH - 40, 16, percent, TFT_GREEN);
                }
                delete incoming->callVariables;
                delete incoming;
            } else if (incoming) { delete incoming; }
        }
    }
    return true;
}

bool CONTROL_LCD::test() {
    log("Testing LCD...");
    
    if (!lcdInitialized) {
        log("LCD not initialized", "ERROR");
        return false;
    }
    displayWelcome();
    delay(3000);
    displayError("const String &error");
    delay(3000);
    getDisplay()->invertDisplay(true);
    delay(3000);
    displayStatus("Status", {"Line 1", "Line 2", "Line 3"});
    delay(3000);
    clear(TFT_RED);
    delay(500);
    clear(TFT_GREEN);
    delay(500);
    clear(TFT_BLUE);
    delay(500);
    clear(TFT_BLACK);
    
    // Test text
   
    
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
        if (lcd.containsKey("brightness")) { brightness = lcd["brightness"]; setBrightness(brightness); }
        if (lcd.containsKey("rotation")) {
            int r = lcd["rotation"];
            int mapped = r;
            if (r == 0 || r == 1 || r == 2 || r == 3) { mapped = r; }
            else if (r == 90) mapped = 1;
            else if (r == 180) mapped = 2;
            else if (r == 270) mapped = 3;
            else mapped = 0;
            rotation = (uint8_t)mapped;
            if (tft) tft->setRotation(rotation);
        }
    }
    return true;
}

void CONTROL_LCD::setupBacklight() {
    pinMode(LCD_BLK, OUTPUT);
    digitalWrite(LCD_BLK, HIGH);
}

void CONTROL_LCD::setBrightness(uint8_t level) {
    brightness = level;
    
    if (brightness == 0) {
        digitalWrite(LCD_BLK, LOW);
    } else if (brightness == 255) {
        digitalWrite(LCD_BLK, HIGH);
    } else {
        // Use PWM for brightness control
        ledcSetup(0, 5000, 8);
        ledcAttachPin(LCD_BLK, 0);
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

void CONTROL_LCD::appendLogLine(const String& line) {
    logLines.push_back(line);
    while (logLines.size() > 5) logLines.erase(logLines.begin());
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
    
    drawCenteredText(LCD_HEIGHT / 2 - 40, "ESP32", TFT_CYAN, 3);
    drawCenteredText(LCD_HEIGHT / 2 - 10, "Modular System", TFT_WHITE, 3);
    drawCenteredText(LCD_HEIGHT / 2 + 20, "v1.0.0", TFT_GREEN, 1);
    
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

void CONTROL_LCD::drawRadarBox(int d, float v, int dir, int type, int ang) {
    if (!tft) return;
    int16_t top = 50;
    int16_t h = 200;
    int16_t left = 10;
    int16_t w = LCD_WIDTH - 20;
    tft->fillRect(left, top, w, h, TFT_DARKGREY);
    tft->drawRect(left, top, w, h, TFT_WHITE);
    tft->setTextColor(TFT_BLACK, TFT_DARKGREY);
    tft->setTextSize(1);
    tft->setTextDatum(MC_DATUM);
    tft->drawString(String("Distance ") + String(d) + " cm", LCD_WIDTH/2, top + 20);
    tft->drawString(String("Angle ") + String(ang) + " deg", LCD_WIDTH/2, top + 40);
    if (type == 2) {
        String sp = String(v, 2) + " cm/s";
        String dd = dir > 0 ? "away" : (dir < 0 ? "near" : "still");
        tft->drawString(String("Speed ") + sp + " (" + dd + ")", LCD_WIDTH/2, top + 60);
        int16_t cx = LCD_WIDTH / 2;
        int16_t cy = top + 120;
        int16_t r = 18;
        tft->fillCircle(cx, cy, r, TFT_BLACK);
        tft->drawCircle(cx, cy, r, TFT_YELLOW);
        int16_t len = 35;
        float rad = ang * 0.0174533f;
        int16_t ex = cx + (int16_t)(len * cos(rad));
        int16_t ey = cy + (int16_t)(len * sin(rad));
        tft->drawLine(cx, cy, ex, ey, TFT_YELLOW);
    }
    {
        Module* wifiMod = ModuleManager::getInstance()->getModule("CONTROL_WIFI");
        String ip = wifiMod ? static_cast<CONTROL_WIFI*>(wifiMod)->getIP() : String("esp32.local");
        drawFooterURL(String("http://") + ip);
    }
}

void CONTROL_LCD::drawFooterURL(const String& url) {
    if (!tft) return;
    tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft->setTextSize(1);
    tft->setTextDatum(MC_DATUM);
    tft->fillRect(0, LCD_HEIGHT - 16, LCD_WIDTH, 16, TFT_BLACK);
    tft->drawString(url, LCD_WIDTH/2, LCD_HEIGHT - 8);
}
