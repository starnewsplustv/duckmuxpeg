#include "QualityAnalyzer.h"
#include "common/Logger.h"

QualityAnalyzer::QualityAnalyzer(const StatMuxConfig& config) : m_config(config), m_running(false), m_sceneChangeDetection(false) {}

QualityAnalyzer::~QualityAnalyzer() { stop(); }

bool QualityAnalyzer::initialize() { Logger::info("Initializing QualityAnalyzer"); return true; }

bool QualityAnalyzer::start() { Logger::info("Starting QualityAnalyzer"); m_running = true; return true; }

bool QualityAnalyzer::stop() { Logger::info("Stopping QualityAnalyzer"); m_running = false; return true; }

double QualityAnalyzer::analyzeFrame(const VideoFrame& frame) {
    std::lock_guard<std::mutex> lock(m_analysisMutex);
    // Analyze frame complexity - simplified implementation
    return 0.5; // Return complexity between 0.0 and 1.0
}

double QualityAnalyzer::getStreamComplexity(const std::string& streamId) const {
    std::lock_guard<std::mutex> lock(m_analysisMutex);
    auto it = m_streamComplexities.find(streamId);
    return (it != m_streamComplexities.end()) ? it->second : 0.5;
}

void QualityAnalyzer::updateConfig(const StatMuxConfig& config) {
    m_config = config;
}

void QualityAnalyzer::setSceneChangeDetection(bool enabled) {
    m_sceneChangeDetection = enabled;
}