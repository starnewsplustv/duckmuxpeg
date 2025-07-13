#include "BitrateController.h"
#include "common/Logger.h"

BitrateController::BitrateController(const StatMuxConfig& config) : m_config(config), m_running(false) {}

BitrateController::~BitrateController() { stop(); }

bool BitrateController::initialize() { Logger::info("Initializing BitrateController"); return true; }

bool BitrateController::start() { Logger::info("Starting BitrateController"); m_running = true; return true; }

bool BitrateController::stop() { Logger::info("Stopping BitrateController"); m_running = false; return true; }

bool BitrateController::updateStreamBitrate(const std::string& streamId, int bitrate) {
    std::lock_guard<std::mutex> lock(m_bitratesMutex);
    m_streamBitrates[streamId] = bitrate;
    return true;
}

bool BitrateController::updateFrameStats(const std::string& streamId, size_t frameSize, double complexity) {
    std::lock_guard<std::mutex> lock(m_bitratesMutex);
    m_streamComplexities[streamId] = complexity;
    return true;
}

void BitrateController::updateConfig(const StatMuxConfig& config) {
    m_config = config;
}

std::map<std::string, int> BitrateController::getOptimalBitrates() const {
    std::lock_guard<std::mutex> lock(m_bitratesMutex);
    return m_streamBitrates;
}