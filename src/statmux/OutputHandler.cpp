#include "OutputHandler.h"
#include "common/Logger.h"

OutputHandler::OutputHandler(const StatMuxConfig& config) : m_config(config), m_running(false) {}

OutputHandler::~OutputHandler() { stop(); }

bool OutputHandler::initialize() { Logger::info("Initializing OutputHandler"); return true; }

bool OutputHandler::start() { Logger::info("Starting OutputHandler"); m_running = true; return true; }

bool OutputHandler::stop() { Logger::info("Stopping OutputHandler"); m_running = false; return true; }

bool OutputHandler::addOutput(const std::string& outputId, const OutputConfig& config) {
    std::lock_guard<std::mutex> lock(m_outputsMutex);
    m_outputs[outputId] = config;
    Logger::info("Added output: " + outputId);
    return true;
}

bool OutputHandler::removeOutput(const std::string& outputId) {
    std::lock_guard<std::mutex> lock(m_outputsMutex);
    m_outputs.erase(outputId);
    Logger::info("Removed output: " + outputId);
    return true;
}