#include "ATSCCompliance.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <numeric>
#include <thread>

#include "TSDuckIntegration.h"
#include "common/Logger.h"

using namespace std::chrono_literals;

namespace {
constexpr double kDefaultFrameRate = 29.97;
constexpr uint16_t kDefaultTransportStreamId = 0x16A0;
}

ATSCCompliance::ATSCCompliance(ATSCStandard standard)
    : m_standard(standard),
      m_video_format(VideoFormat::HDTV_1080i),
      m_aspect_ratio(AspectRatio::AR_16_9),
      m_frame_rate(kDefaultFrameRate),
      m_mgt(std::make_unique<MasterGuideTable>()),
      m_vct(std::make_unique<VirtualChannelTable>()),
      m_stt(std::make_unique<SystemTimeTable>()),
      m_rrt(std::make_unique<RatingRegionTable>()),
      m_closed_captioning_enabled(false),
      m_audio_description_enabled(false),
      m_monitoring_active(false)
{
    if (m_vct) {
        m_vct->table_id = 0xC8;
        m_vct->transport_stream_id = kDefaultTransportStreamId;
        m_vct->version_number = 0;
        m_vct->current_next_indicator = true;
        m_vct->section_number = 0;
        m_vct->last_section_number = 0;
        m_vct->protocol_version = 0;
    }
}

ATSCCompliance::~ATSCCompliance()
{
    stop_compliance_monitoring();
}

bool ATSCCompliance::add_virtual_channel(const VirtualChannelTable::Channel& channel)
{
    if (!m_vct) {
        add_error("Virtual Channel Table not initialised");
        return false;
    }

    if (channel.major_channel_number == 0 || channel.major_channel_number > 1023) {
        add_error("Major channel number out of range");
        return false;
    }
    if (channel.minor_channel_number > 1023) {
        add_error("Minor channel number out of range");
        return false;
    }

    m_vct->channels.push_back(channel);
    return true;
}

bool ATSCCompliance::update_master_guide_table()
{
    if (!m_mgt || !m_vct) {
        add_error("MGT or VCT not initialised");
        return false;
    }

    m_mgt->tables.clear();
    MasterGuideTable::TableEntry entry{};
    entry.table_type = 0x0000; // TVCT
    entry.table_type_PID = 0x1FFB;
    entry.table_type_version_number = m_vct->version_number;
    entry.number_bytes = static_cast<uint32_t>(m_vct->channels.size() * 32);
    entry.table_type_descriptors_length = 0;
    m_mgt->tables.push_back(entry);
    return true;
}

std::vector<uint8_t> ATSCCompliance::generate_vct()
{
    if (!m_vct) {
        add_error("Virtual Channel Table not available");
        return {};
    }

    return broadcast::TSDuckIntegration::serialize_vct(*m_vct);
}

bool ATSCCompliance::load_wmdv_lineup_from_tsduck()
{
    if (!m_vct) {
        add_error("Virtual Channel Table not available");
        return false;
    }

    const auto lineup = broadcast::TSDuckIntegration::build_wmdv_program_lineup();
    if (lineup.empty()) {
        add_error("No WMDV lineup entries generated");
        return false;
    }

    bool ok = true;
    for (const auto& svc : lineup) {
        ok &= add_virtual_channel(broadcast::TSDuckIntegration::to_virtual_channel(svc));
    }

    if (ok) {
        LOG_INFO("Loaded WMDV virtual channel lineup from TSDuck integration");
    }
    return ok;
}

bool ATSCCompliance::validate_psip_tables()
{
    bool ok = true;
    if (m_vct) {
        ok &= m_vct->validate();
    }
    return ok;
}

bool ATSCCompliance::start_compliance_monitoring()
{
    bool expected = false;
    if (!m_monitoring_active.compare_exchange_strong(expected, true)) {
        return true;
    }

    m_stats = ComplianceStats{};
    m_monitoring_thread = std::thread([this]() { monitoring_thread_function(); });
    return true;
}

bool ATSCCompliance::stop_compliance_monitoring()
{
    bool expected = true;
    if (!m_monitoring_active.compare_exchange_strong(expected, false)) {
        return true;
    }

    if (m_monitoring_thread.joinable()) {
        m_monitoring_thread.join();
    }
    return true;
}

bool ATSCCompliance::is_monitoring() const
{
    return m_monitoring_active.load();
}

void ATSCCompliance::monitoring_thread_function()
{
    auto start = std::chrono::steady_clock::now();
    while (m_monitoring_active.load()) {
        std::this_thread::sleep_for(1s);
        std::lock_guard<std::mutex> lock(m_stats_mutex);
        m_stats.monitoring_duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start);
    }
}

ATSCCompliance::ComplianceStats ATSCCompliance::get_compliance_stats() const
{
    std::lock_guard<std::mutex> lock(m_stats_mutex);
    return m_stats;
}

ATSCCompliance::ComplianceReport ATSCCompliance::generate_compliance_report()
{
    ComplianceReport report{};
    report.video_format_compliant = true;
    report.audio_format_compliant = true;
    report.psip_compliant = validate_psip_tables();
    report.closed_caption_compliant = m_closed_captioning_enabled;
    report.timing_compliant = true;
    report.bitrate_compliant = true;
    report.overall_compliant = report.psip_compliant && report.video_format_compliant;
    report.section_compliance["VCT"] = report.psip_compliant;
    if (!report.psip_compliant) {
        report.errors.push_back("PSIP validation failed");
    }
    return report;
}

void ATSCCompliance::add_error(const std::string& error)
{
    LOG_ERROR(error);
    m_errors.push_back(error);
}

void ATSCCompliance::add_warning(const std::string& warning)
{
    LOG_WARN(warning);
    m_warnings.push_back(warning);
}

bool VirtualChannelTable::validate() const
{
    if (transport_stream_id == 0) {
        return false;
    }

    if (channels.empty()) {
        return false;
    }

    for (const auto& channel : channels) {
        if (channel.short_name.empty()) {
            return false;
        }
        if (channel.major_channel_number == 0 || channel.major_channel_number > 1023) {
            return false;
        }
        if (channel.minor_channel_number > 1023) {
            return false;
        }
        if (channel.program_number == 0) {
            return false;
        }
    }
    return true;
}

std::vector<uint8_t> VirtualChannelTable::serialize() const
{
    return broadcast::TSDuckIntegration::serialize_vct(*this);
}

} // namespace
