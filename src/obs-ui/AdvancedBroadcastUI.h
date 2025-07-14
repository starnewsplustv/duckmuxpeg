#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>

#include "BroadcastComplianceUI.h"
#include "broadcast/ATSCCompliance.h"
#include "encoder/ModernMPEG2Encoder.h"
#include "statmux/StatMuxEngine.h"
#include "common/Config.h"

// Forward declarations
struct obs_properties;
struct obs_data;
struct obs_source;

// ASI (Advanced Stream Interface) Configuration
struct ASIConfig {
    std::string device_path;         // "/dev/dtapi0"
    int port_number;                 // 0-3 for 4-port card
    int bitrate;                     // Target bitrate in bps
    std::string output_format;       // "ATSC", "QAM", "DVB-S", "DVB-T"
    bool enable_asi_output;
    std::string asi_mode;            // "burst", "continuous", "gapped"
    int asi_clock_rate;              // 270000000 for SMPTE 310M
    bool enable_stuffing;            // Add null packets for constant bitrate
};

// Social Media Streaming Configuration
struct SocialMediaConfig {
    std::string platform;           // "youtube", "facebook", "twitter", "twitch", "instagram"
    std::string stream_key;
    std::string server_url;
    std::string encoder_preset;     // "ultrafast", "superfast", "veryfast", "faster", "fast", "medium", "slow", "slower", "veryslow"
    int h264_bitrate;              // H.264 bitrate in kbps
    std::string h264_profile;      // "baseline", "main", "high"
    std::string h264_level;        // "3.1", "4.0", "4.1", "4.2", "5.0", "5.1", "5.2"
    int width;
    int height;
    double framerate;
    bool enable_audio;
    int audio_bitrate;
    std::string audio_codec;       // "aac", "opus"
    bool enable_stream;
};

// Experimental StatMux Feeding Methods
enum class StatMuxMethod {
    TRADITIONAL,                   // Traditional CBR/VBR
    X264_LOOKAHEAD,               // Use x264 lookahead data
    SCENE_AWARE,                  // Scene change detection
    CONTENT_COMPLEXITY,           // Content complexity analysis
    MOTION_ADAPTIVE,              // Motion-based allocation
    HYBRID_ANALYSIS,              // Combine multiple methods
    AI_PREDICTED,                 // AI-based prediction (future)
    VIEWER_DRIVEN                 // Viewer engagement based (future)
};

// Scene Encoder Profile
struct SceneProfile {
    std::string scene_name;
    std::string profile_name;
    int priority_weight;           // 1-100, higher = more priority
    int min_bitrate;              // Minimum guaranteed bitrate
    int max_bitrate;              // Maximum allowed bitrate
    int target_bitrate;           // Target bitrate
    std::string encoder_preset;
    bool enable_scene_detection;
    double complexity_threshold;
    StatMuxMethod preferred_method;
    std::map<std::string, std::string> custom_params;
};

// Channel Priority Configuration
struct ChannelPriority {
    std::string channel_id;
    std::string channel_name;
    int priority_weight;          // 1-100
    int guaranteed_bitrate;       // Minimum guaranteed bitrate
    bool is_primary;              // Primary channel (highest priority)
    bool allow_quality_reduction; // Allow quality reduction under load
    std::string fallback_profile; // Fallback profile if bandwidth limited
};

// Commercial Block Configuration
struct CommercialBlock {
    std::string block_id;
    std::string block_name;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    int duration_seconds;
    std::string advertiser;
    std::string campaign_id;
    int target_bitrate;
    bool enable_overlay;
    std::string overlay_text;
    std::vector<std::string> associated_channels;
};

// Program Block Configuration
struct ProgramBlock {
    std::string program_id;
    std::string program_name;
    std::string program_type;     // "news", "sports", "entertainment", "educational"
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    int duration_seconds;
    std::string content_rating;   // "G", "PG", "PG-13", "R", "TV-Y", "TV-G", "TV-PG", "TV-14", "TV-MA"
    std::string description;
    std::vector<std::string> associated_channels;
    std::vector<CommercialBlock> commercial_blocks;
    int target_bitrate;
    SceneProfile preferred_profile;
    bool enable_closed_captioning;
    bool enable_audio_description;
};

// Report Configuration
struct ReportConfig {
    std::string report_type;      // "compliance", "performance", "commercial", "viewer", "quality"
    std::string output_format;    // "pdf", "html", "csv", "json"
    std::string output_path;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    bool auto_generate;
    int generation_interval_hours;
    std::vector<std::string> included_channels;
    std::vector<std::string> metrics;
    bool include_graphs;
    bool include_statistics;
    bool include_compliance_details;
};

// Advanced Broadcast UI Class
class AdvancedBroadcastUI : public BroadcastComplianceUI {
public:
    AdvancedBroadcastUI();
    ~AdvancedBroadcastUI();
    
    // Initialize with existing components
    bool initialize(std::shared_ptr<StatMuxEngine> statmux_engine,
                   std::shared_ptr<ModernMPEG2Encoder> encoder,
                   std::shared_ptr<ATSCCompliance> compliance);
    
    // =========================
    // ASI TRANSMITTER PANEL
    // =========================
    obs_properties* create_asi_properties();
    void update_asi_properties(obs_properties* props, obs_data* settings);
    bool configure_asi_port(int port, const ASIConfig& config);
    bool start_asi_transmission(int port);
    bool stop_asi_transmission(int port);
    std::vector<ASIConfig> get_asi_configurations();
    
    // =========================
    // SOCIAL MEDIA STREAMING PANEL
    // =========================
    obs_properties* create_social_media_properties();
    void update_social_media_properties(obs_properties* props, obs_data* settings);
    bool configure_social_platform(const std::string& platform, const SocialMediaConfig& config);
    bool start_social_streaming(const std::string& platform);
    bool stop_social_streaming(const std::string& platform);
    std::vector<SocialMediaConfig> get_social_configurations();
    
    // =========================
    // EXPERIMENTAL STATMUX PANEL
    // =========================
    obs_properties* create_statmux_properties();
    void update_statmux_properties(obs_properties* props, obs_data* settings);
    bool set_statmux_method(StatMuxMethod method);
    bool configure_x264_integration(bool enable, const std::string& x264_stats_file);
    bool enable_experimental_features(bool enable);
    StatMuxMethod get_current_statmux_method();
    
    // =========================
    // SCENE MANAGEMENT PANEL
    // =========================
    obs_properties* create_scene_properties();
    void update_scene_properties(obs_properties* props, obs_data* settings);
    bool add_scene_profile(const SceneProfile& profile);
    bool remove_scene_profile(const std::string& scene_name);
    bool update_scene_profile(const std::string& scene_name, const SceneProfile& profile);
    std::vector<SceneProfile> get_scene_profiles();
    
    // =========================
    // CHANNEL PRIORITY PANEL
    // =========================
    obs_properties* create_priority_properties();
    void update_priority_properties(obs_properties* props, obs_data* settings);
    bool set_channel_priority(const std::string& channel_id, const ChannelPriority& priority);
    bool rebalance_priorities();
    std::vector<ChannelPriority> get_channel_priorities();
    
    // =========================
    // PROGRAM SCHEDULING PANEL
    // =========================
    obs_properties* create_program_properties();
    void update_program_properties(obs_properties* props, obs_data* settings);
    bool add_program_block(const ProgramBlock& program);
    bool remove_program_block(const std::string& program_id);
    bool update_program_block(const std::string& program_id, const ProgramBlock& program);
    std::vector<ProgramBlock> get_program_schedule();
    
    // =========================
    // COMMERCIAL MANAGEMENT PANEL
    // =========================
    obs_properties* create_commercial_properties();
    void update_commercial_properties(obs_properties* props, obs_data* settings);
    bool add_commercial_block(const CommercialBlock& commercial);
    bool remove_commercial_block(const std::string& block_id);
    bool update_commercial_block(const std::string& block_id, const CommercialBlock& commercial);
    std::vector<CommercialBlock> get_commercial_schedule();
    
    // =========================
    // REPORT GENERATION PANEL
    // =========================
    obs_properties* create_report_properties();
    void update_report_properties(obs_properties* props, obs_data* settings);
    bool generate_report(const ReportConfig& config);
    bool schedule_automatic_reports(const ReportConfig& config);
    bool export_report_data(const std::string& format, const std::string& filename);
    std::vector<std::string> get_available_reports();
    
    // =========================
    // MONITORING & STATISTICS
    // =========================
    obs_properties* create_advanced_monitoring_properties();
    void update_monitoring_display() override;
    void refresh_statistics() override;
    
    // Integration callbacks
    void on_scene_change(const std::string& scene_name);
    void on_statmux_update(const std::vector<StreamStats>& stats);
    void on_asi_status_change(int port, bool active);
    void on_social_stream_status(const std::string& platform, bool active);
    
private:
    // Core engine references
    std::shared_ptr<StatMuxEngine> m_statmux_engine;
    
    // ASI Hardware Interface
    struct ASIPort {
        int port_number;
        ASIConfig config;
        std::atomic<bool> active;
        std::unique_ptr<std::thread> transmission_thread;
        std::atomic<uint64_t> packets_sent;
        std::atomic<uint64_t> bytes_sent;
        std::chrono::system_clock::time_point last_packet_time;
    };
    std::vector<ASIPort> m_asi_ports;
    std::mutex m_asi_mutex;
    
    // Social Media Streaming
    struct SocialStream {
        std::string platform;
        SocialMediaConfig config;
        std::atomic<bool> active;
        std::unique_ptr<std::thread> streaming_thread;
        std::atomic<uint64_t> frames_sent;
        std::atomic<uint64_t> bytes_sent;
        std::chrono::system_clock::time_point start_time;
    };
    std::vector<SocialStream> m_social_streams;
    std::mutex m_social_mutex;
    
    // StatMux Management
    StatMuxMethod m_current_statmux_method;
    bool m_x264_integration_enabled;
    std::string m_x264_stats_file;
    std::atomic<bool> m_experimental_features_enabled;
    
    // Scene Management
    std::vector<SceneProfile> m_scene_profiles;
    std::string m_current_scene;
    std::mutex m_scene_mutex;
    
    // Channel Priority Management
    std::vector<ChannelPriority> m_channel_priorities;
    std::mutex m_priority_mutex;
    
    // Program Scheduling
    std::vector<ProgramBlock> m_program_schedule;
    std::mutex m_program_mutex;
    
    // Commercial Management
    std::vector<CommercialBlock> m_commercial_schedule;
    std::mutex m_commercial_mutex;
    
    // Report Generation
    std::vector<ReportConfig> m_report_configs;
    std::mutex m_report_mutex;
    std::unique_ptr<std::thread> m_report_generator_thread;
    
    // Monitoring and Statistics
    struct AdvancedStats {
        // ASI Stats
        std::map<int, uint64_t> asi_packets_sent;
        std::map<int, uint64_t> asi_bytes_sent;
        std::map<int, bool> asi_port_status;
        
        // Social Media Stats
        std::map<std::string, uint64_t> social_frames_sent;
        std::map<std::string, uint64_t> social_bytes_sent;
        std::map<std::string, bool> social_stream_status;
        
        // StatMux Stats
        StatMuxMethod current_method;
        double statmux_efficiency;
        int total_allocated_bitrate;
        int total_available_bitrate;
        
        // Scene Stats
        std::string current_scene;
        int scene_changes_count;
        std::map<std::string, int> scene_duration_stats;
        
        // Channel Priority Stats
        std::map<std::string, int> channel_allocated_bitrates;
        std::map<std::string, double> channel_quality_scores;
        
        std::chrono::system_clock::time_point last_update;
    };
    AdvancedStats m_advanced_stats;
    std::mutex m_stats_mutex;
    
    // Private helper methods
    void initialize_asi_hardware();
    void initialize_social_streaming();
    void initialize_statmux_integration();
    void initialize_scene_management();
    void initialize_scheduling();
    void initialize_reporting();
    
    // ASI Hardware helpers
    bool detect_asi_hardware();
    bool configure_asi_hardware(int port, const ASIConfig& config);
    void asi_transmission_thread(int port);
    
    // Social Media helpers
    bool initialize_h264_encoder(const SocialMediaConfig& config);
    void social_streaming_thread(const std::string& platform);
    
    // StatMux helpers
    bool integrate_x264_data(const std::string& stats_file);
    void apply_statmux_method(StatMuxMethod method);
    void update_statmux_allocation();
    
    // Scene management helpers
    void apply_scene_profile(const std::string& scene_name);
    void update_scene_priorities();
    
    // Scheduling helpers
    void check_program_schedule();
    void check_commercial_schedule();
    void execute_scheduled_actions();
    
    // Report generation helpers
    void generate_compliance_report(const ReportConfig& config);
    void generate_performance_report(const ReportConfig& config);
    void generate_commercial_report(const ReportConfig& config);
    void generate_viewer_report(const ReportConfig& config);
    void generate_quality_report(const ReportConfig& config);
    void report_generator_thread();
    
    // UI callback handlers
    static bool on_asi_port_selected(obs_properties* props, obs_property* property, obs_data* settings);
    static bool on_asi_start_clicked(obs_properties* props, obs_property* property, void* data);
    static bool on_asi_stop_clicked(obs_properties* props, obs_property* property, void* data);
    static bool on_social_platform_selected(obs_properties* props, obs_property* property, obs_data* settings);
    static bool on_social_start_clicked(obs_properties* props, obs_property* property, void* data);
    static bool on_social_stop_clicked(obs_properties* props, obs_property* property, void* data);
    static bool on_statmux_method_changed(obs_properties* props, obs_property* property, obs_data* settings);
    static bool on_scene_profile_selected(obs_properties* props, obs_property* property, obs_data* settings);
    static bool on_add_scene_profile_clicked(obs_properties* props, obs_property* property, void* data);
    static bool on_channel_priority_changed(obs_properties* props, obs_property* property, obs_data* settings);
    static bool on_add_program_clicked(obs_properties* props, obs_property* property, void* data);
    static bool on_add_commercial_clicked(obs_properties* props, obs_property* property, void* data);
    static bool on_generate_report_clicked(obs_properties* props, obs_property* property, void* data);
    
    // Validation helpers
    bool validate_asi_config(const ASIConfig& config);
    bool validate_social_config(const SocialMediaConfig& config);
    bool validate_scene_profile(const SceneProfile& profile);
    bool validate_channel_priority(const ChannelPriority& priority);
    bool validate_program_block(const ProgramBlock& program);
    bool validate_commercial_block(const CommercialBlock& commercial);
    bool validate_report_config(const ReportConfig& config);
    
    // Configuration persistence
    bool save_configuration(const std::string& filename);
    bool load_configuration(const std::string& filename);
};