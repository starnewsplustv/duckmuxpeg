#include "Config.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>

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
}