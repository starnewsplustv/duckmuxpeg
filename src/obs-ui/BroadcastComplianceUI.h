#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "broadcast/ATSCCompliance.h"
#include "encoder/ModernMPEG2Encoder.h"

namespace obs_stub {
struct Property {
    std::string name;
    std::string description;
    std::string value;
    bool read_only = false;
};

struct Properties {
    std::vector<Property> items;
};

struct Data {
    std::map<std::string, std::string> values;
};
} // namespace obs_stub

class ComplianceMonitorWidget {
public:
    void update(const ComplianceStats& stats);
    std::string render() const;

private:
    ComplianceStats m_stats{};
};

class ChannelManagerWidget {
public:
    void set_channels(const std::vector<VirtualChannelConfig>& channels);
    bool add_channel(const VirtualChannelConfig& channel);
    bool remove_channel(uint16_t major, uint16_t minor);
    std::vector<VirtualChannelConfig> channels() const;
    std::string render() const;

private:
    std::vector<VirtualChannelConfig> m_channels;
};

class BroadcastComplianceUI {
public:
    BroadcastComplianceUI();

    obs_stub::Properties create_properties() const;
    void apply_settings(const obs_stub::Data& settings);
    obs_stub::Data serialize_settings() const;

    void set_compliance_callback(std::function<void(const ComplianceReport&)> callback);
    void attach_encoder(std::shared_ptr<ModernMPEG2Encoder> encoder);
    void attach_compliance_checker(std::shared_ptr<ATSCCompliance> compliance);

    ComplianceReport run_validation();
    ComplianceStats current_stats() const;
    std::string render_dashboard() const;

private:
    struct PanelState {
        ATSCStandard standard = ATSCStandard::ATSC_1_0;
        VideoFormat video_format = VideoFormat::HDTV_1080i;
        AspectRatio aspect_ratio = AspectRatio::AR_16_9;
        double frame_rate = 29.97;
        bool closed_captioning = true;
        bool audio_description = false;
        uint8_t rating_region = 1;
        std::vector<VirtualChannelConfig> channels;
        std::map<uint16_t, std::vector<ProgramEvent>> events;
    } m_state;

    std::shared_ptr<ModernMPEG2Encoder> m_encoder;
    std::shared_ptr<ATSCCompliance> m_compliance;
    std::function<void(const ComplianceReport&)> m_callback;
    ComplianceMonitorWidget m_monitor_widget;
    ChannelManagerWidget m_channel_widget;

    void refresh_from_backend();
    std::string render_channels() const;
    std::string render_events() const;
};
