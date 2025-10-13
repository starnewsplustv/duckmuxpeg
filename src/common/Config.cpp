#include "Config.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace {

bool parseBool(const std::string& value, bool defaultValue = false) {
    std::string normalized = value;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    if (normalized == "true" || normalized == "1" || normalized == "yes" || normalized == "on") {
        return true;
    }
    if (normalized == "false" || normalized == "0" || normalized == "no" || normalized == "off") {
        return false;
    }
    return defaultValue;
}

int parseInt(const std::string& value, int defaultValue = 0) {
    try {
        return std::stoi(value);
    } catch (...) {
        return defaultValue;
    }
}

std::map<std::string, std::string> parseMetadata(const std::string& value) {
    std::map<std::string, std::string> metadata;
    for (const auto& token : Config::splitList(value)) {
        auto delimiter = token.find(':');
        if (delimiter != std::string::npos) {
            std::string key = token.substr(0, delimiter);
            std::string val = token.substr(delimiter + 1);
            metadata[key] = val;
        }
    }
    return metadata;
}

}

Config::Config() {
    createDefaultConfig();
}

Config::~Config() {
}

bool Config::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        Logger::error("Failed to open config file: " + filename);
        return false;
    }

    createDefaultConfig();

    // Simple JSON-like parsing (basic implementation)
    std::string line;
    std::string section = "";

    while (std::getline(file, line)) {
        // Remove whitespace
        line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
        
        if (line.empty() || line[0] == '#' || line[0] == '/') {
            continue;
        }
        
        // Section detection
        if (line.find('[') != std::string::npos && line.find(']') != std::string::npos) {
            section = line.substr(line.find('[') + 1, line.find(']') - line.find('[') - 1);
            continue;
        }
        
        // Key-value parsing
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // Remove quotes if present
            if (value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.length() - 2);
            }
            
            m_keyValues[section + "." + key] = value;
        }
    }

    file.close();

    applyParsedValues();

    Logger::info("Configuration loaded from: " + filename);
    return true;
}

void Config::loadDefault() {
    createDefaultConfig();
    Logger::info("Default configuration loaded");
}

bool Config::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        Logger::error("Failed to create config file: " + filename);
        return false;
    }
    
    file << "# DuckMuxPeg Configuration File\n";
    file << "# Generated automatically\n\n";
    
    // Write StatMux configuration
    file << "[statmux]\n";
    file << "buffer_size_ms=" << m_statmux.buffer_size_ms << "\n";
    file << "update_interval_ms=" << m_statmux.update_interval_ms << "\n";
    file << "min_bitrate_factor=" << m_statmux.min_bitrate_factor << "\n";
    file << "max_bitrate_factor=" << m_statmux.max_bitrate_factor << "\n";
    file << "adaptive_gop=" << (m_statmux.adaptive_gop ? "true" : "false") << "\n";
    file << "scene_change_detection=" << (m_statmux.scene_change_detection ? "true" : "false") << "\n";
    file << "max_concurrent_streams=" << m_statmux.max_concurrent_streams << "\n\n";
    
    // Write OBS configuration
    file << "[obs]\n";
    file << "enabled=" << (m_obs.enabled ? "true" : "false") << "\n";
    file << "obs_websocket_url=\"" << m_obs.obs_websocket_url << "\"\n";
    file << "obs_websocket_password=\"" << m_obs.obs_websocket_password << "\"\n";
    file << "obs_port=" << m_obs.obs_port << "\n\n";

    // Write playlist configuration
    file << "[playlist]\n";
    file << "enabled=" << (m_playlist.enabled ? "true" : "false") << "\n";
    file << "rotation_mode=\"" << m_playlist.rotation_mode << "\"\n";
    file << "allow_live_insertions=" << (m_playlist.allow_live_insertions ? "true" : "false") << "\n\n";

    auto joinMetadata = [](const std::map<std::string, std::string>& metadata) {
        std::ostringstream oss;
        bool first = true;
        for (const auto& entry : metadata) {
            if (!first) {
                oss << ",";
            }
            oss << entry.first << ":" << entry.second;
            first = false;
        }
        return oss.str();
    };

    for (size_t i = 0; i < m_playlist.items.size(); ++i) {
        const auto& item = m_playlist.items[i];
        file << "[playlist_item" << i << "]\n";
        file << "id=\"" << item.id << "\"\n";
        file << "title=\"" << item.title << "\"\n";
        file << "type=\"" << item.type << "\"\n";
        file << "source=\"" << item.source << "\"\n";
        file << "category=\"" << item.category << "\"\n";
        file << "scheduled_start=\"" << item.scheduled_start << "\"\n";
        file << "expected_duration_sec=" << item.expected_duration_sec << "\n";
        file << "mandatory=" << (item.mandatory ? "true" : "false") << "\n";
        if (!item.metadata.empty()) {
            file << "metadata=\"" << joinMetadata(item.metadata) << "\"\n";
        }
        file << "\n";
    }

    // Traffic accounting configuration
    file << "[traffic]\n";
    file << "enabled=" << (m_traffic.enabled ? "true" : "false") << "\n";
    file << "log_path=\"" << m_traffic.log_path << "\"\n";
    file << "auto_reconcile=" << (m_traffic.auto_reconcile ? "true" : "false") << "\n\n";

    for (size_t i = 0; i < m_traffic.windows.size(); ++i) {
        const auto& window = m_traffic.windows[i];
        file << "[traffic_window" << i << "]\n";
        file << "id=\"" << window.id << "\"\n";
        file << "start_time=\"" << window.start_time << "\"\n";
        file << "max_duration_sec=" << window.max_duration_sec << "\n";
        if (!window.categories.empty()) {
            std::ostringstream oss;
            for (size_t j = 0; j < window.categories.size(); ++j) {
                if (j > 0) oss << ",";
                oss << window.categories[j];
            }
            file << "categories=\"" << oss.str() << "\"\n";
        }
        file << "\n";
    }

    // Live scheduling configuration
    file << "[live_scheduling]\n";
    file << "enabled=" << (m_liveScheduling.enabled ? "true" : "false") << "\n";
    file << "pre_roll_seconds=" << m_liveScheduling.pre_roll_seconds << "\n";
    file << "post_roll_seconds=" << m_liveScheduling.post_roll_seconds << "\n\n";

    for (size_t i = 0; i < m_liveScheduling.slots.size(); ++i) {
        const auto& slot = m_liveScheduling.slots[i];
        file << "[live_slot" << i << "]\n";
        file << "id=\"" << slot.id << "\"\n";
        file << "source=\"" << slot.source << "\"\n";
        file << "start_time=\"" << slot.start_time << "\"\n";
        file << "end_time=\"" << slot.end_time << "\"\n";
        file << "recurrence=\"" << slot.recurrence << "\"\n";
        if (!slot.metadata.empty()) {
            file << "metadata=\"" << joinMetadata(slot.metadata) << "\"\n";
        }
        file << "\n";
    }

    // Multi-platform configuration
    file << "[multi_platform]\n";
    file << "enabled=" << (m_multiPlatform.enabled ? "true" : "false") << "\n";
    file << "sync_playlists=" << (m_multiPlatform.sync_playlists ? "true" : "false") << "\n\n";

    for (size_t i = 0; i < m_multiPlatform.platforms.size(); ++i) {
        const auto& platform = m_multiPlatform.platforms[i];
        file << "[platform" << i << "]\n";
        file << "id=\"" << platform.id << "\"\n";
        file << "platform=\"" << platform.platform << "\"\n";
        file << "enabled=" << (platform.enabled ? "true" : "false") << "\n";
        file << "output_type=\"" << platform.output.type << "\"\n";
        file << "output_url=\"" << platform.output.url << "\"\n";
        file << "output_mux_format=\"" << platform.output.mux_format << "\"\n";
        file << "output_bitrate=" << platform.output.bitrate << "\n";
        if (!platform.output.programs.empty()) {
            std::ostringstream oss;
            for (size_t j = 0; j < platform.output.programs.size(); ++j) {
                if (j > 0) oss << ",";
                oss << platform.output.programs[j];
            }
            file << "output_programs=\"" << oss.str() << "\"\n";
        }
        if (!platform.metadata.empty()) {
            file << "metadata=\"" << joinMetadata(platform.metadata) << "\"\n";
        }
        file << "\n";
    }

    // Write input configurations
    for (size_t i = 0; i < m_inputs.size(); ++i) {
        const auto& input = m_inputs[i];
        file << "[input" << i << "]\n";
        file << "type=\"" << input.type << "\"\n";
        file << "url=\"" << input.url << "\"\n";
        file << "codec=\"" << input.codec << "\"\n";
        file << "bitrate=" << input.bitrate << "\n";
        file << "width=" << input.width << "\n";
        file << "height=" << input.height << "\n";
        file << "fps=" << input.fps << "\n";
        file << "enabled=" << (input.enabled ? "true" : "false") << "\n\n";
    }
    
    // Write output configurations
    for (size_t i = 0; i < m_outputs.size(); ++i) {
        const auto& output = m_outputs[i];
        file << "[output" << i << "]\n";
        file << "type=\"" << output.type << "\"\n";
        file << "url=\"" << output.url << "\"\n";
        file << "mux_format=\"" << output.mux_format << "\"\n";
        file << "bitrate=" << output.bitrate << "\n\n";
    }
    
    file.close();
    Logger::info("Configuration saved to: " + filename);
    return true;
}

void Config::addInput(const InputConfig& input) {
    m_inputs.push_back(input);
}

void Config::addOutput(const OutputConfig& output) {
    m_outputs.push_back(output);
}

void Config::setStatMuxConfig(const StatMuxConfig& config) {
    m_statmux = config;
}

void Config::setOBSConfig(const OBSConfig& config) {
    m_obs = config;
}

void Config::setPlaylistConfig(const PlaylistConfig& config) {
    m_playlist = config;
}

void Config::setTrafficAccountingConfig(const TrafficAccountingConfig& config) {
    m_traffic = config;
}

void Config::setLiveSchedulingConfig(const LiveSchedulingConfig& config) {
    m_liveScheduling = config;
}

void Config::setMultiPlatformConfig(const MultiPlatformConfig& config) {
    m_multiPlatform = config;
}

void Config::addPlaylistItem(const PlaylistItemConfig& item) {
    m_playlist.items.push_back(item);
}

void Config::addTrafficWindow(const TrafficWindowConfig& window) {
    m_traffic.windows.push_back(window);
}

void Config::addLiveSlot(const LiveSlotConfig& slot) {
    m_liveScheduling.slots.push_back(slot);
}

void Config::addPlatform(const PlatformOutputConfig& platform) {
    m_multiPlatform.platforms.push_back(platform);
}

std::string Config::getValue(const std::string& key, const std::string& defaultValue) const {
    auto it = m_keyValues.find(key);
    return (it != m_keyValues.end()) ? it->second : defaultValue;
}

int Config::getIntValue(const std::string& key, int defaultValue) const {
    auto it = m_keyValues.find(key);
    if (it != m_keyValues.end()) {
        try {
            return std::stoi(it->second);
        } catch (...) {
            // Fall through to default
        }
    }
    return defaultValue;
}

double Config::getDoubleValue(const std::string& key, double defaultValue) const {
    auto it = m_keyValues.find(key);
    if (it != m_keyValues.end()) {
        try {
            return std::stod(it->second);
        } catch (...) {
            // Fall through to default
        }
    }
    return defaultValue;
}

bool Config::getBoolValue(const std::string& key, bool defaultValue) const {
    auto it = m_keyValues.find(key);
    if (it != m_keyValues.end()) {
        std::string value = it->second;
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        return (value == "true" || value == "1" || value == "yes" || value == "on");
    }
    return defaultValue;
}

void Config::setValue(const std::string& key, const std::string& value) {
    m_keyValues[key] = value;
}

void Config::setIntValue(const std::string& key, int value) {
    m_keyValues[key] = std::to_string(value);
}

void Config::setDoubleValue(const std::string& key, double value) {
    m_keyValues[key] = std::to_string(value);
}

void Config::setBoolValue(const std::string& key, bool value) {
    m_keyValues[key] = value ? "true" : "false";
}



void Config::createDefaultConfig() {
    // Clear existing config
    m_inputs.clear();
    m_outputs.clear();
    m_keyValues.clear();
    
    // Default StatMux configuration
    m_statmux = {
        .buffer_size_ms = 1000,
        .update_interval_ms = 100,
        .min_bitrate_factor = 0.5,
        .max_bitrate_factor = 1.5,
        .adaptive_gop = true,
        .scene_change_detection = true,
        .max_concurrent_streams = 16
    };
    
    // Default OBS configuration
    m_obs = {
        .enabled = false,
        .obs_websocket_url = "ws://localhost:4444",
        .obs_websocket_password = "",
        .obs_port = 9999,
        .scene_names = {"Scene 1", "Scene 2"}
    };
    
    // Default input (example)
    InputConfig defaultInput = {
        .type = "file",
        .url = "test.mp4",
        .codec = "h264",
        .bitrate = 2000,
        .width = 1920,
        .height = 1080,
        .fps = 30.0,
        .enabled = true
    };
    m_inputs.push_back(defaultInput);
    
    // Default output (example)
    OutputConfig defaultOutput = {
        .type = "udp",
        .url = "udp://239.0.0.1:1234",
        .mux_format = "ts",
        .bitrate = 8000,
        .programs = {"program1"}
    };
    m_outputs.push_back(defaultOutput);

    // Default playlist configuration
    m_playlist = {
        .enabled = true,
        .rotation_mode = "sequential",
        .allow_live_insertions = true,
        .items = {
            PlaylistItemConfig{
                .id = "default_program",
                .title = "Default Programming Block",
                .type = "file",
                .source = "test.mp4",
                .category = "program",
                .scheduled_start = "00:00:00",
                .expected_duration_sec = 1800,
                .mandatory = true,
                .metadata = {{"rating", "TV-G"}}
            },
            PlaylistItemConfig{
                .id = "default_ad_break",
                .title = "Generic Ad Break",
                .type = "file",
                .source = "ads/default_ad.ts",
                .category = "advertisement",
                .scheduled_start = "00:30:00",
                .expected_duration_sec = 120,
                .mandatory = false,
                .metadata = {{"pod", "A"}}
            }
        }
    };

    // Default traffic accounting configuration
    m_traffic = {
        .enabled = true,
        .log_path = "logs/traffic.log",
        .auto_reconcile = true,
        .windows = {
            TrafficWindowConfig{
                .id = "primetime_break",
                .start_time = "00:30:00",
                .max_duration_sec = 120,
                .categories = {"advertisement", "promo"}
            }
        }
    };

    // Default live scheduling configuration
    m_liveScheduling = {
        .enabled = true,
        .pre_roll_seconds = 60,
        .post_roll_seconds = 15,
        .slots = {
            LiveSlotConfig{
                .id = "morning_news",
                .source = "rtmp://news-room/live",
                .start_time = "06:00:00",
                .end_time = "07:00:00",
                .recurrence = "daily",
                .metadata = {{"anchor", "Alex"}}
            }
        }
    };

    // Default multi-platform configuration
    m_multiPlatform = {
        .enabled = true,
        .sync_playlists = true,
        .platforms = {
            PlatformOutputConfig{
                .id = "atsc_airchain",
                .platform = "atsc",
                .enabled = true,
                .output = {
                    .type = "atsc",
                    .url = "qam://frequency=573000000",
                    .mux_format = "ts",
                    .bitrate = 19000,
                    .programs = {"main"},
                    .parameters = {}
                },
                .metadata = {{"psip", "primary"}}
            },
            PlatformOutputConfig{
                .id = "ott_linear",
                .platform = "ott",
                .enabled = true,
                .output = {
                    .type = "rtmp",
                    .url = "rtmp://ott-platform/live/duckmuxpeg",
                    .mux_format = "flv",
                    .bitrate = 8000,
                    .programs = {"main"},
                    .parameters = {{"profile", "main"}}
                },
                .metadata = {{"drm", "widevine"}}
            }
        }
    };
}

void Config::applyParsedValues() {
    std::map<std::string, std::map<std::string, std::string>> sections;
    for (const auto& kv : m_keyValues) {
        auto delimiter = kv.first.find('.');
        if (delimiter == std::string::npos) continue;
        std::string section = kv.first.substr(0, delimiter);
        std::string key = kv.first.substr(delimiter + 1);
        sections[section][key] = kv.second;
    }

    auto statmuxIt = sections.find("statmux");
    if (statmuxIt != sections.end()) {
        const auto& values = statmuxIt->second;
        if (auto it = values.find("buffer_size_ms"); it != values.end()) {
            m_statmux.buffer_size_ms = parseInt(it->second, m_statmux.buffer_size_ms);
        }
        if (auto it = values.find("update_interval_ms"); it != values.end()) {
            m_statmux.update_interval_ms = parseInt(it->second, m_statmux.update_interval_ms);
        }
        if (auto it = values.find("min_bitrate_factor"); it != values.end()) {
            try { m_statmux.min_bitrate_factor = std::stod(it->second); } catch (...) {}
        }
        if (auto it = values.find("max_bitrate_factor"); it != values.end()) {
            try { m_statmux.max_bitrate_factor = std::stod(it->second); } catch (...) {}
        }
        if (auto it = values.find("adaptive_gop"); it != values.end()) {
            m_statmux.adaptive_gop = parseBool(it->second, m_statmux.adaptive_gop);
        }
        if (auto it = values.find("scene_change_detection"); it != values.end()) {
            m_statmux.scene_change_detection = parseBool(it->second, m_statmux.scene_change_detection);
        }
        if (auto it = values.find("max_concurrent_streams"); it != values.end()) {
            m_statmux.max_concurrent_streams = parseInt(it->second, m_statmux.max_concurrent_streams);
        }
    }

    auto obsIt = sections.find("obs");
    if (obsIt != sections.end()) {
        const auto& values = obsIt->second;
        if (auto it = values.find("enabled"); it != values.end()) {
            m_obs.enabled = parseBool(it->second, m_obs.enabled);
        }
        if (auto it = values.find("obs_websocket_url"); it != values.end()) {
            m_obs.obs_websocket_url = it->second;
        }
        if (auto it = values.find("obs_websocket_password"); it != values.end()) {
            m_obs.obs_websocket_password = it->second;
        }
        if (auto it = values.find("obs_port"); it != values.end()) {
            m_obs.obs_port = parseInt(it->second, m_obs.obs_port);
        }
    }

    // Playlist
    auto playlistIt = sections.find("playlist");
    if (playlistIt != sections.end()) {
        const auto& values = playlistIt->second;
        if (auto it = values.find("enabled"); it != values.end()) {
            m_playlist.enabled = parseBool(it->second, m_playlist.enabled);
        }
        if (auto it = values.find("rotation_mode"); it != values.end()) {
            m_playlist.rotation_mode = it->second;
        }
        if (auto it = values.find("allow_live_insertions"); it != values.end()) {
            m_playlist.allow_live_insertions = parseBool(it->second, m_playlist.allow_live_insertions);
        }
    }

    std::vector<PlaylistItemConfig> parsedItems;
    for (const auto& sectionEntry : sections) {
        if (sectionEntry.first.rfind("playlist_item", 0) == 0) {
            PlaylistItemConfig item;
            const auto& values = sectionEntry.second;
            if (auto it = values.find("id"); it != values.end()) item.id = it->second;
            if (auto it = values.find("title"); it != values.end()) item.title = it->second;
            if (auto it = values.find("type"); it != values.end()) item.type = it->second;
            if (auto it = values.find("source"); it != values.end()) item.source = it->second;
            if (auto it = values.find("category"); it != values.end()) item.category = it->second;
            if (auto it = values.find("scheduled_start"); it != values.end()) item.scheduled_start = it->second;
            if (auto it = values.find("expected_duration_sec"); it != values.end()) item.expected_duration_sec = parseInt(it->second, 0);
            if (auto it = values.find("mandatory"); it != values.end()) item.mandatory = parseBool(it->second, false);
            if (auto it = values.find("metadata"); it != values.end()) item.metadata = parseMetadata(it->second);
            if (!item.id.empty()) {
                parsedItems.push_back(item);
            }
        }
    }
    if (!parsedItems.empty()) {
        m_playlist.items = parsedItems;
    }

    // Traffic accounting
    auto trafficIt = sections.find("traffic");
    if (trafficIt != sections.end()) {
        const auto& values = trafficIt->second;
        if (auto it = values.find("enabled"); it != values.end()) m_traffic.enabled = parseBool(it->second, m_traffic.enabled);
        if (auto it = values.find("log_path"); it != values.end()) m_traffic.log_path = it->second;
        if (auto it = values.find("auto_reconcile"); it != values.end()) m_traffic.auto_reconcile = parseBool(it->second, m_traffic.auto_reconcile);
    }

    std::vector<TrafficWindowConfig> parsedWindows;
    for (const auto& sectionEntry : sections) {
        if (sectionEntry.first.rfind("traffic_window", 0) == 0) {
            TrafficWindowConfig window;
            const auto& values = sectionEntry.second;
            if (auto it = values.find("id"); it != values.end()) window.id = it->second;
            if (auto it = values.find("start_time"); it != values.end()) window.start_time = it->second;
            if (auto it = values.find("max_duration_sec"); it != values.end()) window.max_duration_sec = parseInt(it->second, 0);
            if (auto it = values.find("categories"); it != values.end()) window.categories = Config::splitList(it->second);
            if (!window.id.empty()) {
                parsedWindows.push_back(window);
            }
        }
    }
    if (!parsedWindows.empty()) {
        m_traffic.windows = parsedWindows;
    }

    // Live scheduling
    auto liveIt = sections.find("live_scheduling");
    if (liveIt != sections.end()) {
        const auto& values = liveIt->second;
        if (auto it = values.find("enabled"); it != values.end()) m_liveScheduling.enabled = parseBool(it->second, m_liveScheduling.enabled);
        if (auto it = values.find("pre_roll_seconds"); it != values.end()) m_liveScheduling.pre_roll_seconds = parseInt(it->second, m_liveScheduling.pre_roll_seconds);
        if (auto it = values.find("post_roll_seconds"); it != values.end()) m_liveScheduling.post_roll_seconds = parseInt(it->second, m_liveScheduling.post_roll_seconds);
    }

    std::vector<LiveSlotConfig> parsedSlots;
    for (const auto& sectionEntry : sections) {
        if (sectionEntry.first.rfind("live_slot", 0) == 0) {
            LiveSlotConfig slot;
            const auto& values = sectionEntry.second;
            if (auto it = values.find("id"); it != values.end()) slot.id = it->second;
            if (auto it = values.find("source"); it != values.end()) slot.source = it->second;
            if (auto it = values.find("start_time"); it != values.end()) slot.start_time = it->second;
            if (auto it = values.find("end_time"); it != values.end()) slot.end_time = it->second;
            if (auto it = values.find("recurrence"); it != values.end()) slot.recurrence = it->second;
            if (auto it = values.find("metadata"); it != values.end()) slot.metadata = parseMetadata(it->second);
            if (!slot.id.empty()) {
                parsedSlots.push_back(slot);
            }
        }
    }
    if (!parsedSlots.empty()) {
        m_liveScheduling.slots = parsedSlots;
    }

    // Multi-platform configuration
    auto multiIt = sections.find("multi_platform");
    if (multiIt != sections.end()) {
        const auto& values = multiIt->second;
        if (auto it = values.find("enabled"); it != values.end()) m_multiPlatform.enabled = parseBool(it->second, m_multiPlatform.enabled);
        if (auto it = values.find("sync_playlists"); it != values.end()) m_multiPlatform.sync_playlists = parseBool(it->second, m_multiPlatform.sync_playlists);
    }

    std::vector<PlatformOutputConfig> parsedPlatforms;
    for (const auto& sectionEntry : sections) {
        if (sectionEntry.first.rfind("platform", 0) == 0) {
            PlatformOutputConfig platform;
            const auto& values = sectionEntry.second;
            if (auto it = values.find("id"); it != values.end()) platform.id = it->second;
            if (auto it = values.find("platform"); it != values.end()) platform.platform = it->second;
            if (auto it = values.find("enabled"); it != values.end()) platform.enabled = parseBool(it->second, true);
            if (auto it = values.find("output_type"); it != values.end()) platform.output.type = it->second;
            if (auto it = values.find("output_url"); it != values.end()) platform.output.url = it->second;
            if (auto it = values.find("output_mux_format"); it != values.end()) platform.output.mux_format = it->second;
            if (auto it = values.find("output_bitrate"); it != values.end()) platform.output.bitrate = parseInt(it->second, platform.output.bitrate);
            if (auto it = values.find("output_programs"); it != values.end()) platform.output.programs = Config::splitList(it->second);
            if (auto it = values.find("metadata"); it != values.end()) platform.metadata = parseMetadata(it->second);
            if (!platform.id.empty()) {
                parsedPlatforms.push_back(platform);
            }
        }
    }
    if (!parsedPlatforms.empty()) {
        m_multiPlatform.platforms = parsedPlatforms;
    }

    // Inputs
    std::vector<InputConfig> parsedInputs;
    for (const auto& sectionEntry : sections) {
        if (sectionEntry.first.rfind("input", 0) == 0) {
            InputConfig input{};
            const auto& values = sectionEntry.second;
            if (auto it = values.find("type"); it != values.end()) input.type = it->second;
            if (auto it = values.find("url"); it != values.end()) input.url = it->second;
            if (auto it = values.find("codec"); it != values.end()) input.codec = it->second;
            if (auto it = values.find("bitrate"); it != values.end()) input.bitrate = parseInt(it->second, input.bitrate);
            if (auto it = values.find("width"); it != values.end()) input.width = parseInt(it->second, input.width);
            if (auto it = values.find("height"); it != values.end()) input.height = parseInt(it->second, input.height);
            if (auto it = values.find("fps"); it != values.end()) {
                try { input.fps = std::stod(it->second); } catch (...) {}
            }
            if (auto it = values.find("enabled"); it != values.end()) input.enabled = parseBool(it->second, true);
            if (!input.url.empty()) {
                parsedInputs.push_back(input);
            }
        }
    }
    if (!parsedInputs.empty()) {
        m_inputs = parsedInputs;
    }

    // Outputs
    std::vector<OutputConfig> parsedOutputs;
    for (const auto& sectionEntry : sections) {
        if (sectionEntry.first.rfind("output", 0) == 0) {
            OutputConfig output{};
            const auto& values = sectionEntry.second;
            if (auto it = values.find("type"); it != values.end()) output.type = it->second;
            if (auto it = values.find("url"); it != values.end()) output.url = it->second;
            if (auto it = values.find("mux_format"); it != values.end()) output.mux_format = it->second;
            if (auto it = values.find("bitrate"); it != values.end()) output.bitrate = parseInt(it->second, output.bitrate);
            if (auto it = values.find("programs"); it != values.end()) output.programs = Config::splitList(it->second);
            if (!output.url.empty()) {
                parsedOutputs.push_back(output);
            }
        }
    }
    if (!parsedOutputs.empty()) {
        m_outputs = parsedOutputs;
    }
}

std::vector<std::string> Config::splitList(const std::string& value) {
    std::vector<std::string> result;
    std::string token;
    std::istringstream stream(value);
    while (std::getline(stream, token, ',')) {
        token.erase(token.begin(), std::find_if(token.begin(), token.end(), [](unsigned char ch) { return !std::isspace(ch); }));
        token.erase(std::find_if(token.rbegin(), token.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), token.end());
        if (!token.empty()) {
            result.push_back(token);
        }
    }
    return result;
}