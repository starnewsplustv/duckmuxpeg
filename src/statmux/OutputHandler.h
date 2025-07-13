#pragma once

#include <memory>
#include <string>
#include <map>
#include <atomic>
#include <mutex>

#include "common/Config.h"

class OutputHandler {
public:
    explicit OutputHandler(const StatMuxConfig& config);
    ~OutputHandler();
    
    bool initialize();
    bool start();
    bool stop();
    
    bool addOutput(const std::string& outputId, const OutputConfig& config);
    bool removeOutput(const std::string& outputId);
    
private:
    StatMuxConfig m_config;
    std::atomic<bool> m_running;
    std::mutex m_outputsMutex;
    std::map<std::string, OutputConfig> m_outputs;
};