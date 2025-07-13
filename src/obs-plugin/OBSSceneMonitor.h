#pragma once

#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <functional>

class OBSSceneMonitor {
public:
    OBSSceneMonitor();
    ~OBSSceneMonitor();
    
    bool initialize(const std::vector<std::string>& sceneNames);
    bool start();
    bool stop();
    
    typedef std::function<void(const std::string&)> SceneChangeCallback;
    void setSceneChangeCallback(SceneChangeCallback callback);
    
    std::string getCurrentScene() const;
    std::vector<std::string> getAvailableScenes() const;
    
private:
    std::vector<std::string> m_sceneNames;
    std::string m_currentScene;
    std::atomic<bool> m_monitoring;
    std::mutex m_sceneMutex;
    SceneChangeCallback m_sceneChangeCallback;
};