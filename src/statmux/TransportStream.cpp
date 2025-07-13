#include "TransportStream.h"
#include "common/Logger.h"

TransportStream::TransportStream(const StatMuxConfig& config) : m_config(config), m_running(false) {}

TransportStream::~TransportStream() { stop(); }

bool TransportStream::initialize() { Logger::info("Initializing TransportStream"); return true; }

bool TransportStream::start() { Logger::info("Starting TransportStream"); m_running = true; return true; }

bool TransportStream::stop() { Logger::info("Stopping TransportStream"); m_running = false; return true; }

bool TransportStream::addFrame(const VideoFrame& frame, const std::string& streamId) {
    std::lock_guard<std::mutex> lock(m_transportMutex);
    // Add frame to transport stream
    return true;
}