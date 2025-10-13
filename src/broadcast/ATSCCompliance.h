#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "common/Config.h"

enum class ATSCStandard {
    ATSC_1_0,
    ATSC_3_0
};

struct VirtualChannelConfig {
    uint16_t major_channel = 1;
    uint16_t minor_channel = 1;
    std::string short_name = "DUCK";
    std::string service_name = "DuckMuxPeg";
    uint16_t program_number = 1;
    uint16_t source_id = 1;
    bool access_controlled = false;
};

struct ProgramEvent {
    uint16_t event_id = 0;
    std::string title;
    std::chrono::system_clock::time_point start_time;
    std::chrono::seconds duration{0};
    std::string description;
};

struct ComplianceReport {
    bool overall_compliant = true;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::map<std::string, bool> section_results;
};

struct ComplianceStats {
    uint64_t total_packets_checked = 0;
    uint64_t compliant_packets = 0;
    uint64_t non_compliant_packets = 0;
    double compliance_percentage = 100.0;
    std::chrono::milliseconds monitoring_duration{0};
};

class ATSCCompliance {
public:
    ATSCCompliance();
    explicit ATSCCompliance(ATSCStandard standard);
    ~ATSCCompliance();

    bool configure_standard(ATSCStandard standard);
    bool set_video_format(VideoFormat format, AspectRatio ratio, double fps);
    bool set_audio_format(int sample_rate, int channels);

    bool add_virtual_channel(const VirtualChannelConfig& channel);
    bool update_virtual_channel(const VirtualChannelConfig& channel);
    std::vector<VirtualChannelConfig> get_virtual_channels() const;

    bool add_event(uint16_t source_id, const ProgramEvent& event);
    std::map<uint16_t, std::vector<ProgramEvent>> get_events() const;

    bool configure_rating_region(uint8_t region);
    bool configure_closed_captioning(bool enabled);
    bool configure_audio_description(bool enabled);

    bool validate_transport_stream(const std::vector<uint8_t>& ts_data);

    ComplianceReport generate_compliance_report() const;
    ComplianceStats get_compliance_stats() const;

    bool start_compliance_monitoring();
    bool stop_compliance_monitoring();
    bool is_monitoring() const;

    std::vector<uint8_t> build_virtual_channel_table() const;
    std::vector<uint8_t> build_event_information_table(uint16_t source_id) const;
    std::vector<uint8_t> build_system_time_table() const;

private:
    ATSCStandard m_standard;
    VideoFormat m_video_format;
    AspectRatio m_aspect_ratio;
    double m_frame_rate;
    int m_audio_sample_rate;
    int m_audio_channels;

    bool m_closed_captioning_enabled;
    bool m_audio_description_enabled;
    uint8_t m_rating_region;

    std::vector<VirtualChannelConfig> m_channels;
    std::map<uint16_t, std::vector<ProgramEvent>> m_events;

    mutable std::mutex m_mutex;
    std::atomic<bool> m_monitoring_active;
    std::thread m_monitor_thread;
    std::chrono::steady_clock::time_point m_monitor_start;
    ComplianceStats m_stats;

    void monitoring_worker();
    void update_stats_locked(uint64_t packets_checked, uint64_t compliant_packets);
};
