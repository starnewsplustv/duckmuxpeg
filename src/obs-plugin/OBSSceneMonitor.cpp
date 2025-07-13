#include "OBSSceneMonitor.h"
#include "common/Logger.h"

OBSSceneMonitor::OBSSceneMonitor() : m_monitoring(false) {}

OBSSceneMonitor::~OBSSceneMonitor() { stop(); }

bool OBSSceneMonitor::initialize(const std::vector<std::string>& sceneNames) {
    std::lock_guard<std::mutex> lock(m_sceneMutex);
    m_sceneNames = sceneNames;
    if (!m_sceneNames.empty()) {
        m_currentScene = m_sceneNames[0];
    }
    Logger::info("OBS Scene Monitor initialized with " + std::to_string(sceneNames.size()) + " scenes");
    return true;
}

bool OBSSceneMonitor::start() {
    m_monitoring = true;
    Logger::info("OBS Scene Monitor started");
    return true;
}

bool OBSSceneMonitor::stop() {
    m_monitoring = false;
    Logger::info("OBS Scene Monitor stopped");
    return true;
}

void OBSSceneMonitor::setSceneChangeCallback(SceneChangeCallback callback) {
    m_sceneChangeCallback = callback;
}

std::string OBSSceneMonitor::getCurrentScene() const {
    std::lock_guard<std::mutex> lock(m_sceneMutex);
    return m_currentScene;
}

std::vector<std::string> OBSSceneMonitor::getAvailableScenes() const {
    std::lock_guard<std::mutex> lock(m_sceneMutex);
    return m_sceneNames;
}