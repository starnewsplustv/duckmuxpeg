#include "PlaylistManager.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "common/Logger.h"

namespace {

std::tm toTm(const std::string& timeString) {
    std::tm tm{};
    if (timeString.empty()) {
        return tm;
    }

    std::istringstream ss(timeString);
    if (timeString.find('T') != std::string::npos) {
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    } else if (timeString.find('-') != std::string::npos) {
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    } else {
        ss >> std::get_time(&tm, "%H:%M:%S");
        if (ss.fail()) {
            ss.clear();
            ss.str(timeString);
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

PlaylistManager::PlaylistManager(const PlaylistConfig& config)
    : m_config(config) {
    rebuildQueue(std::chrono::system_clock::now());
}

void PlaylistManager::updateConfig(const PlaylistConfig& config) {
    m_config = config;
    rebuildQueue(std::chrono::system_clock::now());
}

std::optional<PlaylistRuntimeItem> PlaylistManager::getNextItem(const std::chrono::system_clock::time_point& now) {
    if (!m_config.enabled) {
        return std::nullopt;
    }

    if (m_items.empty()) {
        rebuildQueue(now);
    }

    if (m_items.empty()) {
        return std::nullopt;
    }

    auto item = m_items.front();
    if (now + std::chrono::seconds(1) < item.scheduledStart && !item.config.mandatory) {
        return std::nullopt;
    }

    m_items.pop_front();
    item.lastStart = now;
    item.completed = false;
    m_history[item.config.id] = item;

    if (m_config.rotation_mode == "loop") {
        m_items.push_back(item);
    }

    Logger::info("PlaylistManager starting item: " + item.config.id + " (" + item.config.title + ")");
    return item;
}

void PlaylistManager::markItemCompleted(const std::string& itemId, std::chrono::seconds actualDuration) {
    auto it = m_history.find(itemId);
    if (it == m_history.end()) {
        return;
    }

    it->second.completed = true;
    Logger::info("PlaylistManager completed item: " + itemId +
                 ", actual duration " + std::to_string(actualDuration.count()) + "s");
}

void PlaylistManager::insertBreakingItem(const PlaylistItemConfig& item) {
    PlaylistRuntimeItem runtimeItem;
    runtimeItem.config = item;
    runtimeItem.scheduledStart = std::chrono::system_clock::now();
    runtimeItem.lastStart = runtimeItem.scheduledStart;
    runtimeItem.completed = false;

    m_items.push_front(runtimeItem);
    Logger::warn("PlaylistManager inserting breaking item: " + item.id);
}

std::vector<PlaylistRuntimeItem> PlaylistManager::peekUpcoming(std::size_t count) const {
    std::vector<PlaylistRuntimeItem> upcoming;
    count = std::min(count, m_items.size());
    upcoming.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        upcoming.push_back(m_items[i]);
    }
    return upcoming;
}

std::chrono::system_clock::time_point PlaylistManager::computeStartTime(
    const std::string& timeString,
    const std::chrono::system_clock::time_point& reference) {
    auto referenceDay = floorToDay(reference);
    std::tm tm = toTm(timeString);
    auto base = std::chrono::system_clock::to_time_t(referenceDay);
    std::tm baseTm = *std::localtime(&base);
    if (tm.tm_hour != 0 || tm.tm_min != 0 || tm.tm_sec != 0) {
        baseTm.tm_hour = tm.tm_hour;
        baseTm.tm_min = tm.tm_min;
        baseTm.tm_sec = tm.tm_sec;
    }
    return std::chrono::system_clock::from_time_t(std::mktime(&baseTm));
}

void PlaylistManager::rebuildQueue(const std::chrono::system_clock::time_point& reference) {
    m_items.clear();
    if (!m_config.enabled) {
        return;
    }

    for (const auto& item : m_config.items) {
        PlaylistRuntimeItem runtimeItem;
        runtimeItem.config = item;
        runtimeItem.scheduledStart = computeStartTime(item.scheduled_start, reference);
        runtimeItem.lastStart = std::chrono::system_clock::time_point{};
        runtimeItem.completed = false;
        m_items.push_back(runtimeItem);
    }
}
