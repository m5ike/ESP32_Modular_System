#ifndef CONTROL_WEB_H
#define CONTROL_WEB_H

#include "../ModuleManager.h"
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

class CONTROL_WEB : public Module {
private:
    AsyncWebServer* server;
    bool serverRunning;
    uint16_t port;
    
    // Setup routes
    void setupRoutes();
    void setupAPIRoutes();
    
    // Route handlers
    void handleRoot(AsyncWebServerRequest *request);
    void handleLogs(AsyncWebServerRequest *request);
    void handleDisplay(AsyncWebServerRequest *request);
    void handleControls(AsyncWebServerRequest *request);
    void handleConfig(AsyncWebServerRequest *request);
    void handleNotFound(AsyncWebServerRequest *request);
    
    // API handlers
    void handleAPIStatus(AsyncWebServerRequest *request);
    void handleAPIModules(AsyncWebServerRequest *request);
    void handleAPIModuleControl(AsyncWebServerRequest *request);
    void handleAPIModuleConfig(AsyncWebServerRequest *request);
    void handleAPILogs(AsyncWebServerRequest *request);
    void handleAPITest(AsyncWebServerRequest *request);
    
    // Helper functions
    String buildHTML(const String& title, const String& content);
    String getModulesHTML();
    String getLogsHTML();
    String getConfigHTML();
    
public:
    CONTROL_WEB();
    ~CONTROL_WEB();
    
    // Module interface
    bool init() override;
    bool start() override;
    bool stop() override;
    bool update() override;
    bool test() override;
    DynamicJsonDocument getStatus() override;
    
    // Server control
    AsyncWebServer* getServer() { return server; }
    bool isRunning() { return serverRunning; }
    uint16_t getPort() { return port; }
    void setPort(uint16_t p) { port = p; }
    
    // Custom routes
    bool addRoute(const char* path, WebRequestMethodComposite method, 
                  ArRequestHandlerFunction handler);
    bool addAPIRoute(const char* path, WebRequestMethodComposite method,
                     ArRequestHandlerFunction handler);
};

#endif
