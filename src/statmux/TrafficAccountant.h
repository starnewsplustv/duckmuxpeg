#pragma once

#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <chrono>

#include "common/Config.h"

struct TrafficLogEntry {
    std::string itemId;
    std::string category;
    std::chrono::system_clock::time_point startTime;
    std::chrono::seconds duration{0};
    bool reconciled{false};
};

struct TrafficSummary {
    std::map<std::string, std::chrono::seconds> categoryTotals;
    std::size_t breakCount{0};
    std::size_t discrepancies{0};
};

class TrafficAccountant {
public:
    explicit TrafficAccountant(const TrafficAccountingConfig& config);

    void updateConfig(const TrafficAccountingConfig& config);

    void recordPlayout(const PlaylistItemConfig& item,
                       std::chrono::seconds actualDuration,
                       const std::chrono::system_clock::time_point& startTime);

    std::optional<TrafficSummary> generateDailySummary(const std::chrono::system_clock::time_point& day) const;
    std::vector<TrafficLogEntry> pendingReconciliation() const;

private:
    TrafficAccountingConfig m_config;
    mutable std::mutex m_mutex;
    std::vector<TrafficLogEntry> m_entries;

    void reconcileIfNeeded();
};
