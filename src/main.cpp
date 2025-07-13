#include <iostream>
#include <memory>
#include <signal.h>
#include <thread>
#include <chrono>

#include "statmux/StatMuxEngine.h"
#include "common/Logger.h"
#include "common/Config.h"

volatile bool g_running = true;

void signal_handler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down gracefully..." << std::endl;
    g_running = false;
}

int main(int argc, char* argv[]) {
    // Set up signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    try {
        // Initialize logger
        Logger::initialize("duckmuxpeg");
        Logger::info("Starting DuckMuxPeg Statistical Multiplexing Engine");
        
        // Load configuration
        Config config;
        if (argc > 1) {
            config.loadFromFile(argv[1]);
        } else {
            config.loadDefault();
        }
        
        // Create and initialize the statistical multiplexing engine
        auto statmux = std::make_unique<StatMuxEngine>(config);
        
        if (!statmux->initialize()) {
            Logger::error("Failed to initialize StatMux engine");
            return 1;
        }
        
        // Start the engine
        if (!statmux->start()) {
            Logger::error("Failed to start StatMux engine");
            return 1;
        }
        
        Logger::info("StatMux engine started successfully");
        
        // Main loop
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Check engine status
            if (!statmux->isRunning()) {
                Logger::warn("StatMux engine stopped unexpectedly");
                break;
            }
        }
        
        // Shutdown
        Logger::info("Shutting down StatMux engine");
        statmux->stop();
        
        Logger::info("DuckMuxPeg shutdown complete");
        
    } catch (const std::exception& e) {
        Logger::error("Fatal error: " + std::string(e.what()));
        return 1;
    }
    
    return 0;
}