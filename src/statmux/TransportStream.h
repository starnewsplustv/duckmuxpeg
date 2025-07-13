#pragma once

#include <string>
#include <atomic>
#include <mutex>
#include "common/Config.h"
#include "common/CircularBuffer.h"

class TransportStream {
public:
    explicit TransportStream(const StatMuxConfig& config);
    ~TransportStream();
    
    bool initialize();
    bool start();
    bool stop();
    
    bool addFrame(const VideoFrame& frame, const std::string& streamId);
    
private:
    StatMuxConfig m_config;
    std::atomic<bool> m_running;
    std::mutex m_transportMutex;
};