/**
 * @file CONTROL_SERIAL.h
 * @brief Serial console control module interface.
 * @author Michael Kojdl
 * @email michael@kojdl.com
 * @date 2025-11-16
 * @version 1.0.1
 */
#ifndef CONTROL_SERIAL_H
#define CONTROL_SERIAL_H

#include "../ModuleManager.h"

#define SERIAL_BUFFER_SIZE 256

/**
 * @class CONTROL_SERIAL
 * @brief Command-line interface over serial.
 * @details Provides interactive commands for module management, logs, and system diagnostics.
 */
/**
 * @class CONTROL_SERIAL
 * @brief Serial console/CLI for diagnostics, configuration, and control.
 */
class CONTROL_SERIAL : public Module {
private:
    char inputBuffer[SERIAL_BUFFER_SIZE];
    int bufferIndex;
    bool serialInitialized;
    
    void processCommand(const String& command);
    void printHelp();
    void printPrompt();
    
    /**
     * @brief Command handler functions implementing individual CLI commands.
     * @details These functions execute actions like listing modules, starting/stopping modules,
     * viewing logs, and retrieving system/config information.
     */
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
    void cmdHelpDetailed(const String& command);
    void cmdConfig(const String& args);
    void cmdSystem(const String& args);
    void cmdModuleCommand(const String& moduleName, const String& command, const String& args);
    void cmdRealTimeStatus();
    void cmdSafetyLimits();
    
    /**
     * @brief Safety and validation helpers for command execution.
     * @details Validate command structure and enforce safety limits to prevent harmful actions.
     */
    bool validateModuleCommand(const String& moduleName, const String& command, const String& args);
    bool checkSafetyLimits(const String& moduleName, const String& command, const String& args);
    String getCommandHelp(const String& command);
    
public:
    CONTROL_SERIAL();
    ~CONTROL_SERIAL();
    
    /** @brief Initialize the serial console module. @return True on success. */
    bool init() override;
    /** @brief Start the serial console (begin monitoring input). @return True on success. */
    bool start() override;
    /** @brief Stop the serial console. @return True on success. */
    bool stop() override;
    /** @brief Periodic update hook; processes input and task housekeeping. @return True if work done. */
    bool update() override;
    /** @brief Run self-test for the module. @return True if tests pass. */
    bool test() override;
    /** @brief Current module status as JSON. @return Status document. */
    DynamicJsonDocument getStatus() override;

    /** @brief Read and process incoming serial data into commands. */
    void processSerial();
    /** @brief Print a line to the serial console. @param message Text to print followed by newline. */
    void println(const String& message);
    /** @brief Print without newline to the serial console. @param message Text to print. */
    void print(const String& message);
};

#endif
