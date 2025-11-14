#ifndef CONTROL_WIFI_H
#define CONTROL_WIFI_H

#include "../ModuleManager.h"
#include <WiFi.h>

enum CustomWiFiMode {
    CUSTOM_WIFI_OFF = 0,
    CUSTOM_WIFI_AP = 1,
    CUSTOM_WIFI_CLIENT = 2,
    CUSTOM_WIFI_APSTA = 3
};

struct WiFiConfig {
    String ssid;
    String password;
    CustomWiFiMode mode;
    
    // AP Mode settings
    IPAddress ap_ip;
    IPAddress ap_gateway;
    IPAddress ap_subnet;
    IPAddress ap_dns1;
    IPAddress ap_dns2;
    bool ap_dhcp;
    
    // Client Mode settings
    bool client_dhcp;
    IPAddress client_ip;
    IPAddress client_gateway;
    IPAddress client_subnet;
    IPAddress client_dns1;
    IPAddress client_dns2;
    
    int maxConnections;
    bool hidden;
    int channel;
};

class CONTROL_WIFI : public Module {
private:
    WiFiConfig config;
    bool wifiInitialized;
    bool isConnected;
    unsigned long lastConnectionCheck;
    unsigned long reconnectInterval;
    
    bool startAP();
    bool startClient();
    bool connectToNetwork();
    
public:
    CONTROL_WIFI();
    ~CONTROL_WIFI();
    
    // Module interface
    bool init() override;
    bool start() override;
    bool stop() override;
    bool update() override;
    bool test() override;
    DynamicJsonDocument getStatus() override;
    
    // Configuration
    bool loadConfig(DynamicJsonDocument& doc) override;
    bool setSSID(const String& ssid);
    bool setPassword(const String& password);
    bool setMode(CustomWiFiMode mode);
    
    // AP Mode
    bool setAPConfig(IPAddress ip, IPAddress gateway, IPAddress subnet);
    bool enableAPDHCP(IPAddress start, IPAddress end);
    
    // Client Mode
    bool setClientConfig(IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns1, IPAddress dns2);
    bool enableClientDHCP(bool enable);
    
    // Status
    bool isWiFiConnected() { return isConnected; }
    String getSSID();
    String getIP();
    int getRSSI();
    String getMAC();
    int getConnectedClients(); // For AP mode
    
    // Network scan
    int scanNetworks();
    String getScannedSSID(int index);
    int getScannedRSSI(int index);
    bool getScannedEncryption(int index);
    
    // Utility
    void disconnect();
    void reconnect();
};

#endif
