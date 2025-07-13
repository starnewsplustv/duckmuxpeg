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
    
    // Setters
    void addInput(const InputConfig& input);
    void addOutput(const OutputConfig& output);
    void setStatMuxConfig(const StatMuxConfig& config);
    void setOBSConfig(const OBSConfig& config);
    
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
    std::map<std::string, std::string> m_keyValues;
    
    // Helper methods for configuration
    void createDefaultConfig();
};