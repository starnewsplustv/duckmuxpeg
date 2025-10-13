#include "PlatformManager.h"

#include <numeric>

#include "common/Logger.h"

PlatformManager::PlatformManager(const MultiPlatformConfig& config)
    : m_config(config) {
    rebuildPlatforms();
}

void PlatformManager::updateConfig(const MultiPlatformConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = config;
    rebuildPlatforms();
}

std::vector<PlatformState> PlatformManager::getActivePlatforms() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<PlatformState> active;
    for (const auto& entry : m_platforms) {
        if (entry.second.config.enabled && entry.second.active) {
            active.push_back(entry.second);
        }
    }
    return active;
}

std::optional<PlatformState> PlatformManager::getPlatform(const std::string& platformId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_platforms.find(platformId);
    if (it != m_platforms.end()) {
        return it->second;
    }
    return std::nullopt;
}

int PlatformManager::getAggregateBitrate() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    int total = 0;
    for (const auto& entry : m_platforms) {
        if (entry.second.config.enabled && entry.second.active) {
            total += entry.second.config.output.bitrate;
        }
    }
    return total;
}

void PlatformManager::setPlatformActive(const std::string& platformId, bool active) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_platforms.find(platformId);
    if (it != m_platforms.end()) {
        it->second.active = active;
        Logger::info("PlatformManager set platform " + platformId + (active ? " active" : " inactive"));
    }
}

void PlatformManager::rebuildPlatforms() {
    m_platforms.clear();
    if (!m_config.enabled) {
        return;
    }

    for (const auto& platform : m_config.platforms) {
        PlatformState state;
        state.config = platform;
        state.active = platform.enabled;
        m_platforms[platform.id] = state;
    }
}
