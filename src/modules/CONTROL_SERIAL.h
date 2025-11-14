#ifndef CONTROL_SERIAL_H
#define CONTROL_SERIAL_H

#include "../ModuleManager.h"

#define SERIAL_BUFFER_SIZE 256

class CONTROL_SERIAL : public Module {
private:
    char inputBuffer[SERIAL_BUFFER_SIZE];
    int bufferIndex;
    bool serialInitialized;
    
    void processCommand(const String& command);
    void printHelp();
    void printPrompt();
    
    // Command handlers
    void cmdStatus();
    void cmdModules();
    void cmdModuleInfo(const String& moduleName);
    void cmdModuleStart(const String& moduleName);
    void cmdModuleStop(const String& moduleName);
    void cmdModuleTest(const String& moduleName);
    void cmdModuleConfig(const String& moduleName);
    void cmdLogs(int lines);
    void cmdRestart();
    void cmdClearLogs();
    void cmdHelp();
    
public:
    CONTROL_SERIAL();
    ~CONTROL_SERIAL();
    
    // Module interface
    bool init() override;
    bool start() override;
    bool stop() override;
    bool update() override;
    bool test() override;
    DynamicJsonDocument getStatus() override;
    
    // Serial control
    void processSerial();
    void println(const String& message);
    void print(const String& message);
};

#endif
