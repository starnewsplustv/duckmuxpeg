#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>
#include <mutex>

#include "common/Config.h"

struct PlatformState {
    PlatformOutputConfig config;
    bool active{false};
};

class PlatformManager {
public:
    explicit PlatformManager(const MultiPlatformConfig& config);

    void updateConfig(const MultiPlatformConfig& config);

    std::vector<PlatformState> getActivePlatforms() const;
    std::optional<PlatformState> getPlatform(const std::string& platformId) const;
    int getAggregateBitrate() const;

    void setPlatformActive(const std::string& platformId, bool active);

private:
    MultiPlatformConfig m_config;
    std::map<std::string, PlatformState> m_platforms;
    mutable std::mutex m_mutex;

    void rebuildPlatforms();
};
