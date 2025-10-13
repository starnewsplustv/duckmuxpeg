#include "BroadcastComplianceUI.h"

#include <iomanip>
#include <sstream>

#include "common/Logger.h"

namespace {
std::string to_string(ATSCStandard standard) {
    return standard == ATSCStandard::ATSC_1_0 ? "ATSC_1_0" : "ATSC_3_0";
}

std::string to_string(VideoFormat format) {
    switch (format) {
        case VideoFormat::HDTV_720p: return "HDTV_720p";
        case VideoFormat::HDTV_1080i: return "HDTV_1080i";
        case VideoFormat::HDTV_1080p: return "HDTV_1080p";
        case VideoFormat::SDTV_480i: return "SDTV_480i";
        case VideoFormat::SDTV_480p: return "SDTV_480p";
    }
    return "Unknown";
}

std::string to_string(AspectRatio ratio) {
    return ratio == AspectRatio::AR_16_9 ? "16:9" : "4:3";
}

VideoFormat parse_video_format(const std::string& value, VideoFormat fallback) {
    if (value == "HDTV_720p") return VideoFormat::HDTV_720p;
    if (value == "HDTV_1080p") return VideoFormat::HDTV_1080p;
    if (value == "SDTV_480i") return VideoFormat::SDTV_480i;
    if (value == "SDTV_480p") return VideoFormat::SDTV_480p;
    return fallback;
}

AspectRatio parse_aspect_ratio(const std::string& value, AspectRatio fallback) {
    if (value == "4:3") return AspectRatio::AR_4_3;
    if (value == "16:9") return AspectRatio::AR_16_9;
    return fallback;
}

ATSCStandard parse_standard(const std::string& value, ATSCStandard fallback) {
    if (value == "ATSC_3_0") return ATSCStandard::ATSC_3_0;
    if (value == "ATSC_1_0") return ATSCStandard::ATSC_1_0;
    return fallback;
}
} // namespace

BroadcastComplianceUI::BroadcastComplianceUI() = default;

obs_stub::Properties BroadcastComplianceUI::create_properties() const {
    obs_stub::Properties props;
    props.items.push_back({"standard", "ATSC Standard", to_string(m_state.standard)});
    props.items.push_back({"video_format", "Video Format", to_string(m_state.video_format)});
    props.items.push_back({"aspect_ratio", "Aspect Ratio", to_string(m_state.aspect_ratio)});
    props.items.push_back({"frame_rate", "Frame Rate", std::to_string(m_state.frame_rate)});
    props.items.push_back({"closed_captioning", "Closed Captioning", m_state.closed_captioning ? "true" : "false"});
    props.items.push_back({"audio_description", "Audio Description", m_state.audio_description ? "true" : "false"});
    props.items.push_back({"rating_region", "Rating Region", std::to_string(m_state.rating_region)});
    return props;
}

void BroadcastComplianceUI::apply_settings(const obs_stub::Data& settings) {
    if (settings.values.empty()) {
        return;
    }

    if (auto it = settings.values.find("standard"); it != settings.values.end()) {
        m_state.standard = parse_standard(it->second, m_state.standard);
        if (m_compliance) {
            m_compliance->configure_standard(m_state.standard);
        }
    }
    if (auto it = settings.values.find("video_format"); it != settings.values.end()) {
        m_state.video_format = parse_video_format(it->second, m_state.video_format);
    }
    if (auto it = settings.values.find("aspect_ratio"); it != settings.values.end()) {
        m_state.aspect_ratio = parse_aspect_ratio(it->second, m_state.aspect_ratio);
    }
    if (auto it = settings.values.find("frame_rate"); it != settings.values.end()) {
        try {
            m_state.frame_rate = std::stod(it->second);
        } catch (...) {
            Logger::warn("Invalid frame rate provided to UI");
        }
    }
    if (auto it = settings.values.find("closed_captioning"); it != settings.values.end()) {
        m_state.closed_captioning = (it->second == "true" || it->second == "1");
    }
    if (auto it = settings.values.find("audio_description"); it != settings.values.end()) {
        m_state.audio_description = (it->second == "true" || it->second == "1");
    }
    if (auto it = settings.values.find("rating_region"); it != settings.values.end()) {
        try {
            m_state.rating_region = static_cast<uint8_t>(std::stoi(it->second));
        } catch (...) {
            Logger::warn("Invalid rating region provided to UI");
        }
    }

    if (m_compliance) {
        m_compliance->set_video_format(m_state.video_format, m_state.aspect_ratio, m_state.frame_rate);
        m_compliance->configure_closed_captioning(m_state.closed_captioning);
        m_compliance->configure_audio_description(m_state.audio_description);
        m_compliance->configure_rating_region(m_state.rating_region);
    }

    refresh_from_backend();
}

obs_stub::Data BroadcastComplianceUI::serialize_settings() const {
    obs_stub::Data data;
    data.values["standard"] = to_string(m_state.standard);
    data.values["video_format"] = to_string(m_state.video_format);
    data.values["aspect_ratio"] = to_string(m_state.aspect_ratio);
    data.values["frame_rate"] = std::to_string(m_state.frame_rate);
    data.values["closed_captioning"] = m_state.closed_captioning ? "true" : "false";
    data.values["audio_description"] = m_state.audio_description ? "true" : "false";
    data.values["rating_region"] = std::to_string(m_state.rating_region);
    data.values["channel_count"] = std::to_string(m_state.channels.size());
    data.values["event_sources"] = std::to_string(m_state.events.size());
    return data;
}

void BroadcastComplianceUI::set_compliance_callback(std::function<void(const ComplianceReport&)> callback) {
    m_callback = std::move(callback);
}

void BroadcastComplianceUI::attach_encoder(std::shared_ptr<ModernMPEG2Encoder> encoder) {
    m_encoder = std::move(encoder);
}

void BroadcastComplianceUI::attach_compliance_checker(std::shared_ptr<ATSCCompliance> compliance) {
    m_compliance = std::move(compliance);
    refresh_from_backend();
}

ComplianceReport BroadcastComplianceUI::run_validation() {
    if (!m_compliance) {
        Logger::error("Compliance checker not attached");
        return {};
    }

    auto report = m_compliance->generate_compliance_report();
    if (m_callback) {
        m_callback(report);
    }
    if (auto stats = m_compliance->get_compliance_stats(); stats.total_packets_checked > 0) {
        m_monitor_widget.update(stats);
    }
    return report;
}

ComplianceStats BroadcastComplianceUI::current_stats() const {
    if (m_compliance) {
        return m_compliance->get_compliance_stats();
    }
    return {};
}

std::string BroadcastComplianceUI::render_dashboard() const {
    std::ostringstream oss;
    oss << "DuckMuxPeg Broadcast Dashboard\n";
    oss << "==============================\n";
    oss << "Standard: " << to_string(m_state.standard) << "\n";
    oss << "Video Format: " << to_string(m_state.video_format) << " @ " << m_state.frame_rate << "fps\n";
    oss << "Aspect Ratio: " << to_string(m_state.aspect_ratio) << "\n";
    oss << "Closed Captioning: " << (m_state.closed_captioning ? "Enabled" : "Disabled") << "\n";
    oss << "Audio Description: " << (m_state.audio_description ? "Enabled" : "Disabled") << "\n";
    oss << "Rating Region: " << static_cast<int>(m_state.rating_region) << "\n\n";

    oss << m_monitor_widget.render() << "\n";
    oss << render_channels() << "\n";
    oss << render_events();
    return oss.str();
}

void BroadcastComplianceUI::refresh_from_backend() {
    if (!m_compliance) {
        return;
    }
    m_state.channels = m_compliance->get_virtual_channels();
    m_state.events = m_compliance->get_events();
    m_channel_widget.set_channels(m_state.channels);
}

std::string BroadcastComplianceUI::render_channels() const {
    std::ostringstream oss;
    oss << "Channels (" << m_state.channels.size() << ")\n";
    oss << "------------------------------\n";
    for (const auto& channel : m_state.channels) {
        oss << channel.major_channel << "." << channel.minor_channel
            << " - " << channel.short_name << " (Program " << channel.program_number << ")\n";
    }
    if (m_state.channels.empty()) {
        oss << "No channels configured\n";
    }
    return oss.str();
}

std::string BroadcastComplianceUI::render_events() const {
    std::ostringstream oss;
    oss << "Upcoming Events\n";
    oss << "------------------------------\n";
    if (m_state.events.empty()) {
        oss << "No scheduled events\n";
        return oss.str();
    }
    for (const auto& [source, events] : m_state.events) {
        oss << "Source ID " << source << "\n";
        for (const auto& event : events) {
            const auto start = std::chrono::system_clock::to_time_t(event.start_time);
            oss << "  * " << event.title << " at " << std::put_time(std::localtime(&start), "%Y-%m-%d %H:%M")
                << " for " << event.duration.count() / 60 << " minutes\n";
        }
    }
    return oss.str();
}

void ComplianceMonitorWidget::update(const ComplianceStats& stats) {
    m_stats = stats;
}

std::string ComplianceMonitorWidget::render() const {
    std::ostringstream oss;
    oss << "Compliance Monitor\n";
    oss << "------------------------------\n";
    oss << "Packets Checked: " << m_stats.total_packets_checked << "\n";
    oss << "Compliant Packets: " << m_stats.compliant_packets << "\n";
    oss << "Non-compliant Packets: " << m_stats.non_compliant_packets << "\n";
    oss << std::fixed << std::setprecision(2)
        << "Compliance: " << m_stats.compliance_percentage << "%\n";
    oss << "Monitoring Duration: " << m_stats.monitoring_duration.count() << "ms\n";
    return oss.str();
}

void ChannelManagerWidget::set_channels(const std::vector<VirtualChannelConfig>& channels) {
    m_channels = channels;
}

bool ChannelManagerWidget::add_channel(const VirtualChannelConfig& channel) {
    auto exists = std::find_if(m_channels.begin(), m_channels.end(), [&](const auto& entry) {
        return entry.major_channel == channel.major_channel && entry.minor_channel == channel.minor_channel;
    });
    if (exists != m_channels.end()) {
        return false;
    }
    m_channels.push_back(channel);
    return true;
}

bool ChannelManagerWidget::remove_channel(uint16_t major, uint16_t minor) {
    auto it = std::remove_if(m_channels.begin(), m_channels.end(), [&](const auto& entry) {
        return entry.major_channel == major && entry.minor_channel == minor;
    });
    if (it == m_channels.end()) {
        return false;
    }
    m_channels.erase(it, m_channels.end());
    return true;
}

std::vector<VirtualChannelConfig> ChannelManagerWidget::channels() const {
    return m_channels;
}

std::string ChannelManagerWidget::render() const {
    std::ostringstream oss;
    oss << "Channel Manager\n";
    oss << "------------------------------\n";
    for (const auto& channel : m_channels) {
        oss << channel.major_channel << "." << channel.minor_channel << " - " << channel.service_name << "\n";
    }
    if (m_channels.empty()) {
        oss << "No channels configured\n";
    }
    return oss.str();
}
