#include "LiveSourceScheduler.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <ctime>

#include "common/Logger.h"

namespace {

std::tm parseTime(const std::string& value) {
    std::tm tm{};
    if (value.empty()) {
        return tm;
    }

    std::istringstream ss(value);
    if (value.find('T') != std::string::npos) {
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    } else if (value.find('-') != std::string::npos) {
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    } else {
        ss >> std::get_time(&tm, "%H:%M:%S");
        if (ss.fail()) {
            ss.clear();
            ss.str(value);
            ss >> std::get_time(&tm, "%H:%M");
        }
    }
    return tm;
}

std::chrono::system_clock::time_point floorToDay(const std::chrono::system_clock::time_point& tp) {
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&tt);
    tm.tm_sec = 0;
    tm.tm_min = 0;
    tm.tm_hour = 0;
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

}

LiveSourceScheduler::LiveSourceScheduler(const LiveSchedulingConfig& config)
    : m_config(config) {
    rebuildSlots(std::chrono::system_clock::now());
}

void LiveSourceScheduler::updateConfig(const LiveSchedulingConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = config;
    rebuildSlots(std::chrono::system_clock::now());
}

std::optional<LiveSlotState> LiveSourceScheduler::getActiveSlot(
    const std::chrono::system_clock::time_point& now) const {
    if (!m_config.enabled) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& slot : m_slots) {
        auto preRoll = std::chrono::seconds(m_config.pre_roll_seconds);
        auto postRoll = std::chrono::seconds(m_config.post_roll_seconds);
        if (now >= slot.scheduledStart - preRoll && now <= slot.scheduledEnd + postRoll) {
            return slot;
        }
    }
    return std::nullopt;
}

std::optional<LiveSlotState> LiveSourceScheduler::getUpcomingSlot(
    const std::chrono::system_clock::time_point& now) const {
    if (!m_config.enabled) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::min_element(m_slots.begin(), m_slots.end(),
                               [&now](const LiveSlotState& lhs, const LiveSlotState& rhs) {
                                   auto lhsDiff = lhs.scheduledStart > now ? lhs.scheduledStart - now : std::chrono::seconds::max();
                                   auto rhsDiff = rhs.scheduledStart > now ? rhs.scheduledStart - now : std::chrono::seconds::max();
                                   return lhsDiff < rhsDiff;
                               });
    if (it != m_slots.end() && it->scheduledStart > now) {
        return *it;
    }
    return std::nullopt;
}

void LiveSourceScheduler::markSlotStarted(const std::string& slotId,
                                          const std::chrono::system_clock::time_point& start) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& slot : m_slots) {
        if (slot.config.id == slotId) {
            slot.active = true;
            slot.scheduledStart = start;
            Logger::info("LiveSourceScheduler slot started: " + slotId);
            break;
        }
    }
}

void LiveSourceScheduler::markSlotCompleted(const std::string& slotId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& slot : m_slots) {
        if (slot.config.id == slotId) {
            slot.active = false;
            Logger::info("LiveSourceScheduler slot completed: " + slotId);
            break;
        }
    }
}

std::chrono::system_clock::time_point LiveSourceScheduler::parseTimePoint(
    const std::string& timeString,
    const std::chrono::system_clock::time_point& reference) {
    auto dayStart = floorToDay(reference);
    std::tm tm = parseTime(timeString);
    auto base = std::chrono::system_clock::to_time_t(dayStart);
    std::tm baseTm = *std::localtime(&base);
    if (tm.tm_hour != 0 || tm.tm_min != 0 || tm.tm_sec != 0) {
        baseTm.tm_hour = tm.tm_hour;
        baseTm.tm_min = tm.tm_min;
        baseTm.tm_sec = tm.tm_sec;
    }
    return std::chrono::system_clock::from_time_t(std::mktime(&baseTm));
}

void LiveSourceScheduler::rebuildSlots(const std::chrono::system_clock::time_point& reference) {
    m_slots.clear();
    if (!m_config.enabled) {
        return;
    }

    for (const auto& slot : m_config.slots) {
        LiveSlotState state;
        state.config = slot;
        state.scheduledStart = parseTimePoint(slot.start_time, reference);
        state.scheduledEnd = parseTimePoint(slot.end_time, reference);
        state.active = false;
        m_slots.push_back(state);
    }
}
