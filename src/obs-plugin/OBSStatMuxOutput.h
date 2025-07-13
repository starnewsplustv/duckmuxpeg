#pragma once

#include <memory>
#include <string>
#include <atomic>
#include <mutex>
#include "statmux/StatMuxEngine.h"
#include "common/CircularBuffer.h"

class OBSStatMuxOutput {
public:
    OBSStatMuxOutput(std::shared_ptr<StatMuxEngine> engine);
    ~OBSStatMuxOutput();
    
    bool initialize(const std::string& outputUrl);
    bool start();
    bool stop();
    
    bool processVideoFrame(const VideoFrame& frame);
    bool processAudioFrame(const AudioFrame& frame);
    
    bool isActive() const;
    std::string getStatus() const;
    
private:
    std::shared_ptr<StatMuxEngine> m_statmuxEngine;
    std::string m_outputUrl;
    std::atomic<bool> m_active;
    std::mutex m_outputMutex;
    
    // Output buffers
    std::unique_ptr<VideoFrameBuffer> m_videoOutputBuffer;
    std::unique_ptr<AudioFrameBuffer> m_audioOutputBuffer;
    
    void outputLoop();
    std::thread m_outputThread;
};