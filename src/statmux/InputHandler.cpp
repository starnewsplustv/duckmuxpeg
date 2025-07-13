#include "InputHandler.h"
#include "common/Logger.h"
#include "common/Utils.h"

InputHandler::InputHandler(const StatMuxConfig& config)
    : m_config(config), m_running(false) {
}

InputHandler::~InputHandler() {
    stop();
}

bool InputHandler::initialize() {
    Logger::info("Initializing InputHandler");
    return true;
}

bool InputHandler::start() {
    Logger::info("Starting InputHandler");
    m_running = true;
    return true;
}

bool InputHandler::stop() {
    Logger::info("Stopping InputHandler");
    m_running = false;
    
    std::lock_guard<std::mutex> lock(m_inputsMutex);
    for (auto& [id, context] : m_inputs) {
        context->active = false;
        if (context->processingThread && context->processingThread->joinable()) {
            context->processingThread->join();
        }
    }
    m_inputs.clear();
    
    return true;
}

bool InputHandler::addInput(const std::string& inputId, const InputConfig& config) {
    std::lock_guard<std::mutex> lock(m_inputsMutex);
    
    if (m_inputs.find(inputId) != m_inputs.end()) {
        Logger::warn("Input already exists: " + inputId);
        return false;
    }
    
    auto context = std::make_unique<InputContext>();
    context->id = inputId;
    context->config = config;
    context->videoBuffer = std::make_shared<VideoFrameBuffer>(m_config.buffer_size_ms / 33);
    context->audioBuffer = std::make_shared<AudioFrameBuffer>(m_config.buffer_size_ms / 21);
    context->active = true;
    context->paused = false;
    
    if (!initializeInput(*context)) {
        Logger::error("Failed to initialize input: " + inputId);
        return false;
    }
    
    // Start processing thread
    context->processingThread = std::make_unique<std::thread>(&InputHandler::processInput, this, inputId);
    
    m_inputs[inputId] = std::move(context);
    Logger::info("Added input: " + inputId);
    return true;
}

bool InputHandler::removeInput(const std::string& inputId) {
    std::lock_guard<std::mutex> lock(m_inputsMutex);
    
    auto it = m_inputs.find(inputId);
    if (it == m_inputs.end()) {
        Logger::warn("Input not found: " + inputId);
        return false;
    }
    
    it->second->active = false;
    if (it->second->processingThread && it->second->processingThread->joinable()) {
        it->second->processingThread->join();
    }
    
    cleanupInput(*it->second);
    m_inputs.erase(it);
    
    Logger::info("Removed input: " + inputId);
    return true;
}

bool InputHandler::pauseInput(const std::string& inputId) {
    std::lock_guard<std::mutex> lock(m_inputsMutex);
    
    auto it = m_inputs.find(inputId);
    if (it != m_inputs.end()) {
        it->second->paused = true;
        Logger::info("Paused input: " + inputId);
        return true;
    }
    
    return false;
}

bool InputHandler::resumeInput(const std::string& inputId) {
    std::lock_guard<std::mutex> lock(m_inputsMutex);
    
    auto it = m_inputs.find(inputId);
    if (it != m_inputs.end()) {
        it->second->paused = false;
        Logger::info("Resumed input: " + inputId);
        return true;
    }
    
    return false;
}

std::shared_ptr<VideoFrameBuffer> InputHandler::getVideoBuffer(const std::string& inputId) {
    std::lock_guard<std::mutex> lock(m_inputsMutex);
    
    auto it = m_inputs.find(inputId);
    if (it != m_inputs.end()) {
        return it->second->videoBuffer;
    }
    
    return nullptr;
}

std::shared_ptr<AudioFrameBuffer> InputHandler::getAudioBuffer(const std::string& inputId) {
    std::lock_guard<std::mutex> lock(m_inputsMutex);
    
    auto it = m_inputs.find(inputId);
    if (it != m_inputs.end()) {
        return it->second->audioBuffer;
    }
    
    return nullptr;
}

void InputHandler::processInput(const std::string& inputId) {
    Logger::info("Processing input: " + inputId);
    
    std::unique_ptr<InputContext>* contextPtr;
    {
        std::lock_guard<std::mutex> lock(m_inputsMutex);
        auto it = m_inputs.find(inputId);
        if (it == m_inputs.end()) {
            Logger::error("Input context not found: " + inputId);
            return;
        }
        contextPtr = &it->second;
    }
    
    InputContext& context = **contextPtr;
    
    // Simulate frame processing
    uint64_t frameNumber = 0;
    
    while (context.active && m_running) {
        if (context.paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        // Create dummy video frame
        VideoFrame videoFrame(context.config.width, context.config.height, 
                             context.config.width * 3); // RGB24
        videoFrame.frameNumber = frameNumber++;
        videoFrame.isKeyFrame = (frameNumber % 30 == 0); // Key frame every 30 frames
        
        // Write to buffer
        if (!context.videoBuffer->write(std::move(videoFrame))) {
            Logger::warn("Video buffer full for input: " + inputId);
        }
        
        // Create dummy audio frame
        AudioFrame audioFrame(48000, 2, 1024); // 48kHz, stereo, 1024 samples
        audioFrame.frameNumber = frameNumber;
        
        if (!context.audioBuffer->write(std::move(audioFrame))) {
            Logger::warn("Audio buffer full for input: " + inputId);
        }
        
        // Simulate frame rate
        std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 fps
    }
    
    Logger::info("Input processing stopped: " + inputId);
}

bool InputHandler::initializeInput(InputContext& context) {
    Logger::info("Initializing input: " + context.id);
    
    // Initialize based on input type
    if (context.config.type == "file") {
        // File input initialization
        Logger::info("Initializing file input: " + context.config.url);
    } else if (context.config.type == "udp") {
        // UDP input initialization
        Logger::info("Initializing UDP input: " + context.config.url);
    } else if (context.config.type == "rtmp") {
        // RTMP input initialization
        Logger::info("Initializing RTMP input: " + context.config.url);
    } else if (context.config.type == "obs") {
        // OBS input initialization
        Logger::info("Initializing OBS input: " + context.config.url);
    } else {
        Logger::error("Unknown input type: " + context.config.type);
        return false;
    }
    
    return true;
}

void InputHandler::cleanupInput(InputContext& context) {
    Logger::info("Cleaning up input: " + context.id);
    
    // Cleanup based on input type
    if (context.config.type == "file") {
        // File input cleanup
    } else if (context.config.type == "udp") {
        // UDP input cleanup
    } else if (context.config.type == "rtmp") {
        // RTMP input cleanup
    } else if (context.config.type == "obs") {
        // OBS input cleanup
    }
}