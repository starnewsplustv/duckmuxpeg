#pragma once

#include <memory>
#include <string>
#include <map>
#include <thread>
#include <atomic>
#include <mutex>

#include "common/Config.h"
#include "common/CircularBuffer.h"

class InputHandler {
public:
    explicit InputHandler(const StatMuxConfig& config);
    ~InputHandler();
    
    bool initialize();
    bool start();
    bool stop();
    
    bool addInput(const std::string& inputId, const InputConfig& config);
    bool removeInput(const std::string& inputId);
    
    bool pauseInput(const std::string& inputId);
    bool resumeInput(const std::string& inputId);
    
    std::shared_ptr<VideoFrameBuffer> getVideoBuffer(const std::string& inputId);
    std::shared_ptr<AudioFrameBuffer> getAudioBuffer(const std::string& inputId);
    
private:
    struct InputContext {
        std::string id;
        InputConfig config;
        std::shared_ptr<VideoFrameBuffer> videoBuffer;
        std::shared_ptr<AudioFrameBuffer> audioBuffer;
        std::unique_ptr<std::thread> processingThread;
        std::atomic<bool> active;
        std::atomic<bool> paused;
    };
    
    StatMuxConfig m_config;
    std::map<std::string, std::unique_ptr<InputContext>> m_inputs;
    std::mutex m_inputsMutex;
    std::atomic<bool> m_running;
    
    void processInput(const std::string& inputId);
    bool initializeInput(InputContext& context);
    void cleanupInput(InputContext& context);
};