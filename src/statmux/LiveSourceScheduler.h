#pragma once

#include <optional>
#include <string>
#include <vector>
#include <chrono>
#include <mutex>

#include "common/Config.h"

struct LiveSlotState {
    LiveSlotConfig config;
    std::chrono::system_clock::time_point scheduledStart;
    std::chrono::system_clock::time_point scheduledEnd;
    bool active{false};
};

class LiveSourceScheduler {
public:
    explicit LiveSourceScheduler(const LiveSchedulingConfig& config);

    void updateConfig(const LiveSchedulingConfig& config);

    std::optional<LiveSlotState> getActiveSlot(const std::chrono::system_clock::time_point& now) const;
    std::optional<LiveSlotState> getUpcomingSlot(const std::chrono::system_clock::time_point& now) const;

    void markSlotStarted(const std::string& slotId, const std::chrono::system_clock::time_point& start);
    void markSlotCompleted(const std::string& slotId);

private:
    LiveSchedulingConfig m_config;
    std::vector<LiveSlotState> m_slots;
    mutable std::mutex m_mutex;

    static std::chrono::system_clock::time_point parseTimePoint(const std::string& timeString,
                                                                const std::chrono::system_clock::time_point& reference);
    void rebuildSlots(const std::chrono::system_clock::time_point& reference);
};
