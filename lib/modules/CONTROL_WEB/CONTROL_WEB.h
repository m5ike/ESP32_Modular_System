#ifndef CONTROL_WEB_H
#define CONTROL_WEB_H

#include "ModuleBase.h"
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

// Forward declaration
class ModuleManager;

class CONTROL_WEB : public ModuleBase {
private:
    AsyncWebServer* server;
    ModuleManager* moduleManager;
    uint16_t port;
    unsigned long requestCount;
    
    void setupRoutes() {
        if (!server) return;
        
        // Root page
        server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
            this->handleRoot(request);
        });
        
        // API endpoints
        server->on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request){
            this->handleApiStatus(request);
        });
        
        server->on("/api/modules", HTTP_GET, [this](AsyncWebServerRequest *request){
            this->handleApiModules(request);
        });
        
        server->on("/api/module/start", HTTP_POST, [this](AsyncWebServerRequest *request){
            this->handleApiModuleStart(request);
        });
        
        server->on("/api/module/stop", HTTP_POST, [this](AsyncWebServerRequest *request){
            this->handleApiModuleStop(request);
        });
        
        server->on("/api/module/config", HTTP_GET, [this](AsyncWebServerRequest *request){
            this->handleApiModuleConfig(request);
        });
        
        server->on("/api/logs", HTTP_GET, [this](AsyncWebServerRequest *request){
            this->handleApiLogs(request);
        });
        
        // LCD display endpoint
        server->on("/display", HTTP_GET, [this](AsyncWebServerRequest *request){
            this->handleDisplay(request);
        });
        
        // Controls endpoint
        server->on("/controls", HTTP_GET, [this](AsyncWebServerRequest *request){
            this->handleControls(request);
        });
        
        // Configuration endpoint
        server->on("/config", HTTP_GET, [this](AsyncWebServerRequest *request){
            this->handleConfig(request);
        });
        
        // 404 handler
        server->onNotFound([](AsyncWebServerRequest *request){
            request->send(404, "text/plain", "Not found");
        });
        
        logf("INFO", "Web routes configured");
    }
    
    void handleRoot(AsyncWebServerRequest *request) {
        requestCount++;
        
        String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Modular System</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { 
            font-family: Arial, sans-serif; 
            margin: 20px;
            background: #f0f0f0;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
            background: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        h1 { color: #333; }
        .nav {
            margin: 20px 0;
            border-bottom: 2px solid #ddd;
        }
        .nav a {
            display: inline-block;
            padding: 10px 20px;
            margin-right: 10px;
            text-decoration: none;
            color: #333;
            border-bottom: 3px solid transparent;
        }
        .nav a:hover, .nav a.active {
            border-bottom-color: #007bff;
            color: #007bff;
        }
        .status-box {
            padding: 15px;
            margin: 10px 0;
            border-radius: 5px;
            background: #e9ecef;
        }
        .btn {
            padding: 10px 20px;
            margin: 5px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            background: #007bff;
            color: white;
        }
        .btn:hover { background: #0056b3; }
        .btn-danger { background: #dc3545; }
        .btn-danger:hover { background: #c82333; }
        .module-card {
            border: 1px solid #ddd;
            padding: 15px;
            margin: 10px 0;
            border-radius: 5px;
        }
        .module-card.running { border-left: 4px solid #28a745; }
        .module-card.stopped { border-left: 4px solid #dc3545; }
        .module-card.disabled { border-left: 4px solid #6c757d; }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32 Modular System</h1>
        <div class="nav">
            <a href="/" class="active">Dashboard</a>
            <a href="/display">Display</a>
            <a href="/controls">Controls</a>
            <a href="/config">Configuration</a>
            <a href="/api/status">API Status</a>
        </div>
        
        <div class="status-box">
            <h2>System Status</h2>
            <p><strong>Uptime:</strong> <span id="uptime">Loading...</span></p>
            <p><strong>Free Memory:</strong> <span id="memory">Loading...</span></p>
            <p><strong>Modules:</strong> <span id="moduleCount">Loading...</span></p>
        </div>
        
        <div id="modules">
            <h2>Modules</h2>
            <p>Loading modules...</p>
        </div>
    </div>
    
    <script>
        function loadStatus() {
            fetch('/api/status')
                .then(r => r.json())
                .then(data => {
                    document.getElementById('uptime').textContent = data.uptime + 's';
                    document.getElementById('memory').textContent = data.free_heap + ' bytes';
                    document.getElementById('moduleCount').textContent = data.module_count;
                });
        }
        
        function loadModules() {
            fetch('/api/modules')
                .then(r => r.json())
                .then(data => {
                    let html = '<h2>Modules</h2>';
                    data.modules.forEach(m => {
                        let stateClass = m.state === 4 ? 'running' : 
                                       m.state === 0 ? 'disabled' : 'stopped';
                        let stateName = m.state === 4 ? 'Running' : 
                                      m.state === 0 ? 'Disabled' : 'Stopped';
                        
                        html += `
                            <div class="module-card ${stateClass}">
                                <h3>${m.name}</h3>
                                <p><strong>State:</strong> ${stateName} | 
                                   <strong>Priority:</strong> ${m.priority} | 
                                   <strong>Version:</strong> ${m.version}</p>
                                <button class="btn" onclick="startModule('${m.name}')">Start</button>
                                <button class="btn btn-danger" onclick="stopModule('${m.name}')">Stop</button>
                            </div>
                        `;
                    });
                    document.getElementById('modules').innerHTML = html;
                });
        }
        
        function startModule(name) {
            fetch('/api/module/start?name=' + name, {method: 'POST'})
                .then(() => loadModules());
        }
        
        function stopModule(name) {
            fetch('/api/module/stop?name=' + name, {method: 'POST'})
                .then(() => loadModules());
        }
        
        loadStatus();
        loadModules();
        setInterval(loadStatus, 5000);
        setInterval(loadModules, 10000);
    </script>
</body>
</html>
        )";
        
        request->send(200, "text/html", html);
    }
    
    void handleApiStatus(AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(1024);
        doc["uptime"] = millis() / 1000;
        doc["free_heap"] = ESP.getFreeHeap();
        doc["chip_model"] = ESP.getChipModel();
        doc["cpu_freq"] = ESP.getCpuFreqMHz();
        doc["module_count"] = moduleManager ? moduleManager->getModuleCount() : 0;
        doc["requests"] = requestCount;
        
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    }
    
    void handleApiModules(AsyncWebServerRequest *request) {
        if (!moduleManager) {
            request->send(500, "application/json", "{\"error\":\"Module manager not available\"}");
            return;
        }
        
        String json = moduleManager->getModulesJson();
        request->send(200, "application/json", json);
    }
    
    void handleApiModuleStart(AsyncWebServerRequest *request) {
        if (!request->hasParam("name")) {
            request->send(400, "application/json", "{\"error\":\"Missing name parameter\"}");
            return;
        }
        
        String name = request->getParam("name")->value();
        
        if (!moduleManager) {
            request->send(500, "application/json", "{\"error\":\"Module manager not available\"}");
            return;
        }
        
        bool success = moduleManager->startModule(name);
        String response = success ? "{\"success\":true}" : "{\"success\":false,\"error\":\"Failed to start module\"}";
        request->send(success ? 200 : 500, "application/json", response);
    }
    
    void handleApiModuleStop(AsyncWebServerRequest *request) {
        if (!request->hasParam("name")) {
            request->send(400, "application/json", "{\"error\":\"Missing name parameter\"}");
            return;
        }
        
        String name = request->getParam("name")->value();
        
        if (!moduleManager) {
            request->send(500, "application/json", "{\"error\":\"Module manager not available\"}");
            return;
        }
        
        bool success = moduleManager->stopModule(name);
        String response = success ? "{\"success\":true}" : "{\"success\":false,\"error\":\"Failed to stop module\"}";
        request->send(success ? 200 : 500, "application/json", response);
    }
    
    void handleApiModuleConfig(AsyncWebServerRequest *request) {
        // Implementation for getting module config
        request->send(200, "application/json", "{\"config\":{}}");
    }
    
    void handleApiLogs(AsyncWebServerRequest *request) {
        // Implementation for getting logs
        request->send(200, "application/json", "{\"logs\":[]}");
    }
    
    void handleDisplay(AsyncWebServerRequest *request) {
        String html = "<html><body><h1>LCD Display View</h1><p>Display rendering will be implemented here</p></body></html>";
        request->send(200, "text/html", html);
    }
    
    void handleControls(AsyncWebServerRequest *request) {
        String html = "<html><body><h1>Controls</h1><p>Control interface will be implemented here</p></body></html>";
        request->send(200, "text/html", html);
    }
    
    void handleConfig(AsyncWebServerRequest *request) {
        String html = "<html><body><h1>Configuration</h1><p>Configuration interface will be implemented here</p></body></html>";
        request->send(200, "text/html", html);
    }
    
public:
    CONTROL_WEB() : ModuleBase("CONTROL_WEB"), 
                    server(nullptr),
                    moduleManager(nullptr),
                    port(WEB_SERVER_PORT),
                    requestCount(0) {
        priority = 75;
        autostart = true;
    }
    
    ~CONTROL_WEB() {
        if (server) {
            delete server;
        }
    }
    
    void setModuleManager(ModuleManager* mgr) {
        moduleManager = mgr;
    }
    
    bool init() override {
        log("INFO", "Initializing Web module...");
        
        server = new AsyncWebServer(port);
        setupRoutes();
        
        return true;
    }
    
    bool start() override {
        log("INFO", "Starting Web module...");
        
        if (!server) {
            log("ERROR", "Server not initialized");
            return false;
        }
        
        server->begin();
        logf("INFO", "Web server started on port %d", port);
        
        return true;
    }
    
    bool stop() override {
        log("INFO", "Stopping Web module...");
        
        if (server) {
            server->end();
        }
        
        return true;
    }
    
    bool test() override {
        log("INFO", "Testing Web module...");
        
        if (!server) {
            log("ERROR", "Server not initialized");
            return false;
        }
        
        log("INFO", "Web test passed");
        return true;
    }
    
    void loop() override {
        // Async server handles requests automatically
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
        
        port = (*config)["port"] | WEB_SERVER_PORT;
        
        log("INFO", "Config loaded successfully");
        return true;
    }
    
    bool saveConfig() override {
        (*config)["port"] = port;
        (*config)["enable_cors"] = true;
        (*config)["max_clients"] = 4;
        
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
    
    AsyncWebServer* getServer() { return server; }
    unsigned long getRequestCount() const { return requestCount; }
};

#endif // CONTROL_WEB_H
