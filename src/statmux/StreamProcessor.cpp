#include "StreamProcessor.h"
#include "common/Logger.h"

StreamProcessor::StreamProcessor(const StatMuxConfig& config) : m_config(config), m_running(false), m_adaptiveGOP(false) {}

StreamProcessor::~StreamProcessor() { stop(); }

bool StreamProcessor::initialize() { Logger::info("Initializing StreamProcessor"); return true; }

bool StreamProcessor::start() { Logger::info("Starting StreamProcessor"); m_running = true; return true; }

bool StreamProcessor::stop() { Logger::info("Stopping StreamProcessor"); m_running = false; return true; }

bool StreamProcessor::processFrame(const VideoFrame& frame, const std::string& streamId) {
    std::lock_guard<std::mutex> lock(m_processingMutex);
    // Process frame here
    return true;
}

void StreamProcessor::updateConfig(const StatMuxConfig& config) {
    m_config = config;
}

void StreamProcessor::setAdaptiveGOP(bool enabled) {
    m_adaptiveGOP = enabled;
}