#pragma once

#include <deque>
#include <optional>
#include <string>
#include <vector>
#include <chrono>
#include <unordered_map>

#include "common/Config.h"

struct PlaylistRuntimeItem {
    PlaylistItemConfig config;
    std::chrono::system_clock::time_point scheduledStart;
    std::chrono::system_clock::time_point lastStart;
    bool completed{false};
};

class PlaylistManager {
public:
    explicit PlaylistManager(const PlaylistConfig& config);

    void updateConfig(const PlaylistConfig& config);

    std::optional<PlaylistRuntimeItem> getNextItem(const std::chrono::system_clock::time_point& now);
    void markItemCompleted(const std::string& itemId, std::chrono::seconds actualDuration);
    void insertBreakingItem(const PlaylistItemConfig& item);

    std::vector<PlaylistRuntimeItem> peekUpcoming(std::size_t count) const;

private:
    PlaylistConfig m_config;
    std::deque<PlaylistRuntimeItem> m_items;
    std::unordered_map<std::string, PlaylistRuntimeItem> m_history;

    static std::chrono::system_clock::time_point computeStartTime(const std::string& timeString,
                                                                  const std::chrono::system_clock::time_point& reference);
    void rebuildQueue(const std::chrono::system_clock::time_point& reference);
};
