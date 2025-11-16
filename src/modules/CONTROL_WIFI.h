/**
 * @file CONTROL_WIFI.h
 * @brief WiFi control module interface.
 * @author Michael Kojdl
 * @email michael@kojdl.com
 * @date 2025-11-16
 * @version 1.0.1
 */
#ifndef CONTROL_WIFI_H
#define CONTROL_WIFI_H

#include "../ModuleManager.h"
#include <WiFi.h>

/**
 * @enum CustomWiFiMode
 * @brief Operating modes for WiFi subsystem.
 * @details OFF disables WiFi; AP provides access point; CLIENT connects to existing AP; APSTA runs both.
 */
enum CustomWiFiMode {
    CUSTOM_WIFI_OFF = 0,
    CUSTOM_WIFI_AP = 1,
    CUSTOM_WIFI_CLIENT = 2,
    CUSTOM_WIFI_APSTA = 3
};

/**
 * @struct WiFiConfig
 * @brief Configuration container for AP and Client modes.
 * @details Holds SSID, password, IP settings, DHCP flags, visibility, channel and connection limits.
 */
struct WiFiConfig {
    String ssid;
    String password;
    CustomWiFiMode mode;
    IPAddress ap_ip;
    IPAddress ap_gateway;
    IPAddress ap_subnet;
    IPAddress ap_dns1;
    IPAddress ap_dns2;
    bool ap_dhcp;
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

/**
 * @class CONTROL_WIFI
 * @brief Manages WiFi connectivity and access point functionality.
 * @details Provides configuration, connection management, scanning and status reporting.
 */
/**
 * @class CONTROL_WIFI
 * @brief WiFi connectivity management: AP/STA modes, scans, and status.
 */
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
    
    /** @brief Initialize WiFi module resources. @return True on success. */
    bool init() override;
    /** @brief Start WiFi according to configured mode. @return True on success. */
    bool start() override;
    /** @brief Stop WiFi services. @return True on success. */
    bool stop() override;
    /** @brief Periodic WiFi maintenance and reconnection checks. @return True if work done. */
    bool update() override;
    /** @brief Run self-test for WiFi module. @return True if tests pass. */
    bool test() override;
    /** @brief Current WiFi status as JSON. @return Status document. */
    DynamicJsonDocument getStatus() override;
    
    /** @brief Load configuration from JSON. @param doc Source document. @return True on success. */
    bool loadConfig(DynamicJsonDocument& doc) override;
    /** @brief Set network SSID. @param ssid SSID string. @return True if accepted. */
    bool setSSID(const String& ssid);
    /** @brief Set network password. @param password PSK string. @return True if accepted. */
    bool setPassword(const String& password);
    /** @brief Set operating mode. @param mode Mode enum. @return True if applied. */
    bool setMode(CustomWiFiMode mode);
    
    /** @brief Configure access point networking. @return True on success. */
    bool setAPConfig(IPAddress ip, IPAddress gateway, IPAddress subnet);
    /** @brief Enable AP DHCP with range. @param start Start IP. @param end End IP. @return True on success. */
    bool enableAPDHCP(IPAddress start, IPAddress end);
    
    /** @brief Configure client networking and DNS. @return True on success. */
    bool setClientConfig(IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns1, IPAddress dns2);
    /** @brief Toggle client DHCP. @param enable True to enable. @return True on success. */
    bool enableClientDHCP(bool enable);
    
    /** @brief Connection state flag. @return True if connected (client mode). */
    bool isWiFiConnected() { return isConnected; }
    /** @brief Current SSID. @return SSID string. */
    String getSSID();
    /** @brief Current IP address. @return IP as string. */
    String getIP();
    /** @brief Signal strength in dBm. @return RSSI value. */
    int getRSSI();
    /** @brief MAC address. @return MAC as string. */
    String getMAC();
    /** @brief Number of connected AP clients. @return Client count. */
    int getConnectedClients();
    
    /** @brief Scan available WiFi networks. @return Count of networks found. */
    int scanNetworks();
    /** @brief SSID of scanned network. @param index Scan index. @return SSID string. */
    String getScannedSSID(int index);
    /** @brief RSSI of scanned network. @param index Scan index. @return RSSI value. */
    int getScannedRSSI(int index);
    /** @brief Encryption state of scanned network. @param index Scan index. @return True if encrypted. */
    bool getScannedEncryption(int index);

    /** @brief Disconnect from network (client) or stop AP. */
    void disconnect();
    /** @brief Reconnect to last configuration or restart AP. */
    void reconnect();
};

#endif
