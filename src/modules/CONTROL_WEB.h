/**
 * @file CONTROL_WEB.h
 * @brief Web server control module interface.
 * @author Michael Kojdl
 * @email michael@kojdl.com
 * @date 2025-11-16
 * @version 1.0.1
 */
#ifndef CONTROL_WEB_H
#define CONTROL_WEB_H

#include "../ModuleManager.h"
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

/**
 * @class CONTROL_WEB
 * @brief Provides HTTP routes, UI rendering, and REST API for system control.
 */
/**
 * @class CONTROL_WEB
 * @brief Asynchronous web UI and REST API provider.
 * @details Sets up routes and handlers for HTML pages and JSON API endpoints.
 */
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
    void handleAPIModuleSet(AsyncWebServerRequest *request);
    void handleAPIModuleAutostart(AsyncWebServerRequest *request);
    void handleAPIModuleCommand(AsyncWebServerRequest *request);
    void handleAPIConfigBackup(AsyncWebServerRequest *request);
    void handleAPIConfigValidate(AsyncWebServerRequest *request);
    void handleAPIConfigExport(AsyncWebServerRequest *request);
    void handleAPIConfigImport(AsyncWebServerRequest *request);
    void handleAPISystemInfo(AsyncWebServerRequest *request);
    void handleAPISystemStats(AsyncWebServerRequest *request);
    void handleAPISafetyLimits(AsyncWebServerRequest *request);
    void handleAPISafetyStatus(AsyncWebServerRequest *request);
    void handleAPILogs(AsyncWebServerRequest *request);
    void handleAPIRadar(AsyncWebServerRequest *request);
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
