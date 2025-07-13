#pragma once

#include <string>
#include <map>
#include <atomic>
#include <mutex>
#include "common/Config.h"
#include "common/CircularBuffer.h"

class QualityAnalyzer {
public:
    explicit QualityAnalyzer(const StatMuxConfig& config);
    ~QualityAnalyzer();
    
    bool initialize();
    bool start();
    bool stop();
    
    double analyzeFrame(const VideoFrame& frame);
    double getStreamComplexity(const std::string& streamId) const;
    void updateConfig(const StatMuxConfig& config);
    void setSceneChangeDetection(bool enabled);
    
private:
    StatMuxConfig m_config;
    std::atomic<bool> m_running;
    std::atomic<bool> m_sceneChangeDetection;
    mutable std::mutex m_analysisMutex;
    std::map<std::string, double> m_streamComplexities;
};