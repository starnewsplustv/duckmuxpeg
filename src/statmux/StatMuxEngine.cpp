#include "StatMuxEngine.h"
#include "common/Utils.h"
#include <algorithm>
#include <numeric>

StatMuxEngine::StatMuxEngine(const Config& config)
    : m_config(config)
    , m_statmuxConfig(config.getStatMuxConfig())
    , m_running(false)
    , m_initialized(false)
    , m_statisticsEnabled(true)
    , m_optimizationInterval(std::chrono::milliseconds(m_statmuxConfig.update_interval_ms))
{
    Logger::info("StatMux Engine created with " + std::to_string(m_statmuxConfig.max_concurrent_streams) + " max streams");
    
    // Initialize statistics
    m_muxStats = {};
    m_muxStats.startTime = std::chrono::high_resolution_clock::now();
    m_lastOptimization = std::chrono::high_resolution_clock::now();
}

StatMuxEngine::~StatMuxEngine() {
    stop();
    cleanup();
}

bool StatMuxEngine::initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_initialized) {
        Logger::warn("StatMux Engine already initialized");
        return true;
    }
    
    Logger::info("Initializing StatMux Engine...");
    
    // Validate configuration
    if (!validateConfiguration()) {
        Logger::error("Invalid configuration provided");
        return false;
    }
    
    // Initialize components
    try {
        m_inputHandler = std::make_unique<InputHandler>(m_statmuxConfig);
        m_outputHandler = std::make_unique<OutputHandler>(m_statmuxConfig);
        m_bitrateController = std::make_unique<BitrateController>(m_statmuxConfig);
        m_streamProcessor = std::make_unique<StreamProcessor>(m_statmuxConfig);
        m_qualityAnalyzer = std::make_unique<QualityAnalyzer>(m_statmuxConfig);
        m_transportStream = std::make_unique<TransportStream>(m_statmuxConfig);
        
        // Initialize each component
        if (!m_inputHandler->initialize() ||
            !m_outputHandler->initialize() ||
            !m_bitrateController->initialize() ||
            !m_streamProcessor->initialize() ||
            !m_qualityAnalyzer->initialize() ||
            !m_transportStream->initialize()) {
            Logger::error("Failed to initialize StatMux components");
            return false;
        }
        
        // Allocate resources
        if (!allocateResources()) {
            Logger::error("Failed to allocate resources");
            return false;
        }
        
        m_initialized = true;
        Logger::info("StatMux Engine initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Exception during initialization: " + std::string(e.what()));
        return false;
    }
}

bool StatMuxEngine::start() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        Logger::error("StatMux Engine not initialized");
        return false;
    }
    
    if (m_running) {
        Logger::warn("StatMux Engine already running");
        return true;
    }
    
    Logger::info("Starting StatMux Engine...");
    
    try {
        // Start components
        if (!m_inputHandler->start() ||
            !m_outputHandler->start() ||
            !m_bitrateController->start() ||
            !m_streamProcessor->start() ||
            !m_qualityAnalyzer->start() ||
            !m_transportStream->start()) {
            Logger::error("Failed to start StatMux components");
            return false;
        }
        
        // Start main processing thread
        m_running = true;
        m_mainThread = std::thread(&StatMuxEngine::mainLoop, this);
        
        // Start statistics thread if enabled
        if (m_statisticsEnabled) {
            m_statisticsThread = std::thread(&StatMuxEngine::statisticsLoop, this);
        }
        
        // Initialize input streams from configuration
        for (const auto& input : m_config.getInputs()) {
            if (input.enabled) {
                addInputStream(input);
            }
        }
        
        // Initialize output streams from configuration
        for (const auto& output : m_config.getOutputs()) {
            addOutputStream(output);
        }
        
        Logger::info("StatMux Engine started successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Exception during startup: " + std::string(e.what()));
        m_running = false;
        return false;
    }
}

bool StatMuxEngine::stop() {
    Logger::info("Stopping StatMux Engine...");
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_running = false;
    }
    
    m_condition.notify_all();
    
    // Wait for main thread to finish
    if (m_mainThread.joinable()) {
        m_mainThread.join();
    }
    
    // Wait for statistics thread to finish
    if (m_statisticsThread.joinable()) {
        m_statisticsThread.join();
    }
    
    // Stop components
    if (m_transportStream) m_transportStream->stop();
    if (m_qualityAnalyzer) m_qualityAnalyzer->stop();
    if (m_streamProcessor) m_streamProcessor->stop();
    if (m_bitrateController) m_bitrateController->stop();
    if (m_outputHandler) m_outputHandler->stop();
    if (m_inputHandler) m_inputHandler->stop();
    
    // Clear contexts
    {
        std::lock_guard<std::mutex> lock(m_streamMutex);
        m_streamContexts.clear();
    }
    
    {
        std::lock_guard<std::mutex> lock(m_outputMutex);
        m_outputContexts.clear();
    }
    
    Logger::info("StatMux Engine stopped");
    return true;
}

bool StatMuxEngine::isRunning() const {
    return m_running.load();
}

bool StatMuxEngine::addInputStream(const InputConfig& config) {
    std::lock_guard<std::mutex> lock(m_streamMutex);
    
    if (m_streamContexts.size() >= static_cast<size_t>(m_statmuxConfig.max_concurrent_streams)) {
        Logger::error("Maximum number of concurrent streams reached");
        return false;
    }
    
    // Create stream context
    auto context = std::make_unique<StreamContext>();
    context->id = config.url + "_" + std::to_string(m_streamContexts.size());
    context->inputConfig = config;
    context->frameBuffer = std::make_unique<VideoFrameBuffer>(m_statmuxConfig.buffer_size_ms / 33); // ~30fps
    context->audioBuffer = std::make_unique<AudioFrameBuffer>(m_statmuxConfig.buffer_size_ms / 21); // ~48kHz
    context->active = true;
    context->paused = false;
    context->currentBitrate = config.bitrate;
    context->quality = 1.0;
    context->lastFrameTime = std::chrono::high_resolution_clock::now();
    context->frameCount = 0;
    context->droppedFrames = 0;
    
    // Add to input handler
    if (!m_inputHandler->addInput(context->id, config)) {
        Logger::error("Failed to add input stream: " + context->id);
        return false;
    }
    
    m_streamContexts.push_back(std::move(context));
    
    Logger::info("Added input stream: " + m_streamContexts.back()->id);
    handleStreamEvent("stream_added", m_streamContexts.back()->id);
    
    return true;
}

bool StatMuxEngine::removeInputStream(const std::string& streamId) {
    std::lock_guard<std::mutex> lock(m_streamMutex);
    
    auto it = std::find_if(m_streamContexts.begin(), m_streamContexts.end(),
                          [&streamId](const auto& ctx) { return ctx->id == streamId; });
    
    if (it != m_streamContexts.end()) {
        (*it)->active = false;
        m_inputHandler->removeInput(streamId);
        m_streamContexts.erase(it);
        
        Logger::info("Removed input stream: " + streamId);
        handleStreamEvent("stream_removed", streamId);
        return true;
    }
    
    Logger::warn("Stream not found: " + streamId);
    return false;
}

bool StatMuxEngine::addOutputStream(const OutputConfig& config) {
    std::lock_guard<std::mutex> lock(m_outputMutex);
    
    auto context = std::make_unique<OutputContext>();
    context->id = config.url + "_" + std::to_string(m_outputContexts.size());
    context->config = config;
    context->active = true;
    
    // Add to output handler
    if (!m_outputHandler->addOutput(context->id, config)) {
        Logger::error("Failed to add output stream: " + context->id);
        return false;
    }
    
    m_outputContexts.push_back(std::move(context));
    
    Logger::info("Added output stream: " + m_outputContexts.back()->id);
    return true;
}

bool StatMuxEngine::removeOutputStream(const std::string& streamId) {
    std::lock_guard<std::mutex> lock(m_outputMutex);
    
    auto it = std::find_if(m_outputContexts.begin(), m_outputContexts.end(),
                          [&streamId](const auto& ctx) { return ctx->id == streamId; });
    
    if (it != m_outputContexts.end()) {
        (*it)->active = false;
        m_outputHandler->removeOutput(streamId);
        m_outputContexts.erase(it);
        
        Logger::info("Removed output stream: " + streamId);
        return true;
    }
    
    Logger::warn("Output stream not found: " + streamId);
    return false;
}

bool StatMuxEngine::updateBitrate(const std::string& streamId, int newBitrate) {
    std::lock_guard<std::mutex> lock(m_streamMutex);
    
    auto it = std::find_if(m_streamContexts.begin(), m_streamContexts.end(),
                          [&streamId](const auto& ctx) { return ctx->id == streamId; });
    
    if (it != m_streamContexts.end()) {
        (*it)->currentBitrate = newBitrate;
        m_bitrateController->updateStreamBitrate(streamId, newBitrate);
        
        Logger::info("Updated bitrate for stream " + streamId + " to " + std::to_string(newBitrate) + " kbps");
        return true;
    }
    
    return false;
}

bool StatMuxEngine::pauseStream(const std::string& streamId) {
    std::lock_guard<std::mutex> lock(m_streamMutex);
    
    auto it = std::find_if(m_streamContexts.begin(), m_streamContexts.end(),
                          [&streamId](const auto& ctx) { return ctx->id == streamId; });
    
    if (it != m_streamContexts.end()) {
        (*it)->paused = true;
        Logger::info("Paused stream: " + streamId);
        handleStreamEvent("stream_paused", streamId);
        return true;
    }
    
    return false;
}

bool StatMuxEngine::resumeStream(const std::string& streamId) {
    std::lock_guard<std::mutex> lock(m_streamMutex);
    
    auto it = std::find_if(m_streamContexts.begin(), m_streamContexts.end(),
                          [&streamId](const auto& ctx) { return ctx->id == streamId; });
    
    if (it != m_streamContexts.end()) {
        (*it)->paused = false;
        Logger::info("Resumed stream: " + streamId);
        handleStreamEvent("stream_resumed", streamId);
        return true;
    }
    
    return false;
}

bool StatMuxEngine::adjustQuality(const std::string& streamId, double qualityFactor) {
    std::lock_guard<std::mutex> lock(m_streamMutex);
    
    auto it = std::find_if(m_streamContexts.begin(), m_streamContexts.end(),
                          [&streamId](const auto& ctx) { return ctx->id == streamId; });
    
    if (it != m_streamContexts.end()) {
        (*it)->quality = Utils::clamp(qualityFactor, 0.1, 2.0);
        Logger::info("Adjusted quality for stream " + streamId + " to " + std::to_string((*it)->quality));
        return true;
    }
    
    return false;
}

std::vector<StreamStats> StatMuxEngine::getStreamStats() const {
    std::lock_guard<std::mutex> lock(m_statsLock);
    return m_streamStats;
}

MuxStats StatMuxEngine::getMuxStats() const {
    std::lock_guard<std::mutex> lock(m_statsLock);
    MuxStats stats = m_muxStats;
    stats.uptime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - stats.startTime);
    return stats;
}

bool StatMuxEngine::enableStatistics(bool enable) {
    m_statisticsEnabled = enable;
    return true;
}

bool StatMuxEngine::updateConfig(const StatMuxConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_statmuxConfig = config;
    m_optimizationInterval = std::chrono::milliseconds(config.update_interval_ms);
    
    // Update components with new configuration
    if (m_bitrateController) m_bitrateController->updateConfig(config);
    if (m_streamProcessor) m_streamProcessor->updateConfig(config);
    if (m_qualityAnalyzer) m_qualityAnalyzer->updateConfig(config);
    
    Logger::info("Updated StatMux configuration");
    return true;
}

void StatMuxEngine::setOBSIntegration(bool enabled) {
    // This will be implemented with the OBS plugin
    Logger::info("OBS integration " + std::string(enabled ? "enabled" : "disabled"));
}

void StatMuxEngine::setEventCallback(EventCallback callback) {
    m_eventCallback = callback;
}

bool StatMuxEngine::enableAdaptiveGOP(bool enable) {
    m_statmuxConfig.adaptive_gop = enable;
    if (m_streamProcessor) {
        m_streamProcessor->setAdaptiveGOP(enable);
    }
    return true;
}

bool StatMuxEngine::enableSceneChangeDetection(bool enable) {
    m_statmuxConfig.scene_change_detection = enable;
    if (m_qualityAnalyzer) {
        m_qualityAnalyzer->setSceneChangeDetection(enable);
    }
    return true;
}

bool StatMuxEngine::setMaxConcurrentStreams(int maxStreams) {
    m_statmuxConfig.max_concurrent_streams = maxStreams;
    return true;
}

// Private methods

void StatMuxEngine::mainLoop() {
    Logger::info("StatMux main loop started");
    
    while (m_running) {
        try {
            auto now = std::chrono::high_resolution_clock::now();
            
            // Process input frames
            {
                std::lock_guard<std::mutex> lock(m_streamMutex);
                for (auto& context : m_streamContexts) {
                    if (!context->active || context->paused) continue;
                    
                    VideoFrame frame;
                    if (context->frameBuffer->readWithTimeout(frame, std::chrono::milliseconds(1))) {
                        processFrame(frame, context->id);
                        context->frameCount++;
                        context->lastFrameTime = now;
                    }
                }
            }
            
            // Optimize bitrates periodically
            if (now - m_lastOptimization >= m_optimizationInterval) {
                optimizeBitrates();
                m_lastOptimization = now;
            }
            
            // Small delay to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            
        } catch (const std::exception& e) {
            Logger::error("Exception in main loop: " + std::string(e.what()));
        }
    }
    
    Logger::info("StatMux main loop stopped");
}

void StatMuxEngine::statisticsLoop() {
    Logger::info("StatMux statistics loop started");
    
    while (m_running) {
        try {
            if (m_statisticsEnabled) {
                updateStreamStatistics();
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Update every second
            
        } catch (const std::exception& e) {
            Logger::error("Exception in statistics loop: " + std::string(e.what()));
        }
    }
    
    Logger::info("StatMux statistics loop stopped");
}

void StatMuxEngine::processFrame(const VideoFrame& frame, const std::string& streamId) {
    // Analyze frame quality
    double complexity = m_qualityAnalyzer->analyzeFrame(frame);
    
    // Process frame through stream processor
    m_streamProcessor->processFrame(frame, streamId);
    
    // Update bitrate controller
    m_bitrateController->updateFrameStats(streamId, frame.data.size(), complexity);
    
    // Forward to transport stream
    m_transportStream->addFrame(frame, streamId);
}

void StatMuxEngine::updateStreamStatistics() {
    std::lock_guard<std::mutex> lock(m_statsLock);
    
    m_streamStats.clear();
    
    size_t totalFrames = 0;
    size_t totalDropped = 0;
    int totalBitrate = 0;
    
    {
        std::lock_guard<std::mutex> streamLock(m_streamMutex);
        for (const auto& context : m_streamContexts) {
            if (!context->active) continue;
            
            StreamStats stats;
            stats.streamId = context->id;
            stats.currentBitrate = context->currentBitrate;
            stats.targetBitrate = context->inputConfig.bitrate;
            stats.minBitrate = static_cast<int>(context->inputConfig.bitrate * m_statmuxConfig.min_bitrate_factor);
            stats.maxBitrate = static_cast<int>(context->inputConfig.bitrate * m_statmuxConfig.max_bitrate_factor);
            stats.quality = context->quality;
            stats.complexity = m_qualityAnalyzer->getStreamComplexity(context->id);
            stats.frameCount = context->frameCount;
            stats.droppedFrames = context->droppedFrames;
            
            auto timeDiff = std::chrono::high_resolution_clock::now() - context->lastFrameTime;
            stats.averageFrameTime = std::chrono::duration<double, std::milli>(timeDiff).count();
            stats.lastUpdate = std::chrono::high_resolution_clock::now();
            
            m_streamStats.push_back(stats);
            
            totalFrames += stats.frameCount;
            totalDropped += stats.droppedFrames;
            totalBitrate += stats.currentBitrate;
        }
    }
    
    // Update mux statistics
    m_muxStats.totalInputStreams = m_streamContexts.size();
    m_muxStats.activeStreams = std::count_if(m_streamContexts.begin(), m_streamContexts.end(),
                                            [](const auto& ctx) { return ctx->active && !ctx->paused; });
    m_muxStats.totalBitrate = totalBitrate;
    m_muxStats.totalFrames = totalFrames;
    m_muxStats.droppedFrames = totalDropped;
    m_muxStats.muxEfficiency = (totalFrames > 0) ? 
        static_cast<double>(totalFrames - totalDropped) / totalFrames : 0.0;
}

void StatMuxEngine::handleStreamEvent(const std::string& event, const std::string& streamId) {
    if (m_eventCallback) {
        m_eventCallback(event, streamId);
    }
    
    // Queue event for processing
    std::lock_guard<std::mutex> lock(m_eventMutex);
    m_eventQueue.push(std::make_pair(event, streamId));
}

void StatMuxEngine::optimizeBitrates() {
    std::lock_guard<std::mutex> lock(m_streamMutex);
    
    if (m_streamContexts.empty()) return;
    
    // Calculate total available bitrate
    int totalTargetBitrate = 0;
    for (const auto& context : m_streamContexts) {
        if (context->active && !context->paused) {
            totalTargetBitrate += context->inputConfig.bitrate;
        }
    }
    
    // Get current bitrate requirements from bitrate controller
    auto bitrateRequirements = m_bitrateController->getOptimalBitrates();
    
    // Redistribute bitrates based on complexity and quality requirements
    for (auto& context : m_streamContexts) {
        if (!context->active || context->paused) continue;
        
        auto it = bitrateRequirements.find(context->id);
        if (it != bitrateRequirements.end()) {
            int newBitrate = it->second;
            
            // Apply quality factor
            newBitrate = static_cast<int>(newBitrate * context->quality);
            
            // Clamp to configured limits
            int minBitrate = static_cast<int>(context->inputConfig.bitrate * m_statmuxConfig.min_bitrate_factor);
            int maxBitrate = static_cast<int>(context->inputConfig.bitrate * m_statmuxConfig.max_bitrate_factor);
            newBitrate = Utils::clamp(newBitrate, minBitrate, maxBitrate);
            
            if (newBitrate != context->currentBitrate) {
                context->currentBitrate = newBitrate;
                Logger::debug("Optimized bitrate for stream " + context->id + " to " + std::to_string(newBitrate) + " kbps");
            }
        }
    }
}

bool StatMuxEngine::validateConfiguration() const {
    if (m_statmuxConfig.max_concurrent_streams <= 0) {
        Logger::error("Invalid max_concurrent_streams value");
        return false;
    }
    
    if (m_statmuxConfig.buffer_size_ms <= 0) {
        Logger::error("Invalid buffer_size_ms value");
        return false;
    }
    
    if (m_statmuxConfig.update_interval_ms <= 0) {
        Logger::error("Invalid update_interval_ms value");
        return false;
    }
    
    if (m_statmuxConfig.min_bitrate_factor <= 0 || m_statmuxConfig.min_bitrate_factor > 1.0) {
        Logger::error("Invalid min_bitrate_factor value");
        return false;
    }
    
    if (m_statmuxConfig.max_bitrate_factor <= 1.0) {
        Logger::error("Invalid max_bitrate_factor value");
        return false;
    }
    
    return true;
}

bool StatMuxEngine::allocateResources() {
    // Allocate memory pools and resources
    Logger::info("Allocating resources for " + std::to_string(m_statmuxConfig.max_concurrent_streams) + " streams");
    return true;
}

void StatMuxEngine::releaseResources() {
    Logger::info("Releasing StatMux resources");
}

void StatMuxEngine::cleanup() {
    releaseResources();
    
    // Clear all contexts
    {
        std::lock_guard<std::mutex> lock(m_streamMutex);
        m_streamContexts.clear();
    }
    
    {
        std::lock_guard<std::mutex> lock(m_outputMutex);
        m_outputContexts.clear();
    }
    
    Logger::info("StatMux cleanup completed");
}