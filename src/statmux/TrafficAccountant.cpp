#include "TrafficAccountant.h"

#include <algorithm>
#include <ctime>

#include "common/Logger.h"

namespace {

std::chrono::system_clock::time_point floorToDay(const std::chrono::system_clock::time_point& tp) {
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&tt);
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

}

TrafficAccountant::TrafficAccountant(const TrafficAccountingConfig& config)
    : m_config(config) {}

void TrafficAccountant::updateConfig(const TrafficAccountingConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = config;
    reconcileIfNeeded();
}

void TrafficAccountant::recordPlayout(const PlaylistItemConfig& item,
                                      std::chrono::seconds actualDuration,
                                      const std::chrono::system_clock::time_point& startTime) {
    if (!m_config.enabled) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    TrafficLogEntry entry;
    entry.itemId = item.id;
    entry.category = item.category;
    entry.duration = actualDuration;
    entry.startTime = startTime;
    entry.reconciled = !m_config.auto_reconcile;
    m_entries.push_back(entry);

    Logger::debug("TrafficAccountant recorded playout for " + item.id +
                  " duration=" + std::to_string(actualDuration.count()));

    reconcileIfNeeded();
}

std::optional<TrafficSummary> TrafficAccountant::generateDailySummary(
    const std::chrono::system_clock::time_point& day) const {
    if (!m_config.enabled) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    TrafficSummary summary;
    auto dayStart = floorToDay(day);
    auto dayEnd = dayStart + std::chrono::hours(24);

    for (const auto& entry : m_entries) {
        if (entry.startTime >= dayStart && entry.startTime < dayEnd) {
            summary.categoryTotals[entry.category] += entry.duration;
            summary.breakCount++;
            if (!entry.reconciled) {
                summary.discrepancies++;
            }
        }
    }

    return summary;
}

std::vector<TrafficLogEntry> TrafficAccountant::pendingReconciliation() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<TrafficLogEntry> pending;
    for (const auto& entry : m_entries) {
        if (!entry.reconciled) {
            pending.push_back(entry);
        }
    }
    return pending;
}

void TrafficAccountant::reconcileIfNeeded() {
    if (!m_config.auto_reconcile) {
        return;
    }

    for (auto& entry : m_entries) {
        if (entry.reconciled) {
            continue;
        }

        auto windowIt = std::find_if(m_config.windows.begin(), m_config.windows.end(),
                                     [&entry](const TrafficWindowConfig& window) {
                                         return std::find(window.categories.begin(), window.categories.end(), entry.category) != window.categories.end();
                                     });
        if (windowIt != m_config.windows.end()) {
            entry.reconciled = entry.duration.count() <= windowIt->max_duration_sec;
        }
    }
}
