#pragma once

#include <string>
#include <atomic>
#include <mutex>
#include "common/Config.h"
#include "common/CircularBuffer.h"

class StreamProcessor {
public:
    explicit StreamProcessor(const StatMuxConfig& config);
    ~StreamProcessor();
    
    bool initialize();
    bool start();
    bool stop();
    
    bool processFrame(const VideoFrame& frame, const std::string& streamId);
    void updateConfig(const StatMuxConfig& config);
    void setAdaptiveGOP(bool enabled);
    
private:
    StatMuxConfig m_config;
    std::atomic<bool> m_running;
    std::atomic<bool> m_adaptiveGOP;
    std::mutex m_processingMutex;
};