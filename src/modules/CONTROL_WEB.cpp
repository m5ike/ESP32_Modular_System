#include "CONTROL_WEB.h"
#include "CONTROL_FS.h"
#include "CONTROL_LCD.h"
#include "CONTROL_WIFI.h"

CONTROL_WEB::CONTROL_WEB() : Module("CONTROL_WEB") {
    server = nullptr;
    serverRunning = false;
    port = 80;
    priority = 70;
    autoStart = true;
    version = "1.0.0";
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
    DynamicJsonDocument doc(1024);
    
    doc["module"] = moduleName;
    doc["state"] = state == MODULE_ENABLED ? "enabled" : "disabled";
    doc["version"] = version;
    doc["priority"] = priority;
    doc["autoStart"] = autoStart;
    doc["debug"] = debugEnabled;
    
    doc["running"] = serverRunning;
    doc["port"] = port;
    
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
    
    // API Module control (start/stop/test)
    server->on("/api/module/control", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleAPIModuleControl(request);
    });
    
    // API Module configuration
    server->on("/api/module/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleAPIModuleConfig(request);
    });
    server->on("/api/module/set", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleAPIModuleSet(request);
    });
    server->on("/api/module/autostart", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleAPIModuleAutostart(request);
    });
    
    // API Logs
    server->on("/api/logs", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleAPILogs(request);
    });
    
    // API Test
    server->on("/api/test", HTTP_POST, [this](AsyncWebServerRequest *request) {
        this->handleAPITest(request);
    });
}

void CONTROL_WEB::handleRoot(AsyncWebServerRequest *request) {
    String content = "<h1>ESP32 Modular System</h1>";
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
    String content = "<h1>System Logs</h1>";
    content += "<a href='/'>Back to Home</a><hr>";
    content += getLogsHTML();
    
    request->send(200, "text/html", buildHTML("Logs", content));
}

void CONTROL_WEB::handleDisplay(AsyncWebServerRequest *request) {
    String content = "<h1>Display</h1>";
    content += "<a href='/'>Back to Home</a><hr>";
    content += "<p>Display mirroring would be implemented here</p>";
    content += "<canvas id='display' width='170' height='320'></canvas>";
    
    request->send(200, "text/html", buildHTML("Display", content));
}

void CONTROL_WEB::handleControls(AsyncWebServerRequest *request) {
    String content = "<h1>Controls</h1>";
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
        content += "<div><button onclick=\"showLogs('" + mod->getName() + "')\">Show Logs</button><pre id='logs_" + mod->getName() + "'></pre></div>";
        content += "</div>";
    }
    
    content += "<script>";
    content += "function controlModule(name, action){fetch('/api/module/control?module='+name+'&action='+action).then(r=>r.json()).then(d=>{alert(JSON.stringify(d));location.reload();});}";
    content += "function setAutostart(name,val){fetch('/api/module/autostart?module='+name+'&value='+val).then(r=>r.text()).then(t=>{alert(t);location.reload();});}";
    content += "function toggleEnable(name,val){if(val==='on'){fetch('/api/module/control?module='+name+'&action=start').then(()=>location.reload());}else{fetch('/api/module/control?module='+name+'&action=stop').then(()=>location.reload());}}";
    content += "function showLogs(name){fetch('/api/logs?module='+name).then(r=>r.json()).then(d=>{document.getElementById('logs_'+name).textContent=d.logs;});}";
    content += "</script>";
    
    request->send(200, "text/html", buildHTML("Controls", content));
}

void CONTROL_WEB::handleConfig(AsyncWebServerRequest *request) {
    String content = "<h1>Configuration</h1>";
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
    DynamicJsonDocument doc(8192);
    if (!fs->loadGlobalConfig(doc)) { request->send(500, "text/plain", "Load failed"); return; }
    if (request->hasParam("json")) {
        String jsonStr = request->getParam("json")->value();
        DynamicJsonDocument modDoc(2048);
        DeserializationError err = deserializeJson(modDoc, jsonStr.c_str());
        if (err) { request->send(400, "text/plain", "JSON error"); return; }
        doc[moduleName] = modDoc.as<JsonObject>();
    } else if (request->hasParam("key") && request->hasParam("value")) {
        String key = request->getParam("key")->value();
        String value = request->getParam("value")->value();
        JsonObject mod = doc[moduleName];
        mod[key] = value;
    } else {
        request->send(400, "text/plain", "Missing params");
        return;
    }
    fs->saveGlobalConfig(doc);
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

String CONTROL_WEB::buildHTML(const String& title, const String& content) {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>" + title + " - ESP32</title>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }";
    html += "h1 { color: #333; }";
    html += ".menu { margin: 10px 0; }";
    html += ".menu a { padding: 5px 10px; background: #007bff; color: white; ";
    html += "text-decoration: none; border-radius: 3px; margin-right: 5px; }";
    html += ".menu a:hover { background: #0056b3; }";
    html += "table { border-collapse: collapse; width: 100%; margin: 10px 0; }";
    html += "td, th { border: 1px solid #ddd; padding: 8px; text-align: left; }";
    html += "th { background-color: #4CAF50; color: white; }";
    html += ".module { background: white; padding: 10px; margin: 10px 0; border-radius: 5px; }";
    html += ".module-control { background: white; padding: 10px; margin: 10px 0; border-radius: 5px; }";
    html += "button { padding: 5px 15px; margin: 5px; cursor: pointer; }";
    html += "canvas { border: 1px solid #000; background: #000; }";
    html += "pre { background: #2d2d2d; color: #f8f8f2; padding: 10px; overflow-x: auto; }";
    html += "</style>";
    html += "</head><body>";
    html += content;
    html += "</body></html>";
    
    return html;
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
