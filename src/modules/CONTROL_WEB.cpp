/**
 * @file CONTROL_WEB.cpp
 * @brief Web server module: routes, UI, and API endpoints.
 * @author Michael Kojdl
 * @email michael@kojdl.com
 * @date 2025-11-16
 * @version 1.0.0
 */
#include "CONTROL_WEB.h"
#include "CONTROL_FS.h"
#include "CONTROL_LCD.h"
#include "CONTROL_WIFI.h"
#include "CONTROL_RADAR.h"
#include "ConfigManager.h"

CONTROL_WEB::CONTROL_WEB() : Module("CONTROL_WEB") {
    server = nullptr;
    serverRunning = false;
    port = 80;
    priority = 70;
    autoStart = true;
    version = "1.0.0";
    setUseQueue(true);
    TaskConfig tcfg = getTaskConfig();
    tcfg.name = "CONTROL_WEB_TASK";
    tcfg.stackSize = 8192;
    tcfg.priority = 3;
    tcfg.core = 1;
    setTaskConfig(tcfg);
    QueueConfig qcfg = getQueueConfig();
    qcfg.length = 16;
    setQueueConfig(qcfg);
}

CONTROL_WEB::~CONTROL_WEB() {
    stop();
    if (server) {
        delete server;
    }
}

bool CONTROL_WEB::init() {
    log("Initializing web server...");
    
    // Check if WiFi is available
    Module* wifiModule = ModuleManager::getInstance()->getModule("CONTROL_WIFI");
    if (!wifiModule || wifiModule->getState() != MODULE_ENABLED) {
        log("WiFi module not available", "WARN");
        // Can still init, but won't be useful without WiFi
    }
    
    // Create server instance
    server = new AsyncWebServer(port);
    if (!server) {
        log("Failed to create server instance", "ERROR");
        setState(MODULE_ERROR);
        return false;
    }
    
    // Setup routes
    setupRoutes();
    setupAPIRoutes();
    
    setState(MODULE_ENABLED);
    log("Web server initialized on port " + String(port));
    return true;
}

bool CONTROL_WEB::start() {
    if (!server) {
        if (!init()) return false;
    }
    
    log("Starting web server...");
    
    server->begin();
    serverRunning = true;
    
    setState(MODULE_ENABLED);
    log("Web server started");
    
    // Get IP from WiFi module
    Module* wifiModule = ModuleManager::getInstance()->getModule("CONTROL_WIFI");
    if (wifiModule) {
        CONTROL_WIFI* wifi = static_cast<CONTROL_WIFI*>(wifiModule);
        log("Server available at: http://" + wifi->getIP());
    }
    
    return true;
}

bool CONTROL_WEB::stop() {
    if (server && serverRunning) {
        log("Stopping web server...");
        server->end();
        serverRunning = false;
    }
    
    setState(MODULE_DISABLED);
    log("Web server stopped");
    return true;
}

bool CONTROL_WEB::update() {
    // Web server handles requests automatically
    return true;
}

bool CONTROL_WEB::test() {
    log("Testing web server...");
    
    if (!server) {
        log("Server not initialized", "ERROR");
        return false;
    }
    
    if (!serverRunning) {
        log("Server not running", "ERROR");
        return false;
    }
    
    log("Web server test passed");
    return true;
}

DynamicJsonDocument CONTROL_WEB::getStatus() {
    DynamicJsonDocument doc(2048);
    
    doc["module"] = moduleName;
    doc["state"] = state == MODULE_ENABLED ? "enabled" : "disabled";
    doc["version"] = version;
    doc["priority"] = priority;
    doc["autoStart"] = autoStart;
    doc["debug"] = debugEnabled;
    
    doc["running"] = serverRunning;
    doc["port"] = port;
    
    // Add ConfigManager statistics if available
    Module* fsModule = ModuleManager::getInstance()->getModule("CONTROL_FS");
    if (fsModule) {
        CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsModule);
        ConfigManager* cfg = fs->getConfigManager();
        if (cfg) {
            JsonObject configStats = doc["config_manager"].to<JsonObject>();
            ConfigManager::ConfigStats stats = cfg->getStatistics();
            configStats["version"] = cfg->getCurrentVersion();
            configStats["backup_count"] = stats.backupCount;
            configStats["config_size"] = stats.configSize;
            configStats["total_backup_size"] = stats.totalBackupSize;
            configStats["last_backup_time"] = stats.lastBackupTime;
        }
    }
    
    return doc;
}

void CONTROL_WEB::setupRoutes() {
    if (!server) return;
    
    // Root page
    server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleRoot(request);
    });
    
    // Logs page
    server->on("/logs", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleLogs(request);
    });
    
    // Display page
    server->on("/display", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleDisplay(request);
    });
    
    // Controls page
    server->on("/controls", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleControls(request);
    });
    
    // Configuration page
    server->on("/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleConfig(request);
    });
    // Schema page
    server->on("/schema", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String content;
        content.reserve(4096);
        content = "<h1>Configuration Schema</h1>";
        content += "<a href='/' >Back to Home</a><hr>";
        Module* fsModule = ModuleManager::getInstance()->getModule("CONTROL_FS");
        if (fsModule) {
            CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsModule);
            String schema = fs->readFile("/schema.json");
            if (schema.length() == 0) schema = "(no schema)";
            content += "<pre>" + schema + "</pre>";
        } else {
            content += "<p>FS module not available</p>";
        }
        request->send(200, "text/html", buildHTML("Schema", content));
    });
    
    // 404 handler
    server->onNotFound([this](AsyncWebServerRequest *request) {
        this->handleNotFound(request);
    });
}

void CONTROL_WEB::setupAPIRoutes() {
    if (!server) return;
    
    // API Status
    server->on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleAPIStatus(request);
    });
    
    // API Modules list
    server->on("/api/modules", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleAPIModules(request);
    });
    
    // API Module control (start/stop/test) with safety validation
    server->on("/api/module/control", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleAPIModuleControl(request);
    });
    
    // API Module configuration
    server->on("/api/module/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleAPIModuleConfig(request);
    });
    
    // API Module set with validation
    server->on("/api/module/set", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleAPIModuleSet(request);
    });
    
    // API Module autostart
    server->on("/api/module/autostart", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleAPIModuleAutostart(request);
    });
    
    // API Module commands with safety checks
    server->on("/api/module/command", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleAPIModuleCommand(request);
    });
    
    // API Configuration management
    server->on("/api/config/backup", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleAPIConfigBackup(request);
    });
    
    server->on("/api/config/validate", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleAPIConfigValidate(request);
    });
    
    server->on("/api/config/export", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleAPIConfigExport(request);
    });
    
    server->on("/api/config/import", HTTP_POST, [this](AsyncWebServerRequest *request) {
        this->handleAPIConfigImport(request);
    });
    // API filesystem audit
    server->on("/api/fs/check", HTTP_POST, [this](AsyncWebServerRequest *request) {
        Module* fsModule = ModuleManager::getInstance()->getModule("CONTROL_FS");
        if (!fsModule) { request->send(503, "application/json", "{\"error\":\"FS module not available\"}"); return; }
        CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsModule);
        bool fix = request->hasParam("fix") ? (request->getParam("fix")->value() == "1") : true;
        bool ok = fs->auditFileSystem(fix);
        request->send(200, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
    });
    server->on("/fscheck", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String content;
        content.reserve(1024);
        content = "<h1>Filesystem Audit</h1><p>Runs a comprehensive audit of the board filesystem.</p>";
        content += "<form method='post' action='/api/fs/check'><input type='hidden' name='fix' value='1'><button type='submit'>Run Audit (Fix)</button></form>";
        request->send(200, "text/html", buildHTML("FS Audit", content));
    });
    // API Schema get/save
    server->on("/api/config/schema", HTTP_GET, [this](AsyncWebServerRequest *request) {
        Module* fsModule = ModuleManager::getInstance()->getModule("CONTROL_FS");
        if (!fsModule) { request->send(503, "application/json", "{\"error\":\"FS module not available\"}"); return; }
        CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsModule);
        String schema = fs->readFile("/schema.json");
        if (schema.length() == 0) schema = "{}";
        request->send(200, "application/json", schema);
    });
    server->on("/api/config/schema", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (!request->hasParam("plain", true)) { request->send(400, "application/json", "{\"error\":\"No schema data provided\"}"); return; }
        String schemaData = request->getParam("plain", true)->value();
        Module* fsModule = ModuleManager::getInstance()->getModule("CONTROL_FS");
        if (!fsModule) { request->send(503, "application/json", "{\"error\":\"FS module not available\"}"); return; }
        CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsModule);
        if (!fs->writeFile("/schema.json", schemaData)) { request->send(500, "application/json", "{\"error\":\"Failed to write schema\"}"); return; }
        ConfigManager* cfg = fs->getConfigManager();
        if (cfg) cfg->loadSchemaFromFile("/schema.json");
        request->send(200, "application/json", "{\"success\":true}");
    });
    
    // API System information
    server->on("/api/system/info", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleAPISystemInfo(request);
    });
    
    server->on("/api/system/stats", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleAPISystemStats(request);
    });
    
    // API Safety and limits
    server->on("/api/safety/limits", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleAPISafetyLimits(request);
    });
    
    server->on("/api/safety/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleAPISafetyStatus(request);
    });
    
    // API Logs
    server->on("/api/logs", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleAPILogs(request);
    });
    
    // API Radar status
    server->on("/api/radar", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleAPIRadar(request);
    });
    
    // API Test
    server->on("/api/test", HTTP_POST, [this](AsyncWebServerRequest *request) {
        this->handleAPITest(request);
    });
}

void CONTROL_WEB::handleRoot(AsyncWebServerRequest *request) {
    String content;
    content.reserve(2048);
    content = "<h1>ESP32 Modular System</h1>";
    content += "<div class='menu'>";
    content += "<a href='/'>Home</a> | ";
    content += "<a href='/logs'>Logs</a> | ";
    content += "<a href='/display'>Display</a> | ";
    content += "<a href='/controls'>Controls</a> | ";
    content += "<a href='/config'>Configuration</a>";
    content += "</div>";
    content += "<hr>";
    
    // System info
    content += "<h2>System Information</h2>";
    content += "<table>";
    content += "<tr><td>Uptime:</td><td>" + String(millis() / 1000) + " seconds</td></tr>";
    content += "<tr><td>Free Heap:</td><td>" + String(ESP.getFreeHeap()) + " bytes</td></tr>";
    content += "<tr><td>Chip Model:</td><td>" + String(ESP.getChipModel()) + "</td></tr>";
    content += "</table>";
    
    // Modules status
    content += getModulesHTML();
    
    request->send(200, "text/html", buildHTML("Home", content));
}

void CONTROL_WEB::handleLogs(AsyncWebServerRequest *request) {
    String content;
    content.reserve(1024);
    content = "<h1>System Logs</h1>";
    content += "<a href='/'>Back to Home</a><hr>";
    content += getLogsHTML();
    
    request->send(200, "text/html", buildHTML("Logs", content));
}

void CONTROL_WEB::handleDisplay(AsyncWebServerRequest *request) {
    String content;
    content.reserve(2048);
    content = "<h1>Display</h1>";
    content += "<a href='/'>Back to Home</a><hr>";
    content += "<canvas id='display' width='170' height='320' style='border:1px solid #444'></canvas>";
    content += "<script>\n";
    content += "const c=document.getElementById('display');const ctx=c.getContext('2d');\n";
    content += "function draw(data){ctx.fillStyle='#000';ctx.fillRect(0,0,c.width,c.height);\n";
    content += "ctx.fillStyle='#0ff';ctx.font='14px monospace';ctx.textAlign='center';ctx.fillText('ESP32 Modular System',c.width/2,20);\n";
    content += "ctx.fillStyle='#fff';ctx.font='16px monospace';ctx.fillText('Distance '+data.d+' cm',c.width/2,44);\n";
    content += "ctx.fillStyle='#ff0';ctx.font='12px monospace';ctx.fillText('Angle '+data.ang+' deg',c.width/2,64);\n";
    content += "if(data.type==2){const sp=(Math.round(data.v*100)/100).toFixed(2)+' cm/s';const dd=data.dir>0?'away':(data.dir<0?'near':'still');ctx.fillStyle='#0ff';ctx.font='14px monospace';ctx.fillText('Speed '+sp+' ('+dd+')',c.width/2,84);\n";
    content += "ctx.strokeStyle='#ff0';const cx=c.width/2,cy=140,len=40;ctx.beginPath();ctx.arc(cx,cy,20,0,Math.PI*2);ctx.stroke();\n";
    content += "const rad=data.ang*0.0174533;const ex=cx+Math.round(len*Math.cos(rad));const ey=cy+Math.round(len*Math.sin(rad));ctx.beginPath();ctx.moveTo(cx,cy);ctx.lineTo(ex,ey);ctx.stroke();}\n";
    content += "else{const barX=20,barY=280,barW=c.width-40,barH=18;ctx.strokeStyle='#fff';ctx.strokeRect(barX,barY,barW,barH);ctx.fillStyle='#0f0';const pct=Math.max(0,Math.min(100,Math.round((data.d*100)/400)));ctx.fillRect(barX+2,barY+2,Math.round((barW-4)*pct/100),barH-4);ctx.fillStyle='#fff';ctx.font='12px monospace';ctx.fillText(pct+'%',barX+barW/2,barY+barH/2+4);}\n";
    content += "}\n";
    content += "async function tick(){try{const r=await fetch('/api/radar');if(r.ok){const d=await r.json();draw(d);} }catch(e){} setTimeout(tick,200);} tick();\n";
    content += "</script>";
    
    request->send(200, "text/html", buildHTML("Display", content));
}

void CONTROL_WEB::handleAPIRadar(AsyncWebServerRequest *request) {
    Module* radarModule = ModuleManager::getInstance()->getModule("CONTROL_RADAR");
    int d = -1; float v = 0.0f; int dir = 0; int ang = 0; int type = 0;
    if (radarModule) {
        CONTROL_RADAR* radar = static_cast<CONTROL_RADAR*>(radarModule);
        DynamicJsonDocument status = radar->getStatus();
        d = status["distance_cm"] | d;
        v = status["speed_cms"] | v;
        dir = status["direction"] | dir;
        ang = status["angle_deg"] | ang;
        type = status["type"] | type;
    }
    DynamicJsonDocument doc(256);
    doc["d"] = d;
    doc["v"] = v;
    doc["dir"] = dir;
    doc["ang"] = ang;
    doc["type"] = type;
    String res; serializeJson(doc, res);
    request->send(200, "application/json", res);
}

void CONTROL_WEB::handleControls(AsyncWebServerRequest *request) {
    String content;
    content.reserve(4096);
    content = "<h1>Controls</h1>";
    content += "<a href='/'>Back to Home</a><hr>";
    
    // Module controls
    content += "<h2>Module Controls</h2>";
    auto modules = ModuleManager::getInstance()->getModules();
    
    for (Module* mod : modules) {
        content += "<div class='module-control'>";
        content += "<h3>" + mod->getName() + "</h3>";
        content += "<p>State: " + String(mod->getState() == MODULE_ENABLED ? "Enabled" : "Disabled") + "</p>";
        content += "<button onclick=\"controlModule('" + mod->getName() + "', 'start')\">Start</button> ";
        content += "<button onclick=\"controlModule('" + mod->getName() + "', 'stop')\">Stop</button> ";
        content += "<button onclick=\"controlModule('" + mod->getName() + "', 'test')\">Test</button> ";
        content += "<button onclick=\"toggleEnable('" + mod->getName() + "', 'on')\">Enable</button> ";
        content += "<button onclick=\"toggleEnable('" + mod->getName() + "', 'off')\">Disable</button> ";
        content += "<span>Autostart: ";
        content += "<button onclick=\"setAutostart('" + mod->getName() + "', 'on')\">On</button>";
        content += "<button onclick=\"setAutostart('" + mod->getName() + "', 'off')\">Off</button></span>";
        if (mod->getName() == "CONTROL_LCD") {
            content += "<div><label>Brightness <input id='lcd_brightness' type='range' min='0' max='255' value='128' oninput=saveLCDBrightness(this.value)></label>";
            content += " <label>Rotation <select id='lcd_rotation' onchange=saveLCDRotation(this.value)><option>0</option><option>1</option><option>2</option><option>3</option></select></label></div>";
        }
        if (mod->getName() == "CONTROL_RADAR") {
            content += "<div><label>Rotation <select onchange=saveRadarRotation(this.value)><option value='0'>stop</option><option value='1'>slow</option><option value='2'>fast</option><option value='3'>auto</option></select></label>";
            content += " <label>Measure <select onchange=saveRadarMeasure(this.value)><option value='0'>distance</option><option value='1'>movement</option></select></label></div>";
        }
        content += "<div><button onclick=\"showLogs('" + mod->getName() + "')\">Show Logs</button><pre id='logs_" + mod->getName() + "'></pre></div>";
        content += "</div>";
    }
    
    content += "<script>";
    content += "function controlModule(name, action){fetch('/api/module/control?module='+name+'&action='+action).then(r=>r.json()).then(d=>{alert(JSON.stringify(d));location.reload();});}";
    content += "function setAutostart(name,val){fetch('/api/module/autostart?module='+name+'&value='+val).then(r=>r.text()).then(t=>{alert(t);location.reload();});}";
    content += "function toggleEnable(name,val){if(val==='on'){fetch('/api/module/control?module='+name+'&action=start').then(()=>location.reload());}else{fetch('/api/module/control?module='+name+'&action=stop').then(()=>location.reload());}}";
    content += "function showLogs(name){fetch('/api/logs?module='+name).then(r=>r.json()).then(d=>{document.getElementById('logs_'+name).textContent=d.logs;});}";
    content += "function saveLCDBrightness(v){fetch('/api/module/set?module=CONTROL_LCD&key=brightness&value='+v).then(r=>r.text()).then(()=>{});}";
    content += "function saveLCDRotation(v){fetch('/api/module/set?module=CONTROL_LCD&key=rotation&value='+v).then(r=>r.text()).then(()=>{});}";
    content += "function saveRadarRotation(v){fetch('/api/module/set?module=CONTROL_RADAR&key=rotation_mode&value='+v).then(r=>r.text()).then(()=>{});}";
    content += "function saveRadarMeasure(v){fetch('/api/module/set?module=CONTROL_RADAR&key=measure_mode&value='+v).then(r=>r.text()).then(()=>{});}";
    content += "</script>";
    
    request->send(200, "text/html", buildHTML("Controls", content));
}

void CONTROL_WEB::handleConfig(AsyncWebServerRequest *request) {
    String content;
    content.reserve(4096);
    content = "<h1>Configuration</h1>";
    content += "<a href='/'>Back to Home</a><hr>";
    content += getConfigHTML();
    content += "<hr>";
    content += "<h2>Edit Module</h2>";
    content += "<label>Module: <input id='mod' value='CONTROL_LCD'></label><br>";
    content += "<label>Key: <input id='key' value='brightness'></label><br>";
    content += "<label>Value: <input id='val' value='255'></label><br>";
    content += "<button onclick=saveKey()>Save Key</button>";
    content += "<h3>JSON</h3>";
    content += "<textarea id='json' rows='10' cols='60'>{\"brightness\":255,\"rotation\":0}</textarea><br>";
    content += "<button onclick=saveJson()>Save JSON</button>";
    content += "<h2>Autostart</h2>";
    content += "<label>Module: <input id='amod' value='CONTROL_LCD'></label>";
    content += "<button onclick=autostart('on')>On</button>";
    content += "<button onclick=autostart('off')>Off</button>";
    content += "<script>function saveKey(){var m=document.getElementById('mod').value;var k=document.getElementById('key').value;var v=document.getElementById('val').value;fetch('/api/module/set?module='+m+'&key='+k+'&value='+encodeURIComponent(v)).then(r=>r.text()).then(t=>alert(t));}function saveJson(){var m=document.getElementById('mod').value;var j=document.getElementById('json').value;fetch('/api/module/set?module='+m+'&json='+encodeURIComponent(j)).then(r=>r.text()).then(t=>alert(t));}function autostart(s){var m=document.getElementById('amod').value;fetch('/api/module/autostart?module='+m+'&value='+s).then(r=>r.text()).then(t=>alert(t));}</script>";
    
    request->send(200, "text/html", buildHTML("Configuration", content));
}

void CONTROL_WEB::handleNotFound(AsyncWebServerRequest *request) {
    String message = "404 - Not Found\n\n";
    message += "URI: " + request->url() + "\n";
    message += "Method: " + String(request->method()) + "\n";
    
    request->send(404, "text/plain", message);
}

void CONTROL_WEB::handleAPIStatus(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(2048);
    
    doc["uptime"] = millis() / 1000;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["chipModel"] = ESP.getChipModel();
    
    // Add modules status
    JsonArray modules = doc["modules"].to<JsonArray>();
    for (Module* mod : ModuleManager::getInstance()->getModules()) {
        JsonObject modObj = modules.createNestedObject();
        modObj["name"] = mod->getName();
        modObj["state"] = mod->getState();
        modObj["priority"] = mod->getPriority();
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void CONTROL_WEB::handleAPIModules(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(4096);
    JsonArray modules = doc["modules"].to<JsonArray>();
    
    for (Module* mod : ModuleManager::getInstance()->getModules()) {
        DynamicJsonDocument statusDoc = mod->getStatus();
        modules.add(statusDoc);
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void CONTROL_WEB::handleAPIModuleControl(AsyncWebServerRequest *request) {
    if (!request->hasParam("module") || !request->hasParam("action")) {
        request->send(400, "application/json", "{\"error\":\"Missing params\"}");
        return;
    }
    String moduleName = request->getParam("module")->value();
    String action = request->getParam("action")->value();
    Module* mod = ModuleManager::getInstance()->getModule(moduleName);
    if (!mod) { request->send(404, "application/json", "{\"error\":\"Module not found\"}"); return; }
    bool ok = false;
    if (action == "start") ok = mod->start();
    else if (action == "stop") ok = mod->stop();
    else if (action == "test") ok = mod->test();
    else { request->send(400, "application/json", "{\"error\":\"Invalid action\"}"); return; }
    request->send(200, "application/json", ok ? "{\"result\":\"OK\"}" : "{\"result\":\"FAIL\"}");
}

void CONTROL_WEB::handleAPIModuleConfig(AsyncWebServerRequest *request) {
    String moduleName = request->getParam("module")->value();
    Module* mod = ModuleManager::getInstance()->getModule(moduleName);
    
    if (mod) {
        DynamicJsonDocument statusDoc = mod->getStatus();
        String response;
        serializeJson(statusDoc, response);
        request->send(200, "application/json", response);
    } else {
        request->send(404, "application/json", "{\"error\":\"Module not found\"}");
    }
}

void CONTROL_WEB::handleAPIModuleSet(AsyncWebServerRequest *request) {
    String moduleName = request->getParam("module")->value();
    Module* fsModule = ModuleManager::getInstance()->getModule("CONTROL_FS");
    if (!fsModule) { request->send(503, "text/plain", "FS not available"); return; }
    CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsModule);
    ConfigManager* cfg = fs->getConfigManager();
    if (!cfg || !cfg->getConfiguration()) { request->send(500, "text/plain", "ConfigManager not ready"); return; }
    DynamicJsonDocument& doc = *cfg->getConfiguration();
    if (request->hasParam("json")) {
        String jsonStr = request->getParam("json")->value();
        DynamicJsonDocument modDoc(2048);
        DeserializationError err = deserializeJson(modDoc, jsonStr.c_str());
        if (err) { request->send(400, "text/plain", "JSON error"); return; }
        if (cfg && !cfg->validateModuleConfig(moduleName, modDoc)) { request->send(400, "text/plain", "Module config invalid"); return; }
        doc[moduleName] = modDoc.as<JsonObject>();
    } else if (request->hasParam("key") && request->hasParam("value")) {
        String key = request->getParam("key")->value();
        String value = request->getParam("value")->value();
        JsonObject mod = doc[moduleName];
        mod[key] = value;
        if (cfg && !cfg->validateModuleConfig(moduleName, mod)) { request->send(400, "text/plain", "Module config invalid"); return; }
    } else {
        request->send(400, "text/plain", "Missing params");
        return;
    }
    if (!cfg->saveConfiguration()) { request->send(500, "text/plain", "Save failed"); return; }
    ModuleManager::getInstance()->loadGlobalConfig();
    request->send(200, "text/plain", "OK");
}

void CONTROL_WEB::handleAPIModuleAutostart(AsyncWebServerRequest *request) {
    String moduleName = request->getParam("module")->value();
    String val = request->getParam("value")->value();
    Module* mod = ModuleManager::getInstance()->getModule(moduleName);
    if (!mod) { request->send(404, "text/plain", "Module not found"); return; }
    mod->setAutoStart(val == "on");
    request->send(200, "text/plain", "OK");
}

void CONTROL_WEB::handleAPILogs(AsyncWebServerRequest *request) {
    Module* fsModule = ModuleManager::getInstance()->getModule("CONTROL_FS");
    
    if (fsModule) {
        CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsModule);
        String logs;
        if (request->hasParam("level") && request->getParam("level")->value() == "debug") {
            logs = fs->readFile("/logs/debug.log");
        } else {
            logs = fs->readLogs(100);
        }
        if (request->hasParam("module")) {
            String name = request->getParam("module")->value();
            // naive filter: include lines containing "][<name>]"
            String filtered = "";
            int start = 0;
            while (true) {
                int nl = logs.indexOf('\n', start);
                String line = nl >= 0 ? logs.substring(start, nl + 1) : logs.substring(start);
                if (line.length() == 0) break;
                if (line.indexOf("][" + name + "]") >= 0) filtered += line;
                if (nl < 0) break;
                start = nl + 1;
            }
            logs = filtered;
        }
        
        DynamicJsonDocument doc(2048);
        doc["logs"] = logs;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    } else {
        request->send(503, "application/json", "{\"error\":\"FS module not available\"}");
    }
}

void CONTROL_WEB::handleAPITest(AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"message\":\"Test endpoint\"}");
}

void CONTROL_WEB::handleAPIModuleCommand(AsyncWebServerRequest *request) {
    if (!request->hasParam("module") || !request->hasParam("command")) {
        request->send(400, "application/json", "{\"error\":\"Missing module or command parameter\"}");
        return;
    }
    
    String moduleName = request->getParam("module")->value();
    String command = request->getParam("command")->value();
    
    // Get the module
    Module* mod = ModuleManager::getInstance()->getModule(moduleName);
    if (!mod) {
        request->send(404, "application/json", "{\"error\":\"Module not found\"}");
        return;
    }
    
    // Safety validation for critical commands
    if (command == "restart" || command == "clearlogs" || command == "factoryreset") {
        // Check if confirmation parameter is provided
        if (!request->hasParam("confirm") || request->getParam("confirm")->value() != "yes") {
            request->send(400, "application/json", "{\"error\":\"This command requires confirmation. Add ?confirm=yes to proceed\"}");
            return;
        }
    }
    
    // Check safety limits
    if (command == "restart" && millis() < 30000) { // Prevent restart within first 30 seconds
        request->send(400, "application/json", "{\"error\":\"System restart blocked for safety (wait 30s after boot)\"}");
        return;
    }
    
    // Handle module-specific commands
    DynamicJsonDocument response(512);
    bool success = false;
    String message = "";
    
    if (command == "restart") {
        // Restart the specific module
        success = mod->stop() && mod->start();
        message = success ? "Module restarted successfully" : "Failed to restart module";
    } else if (command == "test") {
        // Test the module
        success = mod->test();
        message = success ? "Module test passed" : "Module test failed";
    } else if (command == "status") {
        // Get detailed status
        DynamicJsonDocument status = mod->getStatus();
        String statusStr;
        serializeJson(status, statusStr);
        request->send(200, "application/json", statusStr);
        return;
    } else if (command == "clearlogs") {
        // Clear logs for this module (if FS is available)
        Module* fsModule = ModuleManager::getInstance()->getModule("CONTROL_FS");
        if (fsModule) {
            CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsModule);
            String logFile = "/logs/" + moduleName + ".log";
            success = fs->writeFile(logFile, ""); // Clear the log file
            message = success ? "Module logs cleared" : "Failed to clear module logs";
        } else {
            message = "FS module not available";
        }
    } else if (command == "config") {
        // Get module configuration
        DynamicJsonDocument status = mod->getStatus();
        if (status.containsKey("config")) {
            String configStr;
            serializeJson(status["config"], configStr);
            request->send(200, "application/json", configStr);
            return;
        } else {
            message = "No configuration available for this module";
        }
    } else {
        // Unknown command
        request->send(400, "application/json", "{\"error\":\"Unknown command\"}");
        return;
    }
    
    response["success"] = success;
    response["message"] = message;
    
    String responseStr;
    serializeJson(response, responseStr);
    request->send(200, "application/json", responseStr);
}

void CONTROL_WEB::handleAPIConfigBackup(AsyncWebServerRequest *request) {
    Module* fsModule = ModuleManager::getInstance()->getModule("CONTROL_FS");
    if (!fsModule) {
        request->send(503, "application/json", "{\"error\":\"FS module not available\"}");
        return;
    }
    
    CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsModule);
    ConfigManager* configManager = fs->getConfigManager();
    
    if (!configManager) {
        request->send(503, "application/json", "{\"error\":\"ConfigManager not available\"}");
        return;
    }
    
    // Create backup
    bool success = configManager->createBackup();
    
    DynamicJsonDocument response(256);
    response["success"] = success;
    response["message"] = success ? "Configuration backup created successfully" : "Failed to create backup";
    
    String responseStr;
    serializeJson(response, responseStr);
    request->send(200, "application/json", responseStr);
}

void CONTROL_WEB::handleAPIConfigValidate(AsyncWebServerRequest *request) {
    Module* fsModule = ModuleManager::getInstance()->getModule("CONTROL_FS");
    if (!fsModule) {
        request->send(503, "application/json", "{\"error\":\"FS module not available\"}");
        return;
    }
    
    CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsModule);
    ConfigManager* configManager = fs->getConfigManager();
    
    if (!configManager) {
        request->send(503, "application/json", "{\"error\":\"ConfigManager not available\"}");
        return;
    }
    
    // Validate current configuration
    ConfigValidationResult result = configManager->validateConfiguration();
    DynamicJsonDocument response(512);
    response["result_code"] = (int)result;
    response["message"] = configManager->getValidationErrorString(result);
    response["version"] = configManager->getCurrentVersion();
    
    String responseStr;
    serializeJson(response, responseStr);
    request->send(200, "application/json", responseStr);
}

void CONTROL_WEB::handleAPIConfigExport(AsyncWebServerRequest *request) {
    Module* fsModule = ModuleManager::getInstance()->getModule("CONTROL_FS");
    if (!fsModule) {
        request->send(503, "application/json", "{\"error\":\"FS module not available\"}");
        return;
    }
    
    CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsModule);
    ConfigManager* configManager = fs->getConfigManager();
    
    if (!configManager) {
        request->send(503, "application/json", "{\"error\":\"ConfigManager not available\"}");
        return;
    }
    
    // Get current configuration
    DynamicJsonDocument* cfgDoc = configManager->getConfiguration();
    if (cfgDoc) {
        String configStr;
        serializeJsonPretty(*cfgDoc, configStr);
        request->send(200, "application/json", configStr);
    } else {
        request->send(500, "application/json", "{\"error\":\"Failed to export configuration\"}");
    }
}

void CONTROL_WEB::handleAPIConfigImport(AsyncWebServerRequest *request) {
    if (!request->hasParam("plain", true)) {
        request->send(400, "application/json", "{\"error\":\"No configuration data provided\"}");
        return;
    }
    
    String configData = request->getParam("plain", true)->value();
    
    Module* fsModule = ModuleManager::getInstance()->getModule("CONTROL_FS");
    if (!fsModule) {
        request->send(503, "application/json", "{\"error\":\"FS module not available\"}");
        return;
    }
    
    CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsModule);
    ConfigManager* configManager = fs->getConfigManager();
    
    if (!configManager) {
        request->send(503, "application/json", "{\"error\":\"ConfigManager not available\"}");
        return;
    }
    
    // Parse the imported configuration
    DynamicJsonDocument newConfig(4096);
    DeserializationError error = deserializeJson(newConfig, configData);
    
    if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON format\"}");
        return;
    }
    
    // Validate the imported configuration
    ConfigValidationResult vres = configManager->validateConfiguration(newConfig);
    if (vres != CONFIG_VALID) {
        DynamicJsonDocument response(256);
        response["error"] = configManager->getValidationErrorString(vres);
        String responseStr;
        serializeJson(response, responseStr);
        request->send(400, "application/json", responseStr);
        return;
    }
    
    // Apply and save the validated configuration
    DynamicJsonDocument* cfgDoc = configManager->getConfiguration();
    if (cfgDoc) {
        *cfgDoc = newConfig;
        if (configManager->saveConfiguration()) {
            ModuleManager::getInstance()->loadGlobalConfig();
            request->send(200, "application/json", "{\"success\":true,\"message\":\"Configuration imported successfully\"}");
            return;
        }
    }
    request->send(500, "application/json", "{\"error\":\"Failed to save imported configuration\"}");
}

void CONTROL_WEB::handleAPISystemInfo(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(1024);
    
    // ESP32 system information
    doc["chip_model"] = ESP.getChipModel();
    doc["chip_revision"] = ESP.getChipRevision();
    doc["chip_cores"] = ESP.getChipCores();
    doc["cpu_freq"] = ESP.getCpuFreqMHz();
    doc["flash_size"] = ESP.getFlashChipSize();
    doc["flash_speed"] = ESP.getFlashChipSpeed();
    
    // Memory information
    doc["free_heap"] = ESP.getFreeHeap();
    doc["total_heap"] = ESP.getHeapSize();
    doc["min_free_heap"] = ESP.getMinFreeHeap();
    doc["max_alloc_heap"] = ESP.getMaxAllocHeap();
    
    // System uptime
    doc["uptime_seconds"] = millis() / 1000;
    
    // SDK version
    doc["sdk_version"] = ESP.getSdkVersion();
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void CONTROL_WEB::handleAPISystemStats(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(2048);
    
    // Module statistics
    JsonArray modules = doc["modules"].to<JsonArray>();
    for (Module* mod : ModuleManager::getInstance()->getModules()) {
        JsonObject modStats = modules.createNestedObject();
        modStats["name"] = mod->getName();
        modStats["state"] = mod->getState();
        modStats["priority"] = mod->getPriority();
        modStats["auto_start"] = mod->isAutoStart();
        modStats["version"] = mod->getVersion();
        
        // Get detailed status
        DynamicJsonDocument status = mod->getStatus();
        modStats["status"] = status;
    }
    
    // ConfigManager statistics if available
    Module* fsModule = ModuleManager::getInstance()->getModule("CONTROL_FS");
    if (fsModule) {
        CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsModule);
        ConfigManager* configManager = fs->getConfigManager();
        if (configManager) {
            JsonObject configStats = doc["config_manager"].to<JsonObject>();
            ConfigManager::ConfigStats stats = configManager->getStatistics();
            configStats["version"] = configManager->getCurrentVersion();
            configStats["backup_count"] = stats.backupCount;
            configStats["config_size"] = stats.configSize;
            configStats["total_backup_size"] = stats.totalBackupSize;
            configStats["last_backup_time"] = stats.lastBackupTime;
        }
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void CONTROL_WEB::handleAPISafetyLimits(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(512);
    
    // System-wide safety limits
    JsonObject limits = doc["safety_limits"].to<JsonObject>();
    limits["min_restart_uptime"] = 30000; // 30 seconds
    limits["max_command_length"] = 256;
    limits["max_config_size"] = 16384; // 16KB
    limits["max_backup_count"] = 10;
    limits["validation_timeout"] = 5000; // 5 seconds
    
    // Critical commands that require confirmation
    JsonArray criticalCommands = doc["critical_commands"].to<JsonArray>();
    criticalCommands.add("restart");
    criticalCommands.add("clearlogs");
    criticalCommands.add("factoryreset");
    criticalCommands.add("format");
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void CONTROL_WEB::handleAPISafetyStatus(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(512);
    
    // Current safety status
    JsonObject status = doc["safety_status"].to<JsonObject>();
    status["system_uptime"] = millis();
    status["can_restart"] = millis() >= 30000; // Can restart after 30 seconds
    status["config_valid"] = true; // Assume valid unless proven otherwise
    status["backup_available"] = false; // Will be updated below
    
    // Check if backup is available
    Module* fsModule = ModuleManager::getInstance()->getModule("CONTROL_FS");
    if (fsModule) {
        CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsModule);
        ConfigManager* configManager = fs->getConfigManager();
        if (configManager) {
            ConfigManager::ConfigStats stats = configManager->getStatistics();
            status["backup_available"] = stats.backupCount > 0;
        }
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

String CONTROL_WEB::getModulesHTML() {
    String html = "<h2>Modules Status</h2>";
    html += "<table>";
    html += "<tr><th>Module</th><th>State</th><th>Priority</th><th>Version</th><th>Auto Start</th></tr>";
    
    auto modules = ModuleManager::getInstance()->getModules();
    for (Module* mod : modules) {
        html += "<tr>";
        html += "<td>" + mod->getName() + "</td>";
        html += "<td>" + String(mod->getState() == MODULE_ENABLED ? "Enabled" : "Disabled") + "</td>";
        html += "<td>" + String(mod->getPriority()) + "</td>";
        html += "<td>" + mod->getVersion() + "</td>";
        html += "<td>" + String(mod->isAutoStart() ? "Yes" : "No") + "</td>";
        html += "</tr>";
    }
    
    html += "</table>";
    return html;
}

String CONTROL_WEB::getLogsHTML() {
    Module* fsModule = ModuleManager::getInstance()->getModule("CONTROL_FS");
    
    if (!fsModule) {
        return "<p>FS module not available</p>";
    }
    
    CONTROL_FS* fs = static_cast<CONTROL_FS*>(fsModule);
    String logs = fs->readLogs(100);
    
    String html = "<pre>" + logs + "</pre>";
    return html;
}

String CONTROL_WEB::getConfigHTML() {
    String html = "<h2>Module Configuration</h2>";
    
    auto modules = ModuleManager::getInstance()->getModules();
    for (Module* mod : modules) {
        html += "<div class='module'>";
        html += "<h3>" + mod->getName() + "</h3>";
        
        DynamicJsonDocument statusDoc = mod->getStatus();
        String status;
        serializeJsonPretty(statusDoc, status);
        
        html += "<pre>" + status + "</pre>";
        html += "</div>";
    }
    
    return html;
}

String CONTROL_WEB::buildHTML(const String& title, const String& content) {
    String html;
    html += "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>" + title + "</title>";
    html += "<style>body{font-family:monospace;background:#111;color:#ddd;margin:0}header{padding:10px;background:#222}a{color:#0af;margin-right:10px}table{border-collapse:collapse}td,th{border:1px solid #444;padding:4px}hr{border-color:#333}.module{border:1px solid #333;padding:8px;margin:6px 0}</style>";
    html += "</head><body>";
    html += "<header><a href='/' >Home</a><a href='/logs'>Logs</a><a href='/display'>Display</a><a href='/controls'>Controls</a><a href='/config'>Configuration</a></header>";
    html += "<main style='padding:10px'>";
    html += content;
    html += "</main></body></html>";
    return html;
}

bool CONTROL_WEB::addRoute(const char* path, WebRequestMethodComposite method, 
                           ArRequestHandlerFunction handler) {
    if (!server) return false;
    server->on(path, method, handler);
    return true;
}

bool CONTROL_WEB::addAPIRoute(const char* path, WebRequestMethodComposite method,
                              ArRequestHandlerFunction handler) {
    String apiPath = String("/api") + path;
    return addRoute(apiPath.c_str(), method, handler);
}
