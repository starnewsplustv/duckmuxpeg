#include "ATSCCompliance.h"

#include <algorithm>
#include <cstring>
#include <random>
#include <sstream>

#include "common/Logger.h"

namespace {
constexpr uint8_t kTsSyncByte = 0x47;

uint32_t write_u32(std::vector<uint8_t>& buffer, uint32_t value) {
    for (int i = 0; i < 4; ++i) {
        buffer.push_back(static_cast<uint8_t>((value >> ((3 - i) * 8)) & 0xFF));
    }
    return value;
}

std::vector<uint8_t> serialize_string(const std::string& text, size_t max_bytes) {
    std::vector<uint8_t> result;
    result.reserve(std::min(max_bytes, text.size()));
    for (size_t i = 0; i < text.size() && i < max_bytes; ++i) {
        result.push_back(static_cast<uint8_t>(text[i]));
    }
    return result;
}
} // namespace

ATSCCompliance::ATSCCompliance()
    : ATSCCompliance(ATSCStandard::ATSC_1_0) {}

ATSCCompliance::ATSCCompliance(ATSCStandard standard)
    : m_standard(standard)
    , m_video_format(VideoFormat::HDTV_1080i)
    , m_aspect_ratio(AspectRatio::AR_16_9)
    , m_frame_rate(29.97)
    , m_audio_sample_rate(48000)
    , m_audio_channels(2)
    , m_closed_captioning_enabled(true)
    , m_audio_description_enabled(false)
    , m_rating_region(1)
    , m_monitoring_active(false) {
    m_stats.compliance_percentage = 100.0;
}

ATSCCompliance::~ATSCCompliance() {
    stop_compliance_monitoring();
}

bool ATSCCompliance::configure_standard(ATSCStandard standard) {
    m_standard = standard;
    if (standard == ATSCStandard::ATSC_1_0) {
        m_frame_rate = 29.97;
        m_audio_sample_rate = 48000;
    } else {
        m_frame_rate = 59.94;
        m_audio_sample_rate = 48000;
    }
    return true;
}

bool ATSCCompliance::set_video_format(VideoFormat format, AspectRatio ratio, double fps) {
    m_video_format = format;
    m_aspect_ratio = ratio;
    if (fps <= 0.0) {
        return false;
    }
    m_frame_rate = fps;
    return true;
}

bool ATSCCompliance::set_audio_format(int sample_rate, int channels) {
    if (sample_rate <= 0 || channels <= 0) {
        return false;
    }
    m_audio_sample_rate = sample_rate;
    m_audio_channels = channels;
    return true;
}

bool ATSCCompliance::add_virtual_channel(const VirtualChannelConfig& channel) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto exists = std::find_if(m_channels.begin(), m_channels.end(), [&](const auto& entry) {
        return entry.major_channel == channel.major_channel && entry.minor_channel == channel.minor_channel;
    });
    if (exists != m_channels.end()) {
        Logger::warn("Virtual channel already exists");
        return false;
    }
    m_channels.push_back(channel);
    return true;
}

bool ATSCCompliance::update_virtual_channel(const VirtualChannelConfig& channel) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::find_if(m_channels.begin(), m_channels.end(), [&](const auto& entry) {
        return entry.major_channel == channel.major_channel && entry.minor_channel == channel.minor_channel;
    });
    if (it == m_channels.end()) {
        m_channels.push_back(channel);
    } else {
        *it = channel;
    }
    return true;
}

std::vector<VirtualChannelConfig> ATSCCompliance::get_virtual_channels() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_channels;
}

bool ATSCCompliance::add_event(uint16_t source_id, const ProgramEvent& event) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_events[source_id].push_back(event);
    return true;
}

std::map<uint16_t, std::vector<ProgramEvent>> ATSCCompliance::get_events() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_events;
}

bool ATSCCompliance::configure_rating_region(uint8_t region) {
    m_rating_region = region;
    return true;
}

bool ATSCCompliance::configure_closed_captioning(bool enabled) {
    m_closed_captioning_enabled = enabled;
    return true;
}

bool ATSCCompliance::configure_audio_description(bool enabled) {
    m_audio_description_enabled = enabled;
    return true;
}

bool ATSCCompliance::validate_transport_stream(const std::vector<uint8_t>& ts_data) {
    if (ts_data.size() < 188) {
        return false;
    }
    size_t packets = ts_data.size() / 188;
    size_t compliant = 0;
    for (size_t i = 0; i < packets; ++i) {
        if (ts_data[i * 188] == kTsSyncByte) {
            compliant++;
        }
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    update_stats_locked(packets, compliant);
    return compliant == packets;
}

ComplianceReport ATSCCompliance::generate_compliance_report() const {
    ComplianceReport report;
    report.section_results["video_format"] = (m_video_format == VideoFormat::HDTV_1080i ||
                                               m_video_format == VideoFormat::HDTV_720p);
    report.section_results["audio_format"] = (m_audio_channels >= 2);
    report.section_results["closed_captioning"] = m_closed_captioning_enabled;
    report.section_results["rating_region"] = (m_rating_region > 0);

    for (const auto& [section, ok] : report.section_results) {
        if (!ok) {
            report.overall_compliant = false;
            report.errors.push_back("Section " + section + " failed compliance");
        }
    }

    if (report.overall_compliant) {
        report.warnings.push_back("Compliance check passed with no issues");
    }

    return report;
}

ComplianceStats ATSCCompliance::get_compliance_stats() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats;
}

bool ATSCCompliance::start_compliance_monitoring() {
    if (m_monitoring_active.load()) {
        return true;
    }
    m_monitoring_active = true;
    m_monitor_start = std::chrono::steady_clock::now();
    m_monitor_thread = std::thread(&ATSCCompliance::monitoring_worker, this);
    return true;
}

bool ATSCCompliance::stop_compliance_monitoring() {
    if (!m_monitoring_active.load()) {
        return true;
    }
    m_monitoring_active = false;
    if (m_monitor_thread.joinable()) {
        m_monitor_thread.join();
    }
    return true;
}

bool ATSCCompliance::is_monitoring() const {
    return m_monitoring_active.load();
}

std::vector<uint8_t> ATSCCompliance::build_virtual_channel_table() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<uint8_t> buffer;
    buffer.reserve(64);
    write_u32(buffer, static_cast<uint32_t>(m_channels.size()));
    for (const auto& channel : m_channels) {
        write_u32(buffer, static_cast<uint32_t>(channel.major_channel));
        write_u32(buffer, static_cast<uint32_t>(channel.minor_channel));
        auto short_name = serialize_string(channel.short_name, 7);
        buffer.push_back(static_cast<uint8_t>(short_name.size()));
        buffer.insert(buffer.end(), short_name.begin(), short_name.end());
    }
    return buffer;
}

std::vector<uint8_t> ATSCCompliance::build_event_information_table(uint16_t source_id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<uint8_t> buffer;
    const auto it = m_events.find(source_id);
    if (it == m_events.end()) {
        return buffer;
    }
    write_u32(buffer, static_cast<uint32_t>(it->second.size()));
    for (const auto& event : it->second) {
        write_u32(buffer, event.event_id);
        const auto start = std::chrono::duration_cast<std::chrono::seconds>(event.start_time.time_since_epoch()).count();
        write_u32(buffer, static_cast<uint32_t>(start));
        write_u32(buffer, static_cast<uint32_t>(event.duration.count()));
        auto title = serialize_string(event.title, 32);
        buffer.push_back(static_cast<uint8_t>(title.size()));
        buffer.insert(buffer.end(), title.begin(), title.end());
    }
    return buffer;
}

std::vector<uint8_t> ATSCCompliance::build_system_time_table() const {
    std::vector<uint8_t> buffer;
    const auto now = std::chrono::system_clock::now();
    const auto gps_time = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    write_u32(buffer, static_cast<uint32_t>(gps_time));
    buffer.push_back(m_rating_region);
    buffer.push_back(static_cast<uint8_t>(m_closed_captioning_enabled));
    buffer.push_back(static_cast<uint8_t>(m_audio_description_enabled));
    return buffer;
}

void ATSCCompliance::monitoring_worker() {
    std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(170, 188);

    while (m_monitoring_active.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        const auto packets = static_cast<uint64_t>(dist(rng));
        const auto compliant = packets - 1; // simulate occasional mismatch
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            update_stats_locked(packets, compliant);
            const auto elapsed = std::chrono::steady_clock::now() - m_monitor_start;
            m_stats.monitoring_duration = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
        }
    }
}

void ATSCCompliance::update_stats_locked(uint64_t packets_checked, uint64_t compliant_packets) {
    m_stats.total_packets_checked += packets_checked;
    m_stats.compliant_packets += compliant_packets;
    m_stats.non_compliant_packets += (packets_checked - compliant_packets);
    if (m_stats.total_packets_checked > 0) {
        m_stats.compliance_percentage =
            100.0 * static_cast<double>(m_stats.compliant_packets) /
            static_cast<double>(m_stats.total_packets_checked);
    }
}
