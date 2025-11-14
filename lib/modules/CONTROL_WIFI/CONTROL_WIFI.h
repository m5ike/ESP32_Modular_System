#ifndef CONTROL_WIFI_H
#define CONTROL_WIFI_H

#include "ModuleBase.h"
#include <WiFi.h>
#include <WiFiAP.h>

enum WiFiMode {
    WIFI_MODE_OFF = 0,
    WIFI_MODE_AP = 1,
    WIFI_MODE_CLIENT = 2,
    WIFI_MODE_AP_CLIENT = 3
};

class CONTROL_WIFI : public ModuleBase {
private:
    WiFiMode wifiMode;
    String ssid;
    String password;
    IPAddress apIP;
    IPAddress apGateway;
    IPAddress apSubnet;
    bool dhcpEnabled;
    
    IPAddress clientIP;
    IPAddress clientGateway;
    IPAddress clientSubnet;
    IPAddress clientDNS1;
    IPAddress clientDNS2;
    
    bool isConnected;
    int clientCount;
    
public:
    CONTROL_WIFI() : ModuleBase("CONTROL_WIFI"), 
                     wifiMode(WIFI_MODE_AP),
                     isConnected(false),
                     clientCount(0),
                     dhcpEnabled(true) {
        priority = 90;
        autostart = true;
        
        ssid = WIFI_DEFAULT_SSID;
        password = WIFI_DEFAULT_PSK;
        
        apIP.fromString(WIFI_AP_IP);
        apGateway.fromString(WIFI_AP_GATEWAY);
        apSubnet.fromString(WIFI_AP_SUBNET);
    }
    
    bool init() override {
        log("INFO", "Initializing WiFi module...");
        
        WiFi.mode(WIFI_OFF);
        delay(100);
        
        return true;
    }
    
    bool start() override {
        log("INFO", "Starting WiFi module...");
        
        if (wifiMode == WIFI_MODE_AP || wifiMode == WIFI_MODE_AP_CLIENT) {
            return startAP();
        } else if (wifiMode == WIFI_MODE_CLIENT) {
            return startClient();
        }
        
        return false;
    }
    
    bool stop() override {
        log("INFO", "Stopping WiFi module...");
        
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        isConnected = false;
        
        return true;
    }
    
    bool test() override {
        log("INFO", "Testing WiFi module...");
        
        // Test WiFi chip
        if (!WiFi.getMode()) {
            log("ERROR", "WiFi chip not responding");
            return false;
        }
        
        log("INFO", "WiFi test passed");
        return true;
    }
    
    void loop() override {
        // Monitor connection status
        static unsigned long lastCheck = 0;
        if (millis() - lastCheck > 5000) { // Every 5 seconds
            if (wifiMode == WIFI_MODE_CLIENT || wifiMode == WIFI_MODE_AP_CLIENT) {
                isConnected = WiFi.isConnected();
                
                if (!isConnected && debugMode) {
                    log("WARN", "WiFi disconnected, attempting reconnect...");
                    WiFi.reconnect();
                }
            }
            
            if (wifiMode == WIFI_MODE_AP || wifiMode == WIFI_MODE_AP_CLIENT) {
                clientCount = WiFi.softAPgetStationNum();
                
                if (debugMode) {
                    logf("INFO", "AP clients: %d", clientCount);
                }
            }
            
            lastCheck = millis();
        }
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
        
        // Load config
        ssid = (*config)["ssid"] | WIFI_DEFAULT_SSID;
        password = (*config)["password"] | WIFI_DEFAULT_PSK;
        wifiMode = (WiFiMode)((*config)["mode"] | WIFI_MODE_AP);
        
        log("INFO", "Config loaded successfully");
        return true;
    }
    
    bool saveConfig() override {
        (*config)["ssid"] = ssid;
        (*config)["password"] = password;
        (*config)["mode"] = (int)wifiMode;
        
        // AP settings
        JsonObject ap = config->createNestedObject("ap");
        ap["ip"] = apIP.toString();
        ap["gateway"] = apGateway.toString();
        ap["subnet"] = apSubnet.toString();
        
        // Client settings
        JsonObject client = config->createNestedObject("client");
        client["dhcp"] = dhcpEnabled;
        client["ip"] = clientIP.toString();
        client["gateway"] = clientGateway.toString();
        client["subnet"] = clientSubnet.toString();
        client["dns1"] = clientDNS1.toString();
        client["dns2"] = clientDNS2.toString();
        
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
    
    // WiFi AP Functions
    bool startAP() {
        logf("INFO", "Starting AP mode: %s", ssid.c_str());
        
        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(apIP, apGateway, apSubnet);
        
        if (!WiFi.softAP(ssid.c_str(), password.c_str())) {
            log("ERROR", "Failed to start AP");
            return false;
        }
        
        logf("INFO", "AP started - IP: %s", WiFi.softAPIP().toString().c_str());
        return true;
    }
    
    // WiFi Client Functions
    bool startClient() {
        logf("INFO", "Connecting to: %s", ssid.c_str());
        
        WiFi.mode(WIFI_STA);
        
        if (!dhcpEnabled) {
            WiFi.config(clientIP, clientGateway, clientSubnet, clientDNS1, clientDNS2);
        }
        
        WiFi.begin(ssid.c_str(), password.c_str());
        
        // Wait for connection
        int timeout = 20; // 20 seconds
        while (WiFi.status() != WL_CONNECTED && timeout > 0) {
            delay(1000);
            timeout--;
            
            if (debugMode) {
                logf("INFO", "Connecting... (%d)", timeout);
            }
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            isConnected = true;
            logf("INFO", "Connected - IP: %s", WiFi.localIP().toString().c_str());
            return true;
        }
        
        log("ERROR", "Failed to connect");
        return false;
    }
    
    // Getters
    WiFiMode getMode() const { return wifiMode; }
    String getSSID() const { return ssid; }
    bool getIsConnected() const { return isConnected; }
    int getClientCount() const { return clientCount; }
    IPAddress getIP() const {
        if (wifiMode == WIFI_MODE_AP) {
            return WiFi.softAPIP();
        }
        return WiFi.localIP();
    }
    
    // Setters
    void setMode(WiFiMode mode) { wifiMode = mode; }
    void setSSID(const String& s) { ssid = s; }
    void setPassword(const String& p) { password = p; }
    
    String getStatusJson() override {
        DynamicJsonDocument doc(1024);
        doc["name"] = moduleName;
        doc["state"] = (int)state;
        doc["mode"] = (int)wifiMode;
        doc["ssid"] = ssid;
        doc["connected"] = isConnected;
        doc["ip"] = getIP().toString();
        doc["clients"] = clientCount;
        doc["rssi"] = WiFi.RSSI();
        
        String output;
        serializeJson(doc, output);
        return output;
    }
};

#endif // CONTROL_WIFI_H
