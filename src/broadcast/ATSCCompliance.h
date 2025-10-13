#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>

#include "common/Config.h"

// ATSC 1.0 Standard Compliance System

enum class ATSCStandard {
    ATSC_1_0,
    ATSC_3_0
};

enum class VideoFormat {
    HDTV_720p,
    HDTV_1080i,
    HDTV_1080p,
    SDTV_480i,
    SDTV_480p
};

enum class AspectRatio {
    AR_4_3,
    AR_16_9
};

// Program and System Information Protocol (PSIP) Tables
struct PSIPTable {
    virtual ~PSIPTable() = default;
    virtual std::vector<uint8_t> serialize() const = 0;
    virtual bool validate() const = 0;
};

// Master Guide Table (MGT)
struct MasterGuideTable : public PSIPTable {
    uint16_t table_id = 0xC7;
    uint16_t table_id_extension;
    uint8_t version_number = 0;
    bool current_next_indicator = true;
    uint8_t section_number = 0;
    uint8_t last_section_number = 0;
    uint8_t protocol_version = 0;
    
    struct TableEntry {
        uint16_t table_type;
        uint16_t table_type_PID;
        uint8_t table_type_version_number;
        uint32_t number_bytes;
        uint16_t table_type_descriptors_length;
        std::vector<uint8_t> descriptors;
    };
    
    std::vector<TableEntry> tables;
    uint16_t descriptors_length = 0;
    std::vector<uint8_t> descriptors;
    
    std::vector<uint8_t> serialize() const override;
    bool validate() const override;
};

// Virtual Channel Table (VCT)
struct VirtualChannelTable : public PSIPTable {
    uint16_t table_id = 0xC8; // TVCT
    uint16_t transport_stream_id;
    uint8_t version_number = 0;
    bool current_next_indicator = true;
    uint8_t section_number = 0;
    uint8_t last_section_number = 0;
    uint8_t protocol_version = 0;
    
    struct Channel {
        std::string short_name; // 7 UTF-16 characters
        uint32_t major_channel_number : 10;
        uint32_t minor_channel_number : 10;
        uint8_t modulation_mode;
        uint32_t carrier_frequency;
        uint16_t channel_TSID;
        uint16_t program_number;
        uint8_t ETM_location : 2;
        bool access_controlled : 1;
        bool hidden : 1;
        bool hide_guide : 1;
        uint8_t service_type : 6;
        uint16_t source_id;
        uint16_t descriptors_length;
        std::vector<uint8_t> descriptors;
    };
    
    std::vector<Channel> channels;
    uint16_t additional_descriptors_length = 0;
    std::vector<uint8_t> additional_descriptors;
    
    std::vector<uint8_t> serialize() const override;
    bool validate() const override;
};

// Event Information Table (EIT)
struct EventInformationTable : public PSIPTable {
    uint16_t table_id = 0xCB;
    uint16_t source_id;
    uint8_t version_number = 0;
    bool current_next_indicator = true;
    uint8_t section_number = 0;
    uint8_t last_section_number = 0;
    uint8_t protocol_version = 0;
    
    struct Event {
        uint16_t event_id;
        uint32_t start_time; // GPS seconds since 1980-01-06 00:00:00 UTC
        uint8_t ETM_location : 2;
        uint32_t length_in_seconds : 20;
        std::string title_text;
        uint16_t descriptors_length;
        std::vector<uint8_t> descriptors;
    };
    
    std::vector<Event> events;
    
    std::vector<uint8_t> serialize() const override;
    bool validate() const override;
};

// System Time Table (STT)
struct SystemTimeTable : public PSIPTable {
    uint16_t table_id = 0xCD;
    uint8_t version_number = 0;
    bool current_next_indicator = true;
    uint8_t section_number = 0;
    uint8_t last_section_number = 0;
    uint8_t protocol_version = 0;
    
    uint32_t system_time; // GPS seconds since 1980-01-06 00:00:00 UTC
    uint8_t GPS_UTC_offset;
    uint16_t daylight_saving : 16;
    uint16_t descriptors_length = 0;
    std::vector<uint8_t> descriptors;
    
    std::vector<uint8_t> serialize() const override;
    bool validate() const override;
};

// Rating Region Table (RRT)
struct RatingRegionTable : public PSIPTable {
    uint16_t table_id = 0xCA;
    uint8_t rating_region;
    uint8_t version_number = 0;
    bool current_next_indicator = true;
    uint8_t section_number = 0;
    uint8_t last_section_number = 0;
    uint8_t protocol_version = 0;
    
    std::string rating_region_name_text;
    
    struct Dimension {
        std::string dimension_name_text;
        bool graduated_scale : 1;
        uint8_t values_defined : 4;
        
        struct Value {
            std::string rating_value_abbrev_text;
            std::string rating_value_text;
        };
        std::vector<Value> values;
    };
    
    std::vector<Dimension> dimensions;
    uint16_t descriptors_length = 0;
    std::vector<uint8_t> descriptors;
    
    std::vector<uint8_t> serialize() const override;
    bool validate() const override;
};

// ATSC Compliance Checker
class ATSCCompliance {
public:
    explicit ATSCCompliance(ATSCStandard standard = ATSCStandard::ATSC_1_0);
    ~ATSCCompliance();
    
    // Configuration
    bool configure_standard(ATSCStandard standard);
    bool set_video_format(VideoFormat format);
    bool set_aspect_ratio(AspectRatio ratio);
    bool set_frame_rate(double fps);
    
    // PSIP Table Management
    bool add_virtual_channel(const VirtualChannelTable::Channel& channel);
    bool update_master_guide_table();
    bool add_event(uint16_t source_id, const EventInformationTable::Event& event);
    bool update_system_time();
    bool configure_rating_region(uint8_t region);

    // Validation
    bool validate_transport_stream(const std::vector<uint8_t>& ts_data);
    bool validate_video_compliance(int width, int height, double fps);
    bool validate_audio_compliance(int sample_rate, int channels);
    bool validate_psip_tables();

    // Serialization
    std::vector<uint8_t> generate_pat(); // Program Association Table
    std::vector<uint8_t> generate_pmt(uint16_t program_number); // Program Map Table
    std::vector<uint8_t> generate_mgt(); // Master Guide Table
    std::vector<uint8_t> generate_vct(); // Virtual Channel Table
    std::vector<uint8_t> generate_eit(uint16_t source_id); // Event Information Table
    std::vector<uint8_t> generate_stt(); // System Time Table
    std::vector<uint8_t> generate_rrt(); // Rating Region Table

    // Integration helpers
    bool load_wmdv_lineup_from_tsduck();
    
    // Closed Captioning (CEA-608/CEA-708)
    bool configure_closed_captioning(bool enabled);
    bool validate_cc_data(const std::vector<uint8_t>& cc_data);
    
    // Audio Description
    bool configure_audio_description(bool enabled);
    bool validate_audio_description();
    
    // Compliance Reporting
    struct ComplianceReport {
        bool overall_compliant;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        std::map<std::string, bool> section_compliance;
        
        // Detailed checks
        bool video_format_compliant;
        bool audio_format_compliant;
        bool psip_compliant;
        bool closed_caption_compliant;
        bool timing_compliant;
        bool bitrate_compliant;
    };
    
    ComplianceReport generate_compliance_report();
    bool export_compliance_report(const std::string& filename);
    
    // Real-time monitoring
    bool start_compliance_monitoring();
    bool stop_compliance_monitoring();
    bool is_monitoring() const;
    
    // Statistics
    struct ComplianceStats {
        uint64_t total_packets_checked;
        uint64_t compliant_packets;
        uint64_t non_compliant_packets;
        double compliance_percentage;
        std::chrono::milliseconds monitoring_duration;
    };
    
    ComplianceStats get_compliance_stats() const;
    
private:
    ATSCStandard m_standard;
    VideoFormat m_video_format;
    AspectRatio m_aspect_ratio;
    double m_frame_rate;
    
    // PSIP Tables
    std::unique_ptr<MasterGuideTable> m_mgt;
    std::unique_ptr<VirtualChannelTable> m_vct;
    std::vector<std::unique_ptr<EventInformationTable>> m_eits;
    std::unique_ptr<SystemTimeTable> m_stt;
    std::unique_ptr<RatingRegionTable> m_rrt;
    
    // Configuration
    bool m_closed_captioning_enabled;
    bool m_audio_description_enabled;
    
    // Monitoring
    std::atomic<bool> m_monitoring_active;
    std::thread m_monitoring_thread;
    mutable std::mutex m_stats_mutex;
    ComplianceStats m_stats;
    
    // Internal methods
    bool validate_video_format_compliance(int width, int height, double fps);
    bool validate_audio_format_compliance(int sample_rate, int channels);
    bool validate_bitrate_compliance(int bitrate);
    bool validate_gop_structure(const std::vector<uint8_t>& video_data);
    
    // PSIP validation
    bool validate_mgt();
    bool validate_vct();
    bool validate_eit(const EventInformationTable& eit);
    bool validate_stt();
    bool validate_rrt();
    
    // Transport Stream parsing
    struct TSPacket {
        uint8_t sync_byte;
        bool transport_error_indicator;
        bool payload_unit_start_indicator;
        bool transport_priority;
        uint16_t PID;
        uint8_t transport_scrambling_control;
        uint8_t adaptation_field_control;
        uint8_t continuity_counter;
        std::vector<uint8_t> payload;
    };
    
    std::vector<TSPacket> parse_transport_stream(const std::vector<uint8_t>& ts_data);
    bool validate_ts_packet(const TSPacket& packet);
    
    // Timing validation
    bool validate_pcr_timing(const std::vector<TSPacket>& packets);
    bool validate_pts_dts_timing(const std::vector<uint8_t>& pes_data);
    
    // Monitoring thread
    void monitoring_thread_function();
    
    // Error handling
    std::vector<std::string> m_errors;
    std::vector<std::string> m_warnings;
    void add_error(const std::string& error);
    void add_warning(const std::string& warning);
    
    // CRC calculation for PSIP tables
    uint32_t calculate_crc32(const std::vector<uint8_t>& data);
    
    // GPS time conversion
    uint32_t current_gps_time();
    std::string gps_time_to_string(uint32_t gps_time);
};