#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>

struct InputConfig {
    std::string type;           // "file", "udp", "rtmp", "obs"
    std::string url;            // Input source URL/path
    std::string codec;          // Video codec (h264, h265, etc.)
    int bitrate;                // Target bitrate in kbps
    int width;                  // Video width
    int height;                 // Video height
    double fps;                 // Frame rate
    bool enabled;               // Whether this input is enabled
    std::map<std::string, std::string> metadata;
};

struct OutputConfig {
    std::string type;           // "udp", "rtmp", "file", "qam", "atsc"
    std::string url;            // Output destination URL/path
    std::string mux_format;     // Container format (ts, mp4, etc.)
    int bitrate;                // Total output bitrate in kbps
    std::vector<std::string> programs; // Program names to include
    std::map<std::string, std::string> parameters;
};

struct StatMuxConfig {
    int buffer_size_ms;         // Buffer size in milliseconds
    int update_interval_ms;     // Bitrate update interval
    double min_bitrate_factor;  // Minimum bitrate as factor of target
    double max_bitrate_factor;  // Maximum bitrate as factor of target
    bool adaptive_gop;          // Enable adaptive GOP sizing
    bool scene_change_detection; // Enable scene change detection
    int max_concurrent_streams; // Maximum concurrent input streams
};

struct OBSConfig {
    bool enabled;               // Enable OBS integration
    std::string obs_websocket_url; // OBS WebSocket URL
    std::string obs_websocket_password; // OBS WebSocket password
    int obs_port;               // OBS plugin port
    std::vector<std::string> scene_names; // Scene names to monitor
};

struct PlaylistItemConfig {
    std::string id;                 // Unique identifier for playlist item
    std::string title;              // Human readable title
    std::string type;               // file, live, slate, promo, ad
    std::string source;             // Path or stream URL
    std::string category;           // Category for traffic/accounting
    std::string scheduled_start;    // Scheduled start time (HH:MM[:SS])
    int expected_duration_sec;      // Planned duration in seconds
    bool mandatory;                 // Must play even if out of schedule
    std::map<std::string, std::string> metadata; // Arbitrary metadata
};

struct PlaylistConfig {
    bool enabled;                       // Enable playlist driven playout
    std::string rotation_mode;          // sequential, loop, weighted
    bool allow_live_insertions;         // Allow live events to interrupt
    std::vector<PlaylistItemConfig> items; // Playlist items
};

struct TrafficWindowConfig {
    std::string id;                 // Break identifier
    std::string start_time;         // Start time of traffic window
    int max_duration_sec;           // Maximum allowed duration in seconds
    std::vector<std::string> categories; // Allowed ad categories
};

struct TrafficAccountingConfig {
    bool enabled;                       // Enable traffic accounting
    std::string log_path;               // Location to write traffic logs
    bool auto_reconcile;                // Auto reconcile played vs scheduled
    std::vector<TrafficWindowConfig> windows; // Scheduled breaks
};

struct LiveSlotConfig {
    std::string id;                 // Unique slot identifier
    std::string source;             // Associated live input source
    std::string start_time;         // Scheduled start time (RFC3339 or HH:MM)
    std::string end_time;           // Scheduled end time
    std::string recurrence;         // Recurrence rule (daily, weekly, once)
    std::map<std::string, std::string> metadata; // Additional metadata
};

struct LiveSchedulingConfig {
    bool enabled;                   // Enable live source scheduling
    int pre_roll_seconds;           // Seconds to pre-roll before live start
    int post_roll_seconds;          // Seconds to keep live after scheduled end
    std::vector<LiveSlotConfig> slots; // Configured live slots
};

struct PlatformOutputConfig {
    std::string id;                 // Platform identifier
    std::string platform;           // Platform name (atsc, ott, fast, etc)
    bool enabled;                   // Should this platform be active
    OutputConfig output;            // Base output configuration
    std::map<std::string, std::string> metadata; // Platform specific settings
};

struct MultiPlatformConfig {
    bool enabled;                       // Enable multi-platform routing
    bool sync_playlists;                // Share playlist state across outputs
    std::vector<PlatformOutputConfig> platforms; // Configured platforms
};

class Config {
public:
    Config();
    ~Config();
    
    // Load configuration from file
    bool loadFromFile(const std::string& filename);
    
    // Load default configuration
    void loadDefault();
    
    // Save configuration to file
    bool saveToFile(const std::string& filename) const;
    
    // Getters
    const std::vector<InputConfig>& getInputs() const { return m_inputs; }
    const std::vector<OutputConfig>& getOutputs() const { return m_outputs; }
    const StatMuxConfig& getStatMuxConfig() const { return m_statmux; }
    const OBSConfig& getOBSConfig() const { return m_obs; }
    const PlaylistConfig& getPlaylistConfig() const { return m_playlist; }
    const TrafficAccountingConfig& getTrafficAccountingConfig() const { return m_traffic; }
    const LiveSchedulingConfig& getLiveSchedulingConfig() const { return m_liveScheduling; }
    const MultiPlatformConfig& getMultiPlatformConfig() const { return m_multiPlatform; }

    // Setters
    void addInput(const InputConfig& input);
    void addOutput(const OutputConfig& output);
    void setStatMuxConfig(const StatMuxConfig& config);
    void setOBSConfig(const OBSConfig& config);
    void setPlaylistConfig(const PlaylistConfig& config);
    void setTrafficAccountingConfig(const TrafficAccountingConfig& config);
    void setLiveSchedulingConfig(const LiveSchedulingConfig& config);
    void setMultiPlatformConfig(const MultiPlatformConfig& config);

    void addPlaylistItem(const PlaylistItemConfig& item);
    void addTrafficWindow(const TrafficWindowConfig& window);
    void addLiveSlot(const LiveSlotConfig& slot);
    void addPlatform(const PlatformOutputConfig& platform);

    // Utility methods
    std::string getValue(const std::string& key, const std::string& defaultValue = "") const;
    int getIntValue(const std::string& key, int defaultValue = 0) const;
    double getDoubleValue(const std::string& key, double defaultValue = 0.0) const;
    bool getBoolValue(const std::string& key, bool defaultValue = false) const;
    
    void setValue(const std::string& key, const std::string& value);
    void setIntValue(const std::string& key, int value);
    void setDoubleValue(const std::string& key, double value);
    void setBoolValue(const std::string& key, bool value);

private:
    std::vector<InputConfig> m_inputs;
    std::vector<OutputConfig> m_outputs;
    StatMuxConfig m_statmux;
    OBSConfig m_obs;
    PlaylistConfig m_playlist;
    TrafficAccountingConfig m_traffic;
    LiveSchedulingConfig m_liveScheduling;
    MultiPlatformConfig m_multiPlatform;
    std::map<std::string, std::string> m_keyValues;

    // Helper methods for configuration
    void createDefaultConfig();
    void applyParsedValues();
    static std::vector<std::string> splitList(const std::string& value);
};