#pragma once

#include <string>
#include <map>
#include <atomic>
#include <mutex>

#include "common/Config.h"

class BitrateController {
public:
    explicit BitrateController(const StatMuxConfig& config);
    ~BitrateController();
    
    bool initialize();
    bool start();
    bool stop();
    
    bool updateStreamBitrate(const std::string& streamId, int bitrate);
    bool updateFrameStats(const std::string& streamId, size_t frameSize, double complexity);
    void updateConfig(const StatMuxConfig& config);
    
    std::map<std::string, int> getOptimalBitrates() const;
    
private:
    StatMuxConfig m_config;
    std::atomic<bool> m_running;
    mutable std::mutex m_bitratesMutex;
    std::map<std::string, int> m_streamBitrates;
    std::map<std::string, double> m_streamComplexities;
};