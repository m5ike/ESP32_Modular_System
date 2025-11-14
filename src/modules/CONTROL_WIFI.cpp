#include "CONTROL_WIFI.h"

CONTROL_WIFI::CONTROL_WIFI() : Module("CONTROL_WIFI") {
    wifiInitialized = false;
    isConnected = false;
    lastConnectionCheck = 0;
    reconnectInterval = 30000; // 30 seconds
    
    // Default configuration
    config.ssid = "ESP32-AP";
    config.password = "12345678";
    config.mode = CUSTOM_WIFI_CLIENT;
    config.ap_dhcp = true;
    config.client_dhcp = true;
    config.maxConnections = 4;
    config.hidden = false;
    config.channel = 1;
    
    // Default AP settings
    config.ap_ip = IPAddress(192, 168, 4, 1);
    config.ap_gateway = IPAddress(192, 168, 4, 1);
    config.ap_subnet = IPAddress(255, 255, 255, 0);
    
    priority = 85;
    autoStart = true;
    version = "1.0.0";
    TaskConfig tcfg = getTaskConfig();
    tcfg.name = "CONTROL_WIFI_TASK";
    tcfg.stackSize = 4096;
    tcfg.priority = 4;
    tcfg.core = 0;
    setTaskConfig(tcfg);
}

CONTROL_WIFI::~CONTROL_WIFI() {
    stop();
}

bool CONTROL_WIFI::init() {
    log("Initializing WiFi...");
    
    WiFi.mode(WIFI_OFF);
    delay(100);
    
    wifiInitialized = true;
    setState(MODULE_ENABLED);
    
    log("WiFi initialized");
    return true;
}

bool CONTROL_WIFI::start() {
    if (!wifiInitialized) {
        if (!init()) return false;
    }
    
    log("Starting WiFi in mode: " + String(config.mode));
    
    bool success = false;
    
    switch (config.mode) {
        case CUSTOM_WIFI_AP:
            success = startAP();
            break;
        case CUSTOM_WIFI_CLIENT:
            success = startClient();
            break;
        case CUSTOM_WIFI_APSTA:
            success = startAP() && startClient();
            break;
        default:
            log("Invalid WiFi mode", "ERROR");
            return false;
    }
    
    if (success) {
        setState(MODULE_ENABLED);
        log("WiFi started successfully");
    } else {
        setState(MODULE_ERROR);
        log("Failed to start WiFi", "ERROR");
    }
    
    return success;
}

bool CONTROL_WIFI::stop() {
    log("Stopping WiFi...");
    
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    
    isConnected = false;
    wifiInitialized = false;
    setState(MODULE_DISABLED);
    
    log("WiFi stopped");
    return true;
}

bool CONTROL_WIFI::update() {
    if (state != MODULE_ENABLED) return true;
    
    // Check connection status periodically
    if (millis() - lastConnectionCheck > reconnectInterval) {
        lastConnectionCheck = millis();
        
        if (config.mode == CUSTOM_WIFI_CLIENT || config.mode == CUSTOM_WIFI_APSTA) {
            bool connected = WiFi.status() == WL_CONNECTED;
            
            if (connected != isConnected) {
                isConnected = connected;
                
                if (isConnected) {
                    log("WiFi connected: " + WiFi.localIP().toString());
                } else {
                    log("WiFi disconnected", "WARN");
                    // Try to reconnect
                    reconnect();
                }
            }
        }
    }
    
    return true;
}

bool CONTROL_WIFI::test() {
    log("Testing WiFi...");
    
    if (!wifiInitialized) {
        log("WiFi not initialized", "ERROR");
        return false;
    }
    
    // Test WiFi scan
    int networks = scanNetworks();
    log("Found " + String(networks) + " networks");
    
    if (networks > 0) {
        log("WiFi test passed");
        return true;
    }
    
    log("WiFi test failed", "ERROR");
    return false;
}

DynamicJsonDocument CONTROL_WIFI::getStatus() {
    DynamicJsonDocument doc(1024);
    
    doc["module"] = moduleName;
    doc["state"] = state == MODULE_ENABLED ? "enabled" : "disabled";
    doc["version"] = version;
    doc["priority"] = priority;
    doc["autoStart"] = autoStart;
    doc["debug"] = debugEnabled;
    
    doc["mode"] = config.mode;
    doc["ssid"] = config.ssid;
    doc["connected"] = isConnected;
    
    if (config.mode == CUSTOM_WIFI_CLIENT || config.mode == CUSTOM_WIFI_APSTA) {
        if (isConnected) {
            doc["ip"] = WiFi.localIP().toString();
            doc["rssi"] = WiFi.RSSI();
        }
    }
    
    if (config.mode == CUSTOM_WIFI_AP || config.mode == CUSTOM_WIFI_APSTA) {
        doc["ap_ip"] = config.ap_ip.toString();
        doc["clients"] = WiFi.softAPgetStationNum();
    }
    
    doc["mac"] = WiFi.macAddress();
    
    return doc;
}

bool CONTROL_WIFI::loadConfig(DynamicJsonDocument& doc) {
    Module::loadConfig(doc);
    
    if (doc.containsKey("CONTROL_WIFI")) {
        JsonObject wifiConfig = doc["CONTROL_WIFI"];
        
        if (wifiConfig.containsKey("ssid"))
            config.ssid = wifiConfig["ssid"].as<String>();
        if (wifiConfig.containsKey("password"))
            config.password = wifiConfig["password"].as<String>();
        if (wifiConfig.containsKey("mode"))
            config.mode = (CustomWiFiMode)wifiConfig["mode"].as<int>();
        
        // AP settings
        if (wifiConfig.containsKey("ap_dhcp"))
            config.ap_dhcp = wifiConfig["ap_dhcp"];
        
        // Client settings
        if (wifiConfig.containsKey("client_dhcp"))
            config.client_dhcp = wifiConfig["client_dhcp"];
    }
    
    return true;
}

bool CONTROL_WIFI::setSSID(const String& ssid) {
    config.ssid = ssid;
    log("SSID set to: " + ssid);
    return true;
}

bool CONTROL_WIFI::setPassword(const String& password) {
    config.password = password;
    log("Password updated");
    return true;
}

bool CONTROL_WIFI::setMode(CustomWiFiMode mode) {
    config.mode = mode;
    log("WiFi mode set to: " + String(mode));
    return true;
}

bool CONTROL_WIFI::startAP() {
    log("Starting Access Point...");
    
    WiFi.mode(WIFI_AP);
    
    if (!config.ap_dhcp) {
        WiFi.softAPConfig(config.ap_ip, config.ap_gateway, config.ap_subnet);
    }
    
    bool success = WiFi.softAP(
        config.ssid.c_str(), 
        config.password.c_str(), 
        config.channel, 
        config.hidden, 
        config.maxConnections
    );
    
    if (success) {
        log("AP started: " + WiFi.softAPIP().toString());
        isConnected = true;
        return true;
    }
    
    log("Failed to start AP", "ERROR");
    return false;
}

bool CONTROL_WIFI::startClient() {
    log("Starting WiFi Client...");
    
    WiFi.mode(WIFI_STA);
    
    if (!config.client_dhcp) {
        WiFi.config(
            config.client_ip,
            config.client_gateway,
            config.client_subnet,
            config.client_dns1,
            config.client_dns2
        );
    }
    
    return connectToNetwork();
}

bool CONTROL_WIFI::connectToNetwork() {
    log("Connecting to: " + config.ssid);
    
    WiFi.begin(config.ssid.c_str(), config.password.c_str());
    
    int timeout = 20; // 10 seconds
    while (WiFi.status() != WL_CONNECTED && timeout > 0) {
        delay(500);
        Serial.print(".");
        timeout--;
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        log("Connected! IP: " + WiFi.localIP().toString());
        isConnected = true;
        return true;
    }
    
    log("Connection failed", "ERROR");
    isConnected = false;
    return false;
}

bool CONTROL_WIFI::setAPConfig(IPAddress ip, IPAddress gateway, IPAddress subnet) {
    config.ap_ip = ip;
    config.ap_gateway = gateway;
    config.ap_subnet = subnet;
    config.ap_dhcp = false;
    return true;
}

bool CONTROL_WIFI::enableAPDHCP(IPAddress start, IPAddress end) {
    config.ap_dhcp = true;
    // DHCP range configuration would go here
    return true;
}

bool CONTROL_WIFI::setClientConfig(IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns1, IPAddress dns2) {
    config.client_ip = ip;
    config.client_gateway = gateway;
    config.client_subnet = subnet;
    config.client_dns1 = dns1;
    config.client_dns2 = dns2;
    config.client_dhcp = false;
    return true;
}

bool CONTROL_WIFI::enableClientDHCP(bool enable) {
    config.client_dhcp = enable;
    return true;
}

String CONTROL_WIFI::getSSID() {
    if (config.mode == CUSTOM_WIFI_CLIENT || config.mode == CUSTOM_WIFI_APSTA) {
        return WiFi.SSID();
    }
    return config.ssid;
}

String CONTROL_WIFI::getIP() {
    if (config.mode == CUSTOM_WIFI_CLIENT || config.mode == CUSTOM_WIFI_APSTA) {
        return WiFi.localIP().toString();
    } else if (config.mode == CUSTOM_WIFI_AP) {
        return WiFi.softAPIP().toString();
    }
    return "0.0.0.0";
}

int CONTROL_WIFI::getRSSI() {
    if (isConnected && (config.mode == CUSTOM_WIFI_CLIENT || config.mode == CUSTOM_WIFI_APSTA)) {
        return WiFi.RSSI();
    }
    return 0;
}

String CONTROL_WIFI::getMAC() {
    return WiFi.macAddress();
}

int CONTROL_WIFI::getConnectedClients() {
    if (config.mode == CUSTOM_WIFI_AP || config.mode == CUSTOM_WIFI_APSTA) {
        return WiFi.softAPgetStationNum();
    }
    return 0;
}

int CONTROL_WIFI::scanNetworks() {
    log("Scanning networks...");
    int n = WiFi.scanNetworks();
    log("Scan complete: " + String(n) + " networks found");
    return n;
}

String CONTROL_WIFI::getScannedSSID(int index) {
    return WiFi.SSID(index);
}

int CONTROL_WIFI::getScannedRSSI(int index) {
    return WiFi.RSSI(index);
}

bool CONTROL_WIFI::getScannedEncryption(int index) {
    return WiFi.encryptionType(index) != WIFI_AUTH_OPEN;
}

void CONTROL_WIFI::disconnect() {
    log("Disconnecting WiFi...");
    WiFi.disconnect();
    isConnected = false;
}

void CONTROL_WIFI::reconnect() {
    log("Reconnecting WiFi...");
    
    if (config.mode == CUSTOM_WIFI_CLIENT || config.mode == CUSTOM_WIFI_APSTA) {
        WiFi.disconnect();
        delay(100);
        connectToNetwork();
    }
}
