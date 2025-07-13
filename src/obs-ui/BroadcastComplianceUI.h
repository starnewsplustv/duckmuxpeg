#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <chrono>

#include "broadcast/ATSCCompliance.h"
#include "encoder/ModernMPEG2Encoder.h"

// Forward declarations for OBS UI types
struct obs_properties;
struct obs_data;
struct obs_source;

// OBS UI Panel for Broadcast Compliance
class BroadcastComplianceUI {
public:
    BroadcastComplianceUI();
    ~BroadcastComplianceUI();
    
    // OBS Integration
    bool register_ui_panels();
    void unregister_ui_panels();
    
    // Main compliance panel
    obs_properties* create_compliance_properties();
    void update_compliance_properties(obs_properties* props, obs_data* settings);
    bool validate_compliance_settings(obs_data* settings);
    
    // PSIP Table Editor Panel
    obs_properties* create_psip_properties();
    void update_psip_properties(obs_properties* props, obs_data* settings);
    
    // Program Guide Panel
    obs_properties* create_program_guide_properties();
    void update_program_guide_properties(obs_properties* props, obs_data* settings);
    
    // Real-time Monitoring Panel
    obs_properties* create_monitoring_properties();
    void update_monitoring_display();
    
    // Statistics Panel
    obs_properties* create_statistics_properties();
    void refresh_statistics();
    
    // Settings and Configuration
    void load_settings(obs_data* settings);
    void save_settings(obs_data* settings);
    obs_data* get_default_settings();
    
    // Event Callbacks
    typedef std::function<void(const std::string&)> ComplianceEventCallback;
    void set_compliance_callback(ComplianceEventCallback callback);
    
    // Integration with encoder
    bool connect_encoder(std::shared_ptr<ModernMPEG2Encoder> encoder);
    bool connect_compliance_checker(std::shared_ptr<ATSCCompliance> compliance);
    
private:
    std::shared_ptr<ATSCCompliance> m_compliance_checker;
    std::shared_ptr<ModernMPEG2Encoder> m_encoder;
    
    // UI State
    struct UIState {
        // Compliance settings
        ATSCStandard selected_standard;
        VideoFormat video_format;
        AspectRatio aspect_ratio;
        double frame_rate;
        bool closed_captioning_enabled;
        bool audio_description_enabled;
        
        // PSIP data
        std::vector<VirtualChannelTable::Channel> channels;
        std::vector<EventInformationTable::Event> events;
        uint8_t rating_region;
        
        // Monitoring
        bool monitoring_active;
        ATSCCompliance::ComplianceStats current_stats;
        ATSCCompliance::ComplianceReport last_report;
        
        // UI preferences
        bool auto_refresh_enabled;
        int refresh_interval_ms;
        bool show_advanced_options;
        bool enable_notifications;
    };
    
    UIState m_ui_state;
    
    // Property creation helpers
    void add_compliance_standard_properties(obs_properties* props);
    void add_video_format_properties(obs_properties* props);
    void add_audio_format_properties(obs_properties* props);
    void add_psip_table_properties(obs_properties* props);
    void add_monitoring_properties(obs_properties* props);
    
    // Channel management
    void add_channel_properties(obs_properties* props);
    bool add_new_channel(obs_data* settings);
    bool edit_channel(int index, obs_data* settings);
    bool delete_channel(int index);
    
    // Event management
    void add_event_properties(obs_properties* props);
    bool add_new_event(obs_data* settings);
    bool edit_event(int index, obs_data* settings);
    bool delete_event(int index);
    
    // Real-time updates
    void start_ui_update_timer();
    void stop_ui_update_timer();
    void ui_update_callback();
    
    // Validation helpers
    bool validate_channel_data(const VirtualChannelTable::Channel& channel);
    bool validate_event_data(const EventInformationTable::Event& event);
    bool validate_compliance_configuration();
    
    // UI callbacks
    ComplianceEventCallback m_compliance_callback;
    
    // Error handling
    void show_error_message(const std::string& title, const std::string& message);
    void show_warning_message(const std::string& title, const std::string& message);
    void show_info_message(const std::string& title, const std::string& message);
    
    // File operations
    bool import_psip_data(const std::string& filename);
    bool export_psip_data(const std::string& filename);
    bool import_channel_list(const std::string& filename);
    bool export_channel_list(const std::string& filename);
    
    // Property update triggers
    static bool on_standard_changed(obs_properties* props, obs_property* property, obs_data* settings);
    static bool on_video_format_changed(obs_properties* props, obs_property* property, obs_data* settings);
    static bool on_add_channel_clicked(obs_properties* props, obs_property* property, void* data);
    static bool on_edit_channel_clicked(obs_properties* props, obs_property* property, void* data);
    static bool on_delete_channel_clicked(obs_properties* props, obs_property* property, void* data);
    static bool on_add_event_clicked(obs_properties* props, obs_property* property, void* data);
    static bool on_import_psip_clicked(obs_properties* props, obs_property* property, void* data);
    static bool on_export_psip_clicked(obs_properties* props, obs_property* property, void* data);
    static bool on_start_monitoring_clicked(obs_properties* props, obs_property* property, void* data);
    static bool on_stop_monitoring_clicked(obs_properties* props, obs_property* property, void* data);
    static bool on_generate_report_clicked(obs_properties* props, obs_property* property, void* data);
    static bool on_validate_compliance_clicked(obs_properties* props, obs_property* property, void* data);
};

// PSIP Table Editor Widget
class PSIPTableEditor {
public:
    PSIPTableEditor();
    ~PSIPTableEditor();
    
    // Master Guide Table Editor
    obs_properties* create_mgt_editor();
    bool update_mgt_from_ui(obs_data* settings, MasterGuideTable& mgt);
    
    // Virtual Channel Table Editor
    obs_properties* create_vct_editor();
    bool update_vct_from_ui(obs_data* settings, VirtualChannelTable& vct);
    
    // Event Information Table Editor
    obs_properties* create_eit_editor();
    bool update_eit_from_ui(obs_data* settings, EventInformationTable& eit);
    
    // System Time Table Editor
    obs_properties* create_stt_editor();
    bool update_stt_from_ui(obs_data* settings, SystemTimeTable& stt);
    
    // Rating Region Table Editor
    obs_properties* create_rrt_editor();
    bool update_rrt_from_ui(obs_data* settings, RatingRegionTable& rrt);
    
private:
    // Helper methods for table-specific UI
    void add_table_header_properties(obs_properties* props, const std::string& table_name);
    void add_descriptor_properties(obs_properties* props, const std::string& prefix);
    
    // Validation
    bool validate_table_id(uint16_t table_id, const std::string& expected_table);
    bool validate_section_syntax(uint8_t section_number, uint8_t last_section_number);
    
    // Time conversion helpers
    uint32_t ui_time_to_gps_time(const std::string& time_str);
    std::string gps_time_to_ui_time(uint32_t gps_time);
};

// Real-time Compliance Monitor Widget
class ComplianceMonitorWidget {
public:
    ComplianceMonitorWidget();
    ~ComplianceMonitorWidget();
    
    obs_properties* create_monitor_display();
    void update_compliance_status(const ATSCCompliance::ComplianceReport& report);
    void update_statistics(const ATSCCompliance::ComplianceStats& stats);
    
    // Visual indicators
    void set_compliance_indicator(bool compliant);
    void set_video_compliance_indicator(bool compliant);
    void set_audio_compliance_indicator(bool compliant);
    void set_psip_compliance_indicator(bool compliant);
    void set_timing_compliance_indicator(bool compliant);
    
    // Alert system
    bool configure_alerts(bool enabled);
    void trigger_compliance_alert(const std::string& message);
    
private:
    struct MonitorState {
        bool overall_compliant;
        bool video_compliant;
        bool audio_compliant;
        bool psip_compliant;
        bool timing_compliant;
        bool cc_compliant;
        
        uint64_t total_packets;
        uint64_t compliant_packets;
        double compliance_percentage;
        
        std::vector<std::string> recent_errors;
        std::vector<std::string> recent_warnings;
    };
    
    MonitorState m_monitor_state;
    
    // UI elements
    void create_status_indicators(obs_properties* props);
    void create_statistics_display(obs_properties* props);
    void create_error_log_display(obs_properties* props);
    
    // Alert management
    bool m_alerts_enabled;
    std::chrono::steady_clock::time_point m_last_alert_time;
    void show_compliance_alert(const std::string& title, const std::string& message);
};

// Channel Manager Widget
class ChannelManagerWidget {
public:
    ChannelManagerWidget();
    ~ChannelManagerWidget();
    
    obs_properties* create_channel_manager();
    void populate_channel_list(const std::vector<VirtualChannelTable::Channel>& channels);
    
    // Channel operations
    bool add_channel(const VirtualChannelTable::Channel& channel);
    bool edit_channel(int index, const VirtualChannelTable::Channel& channel);
    bool delete_channel(int index);
    bool move_channel(int from_index, int to_index);
    
    // Batch operations
    bool import_channels_from_file(const std::string& filename);
    bool export_channels_to_file(const std::string& filename);
    bool clear_all_channels();
    
    // Validation
    bool validate_channel_numbers();
    bool check_duplicate_channels();
    
private:
    std::vector<VirtualChannelTable::Channel> m_channels;
    
    // UI helpers
    void create_channel_list_display(obs_properties* props);
    void create_channel_editor_form(obs_properties* props);
    void create_batch_operation_buttons(obs_properties* props);
    
    // Channel validation
    bool is_valid_major_channel_number(uint16_t major);
    bool is_valid_minor_channel_number(uint16_t minor);
    bool is_channel_number_unique(uint16_t major, uint16_t minor, int exclude_index = -1);
    
    // File format support
    bool load_channels_from_json(const std::string& filename);
    bool save_channels_to_json(const std::string& filename);
    bool load_channels_from_xml(const std::string& filename);
    bool save_channels_to_xml(const std::string& filename);
};