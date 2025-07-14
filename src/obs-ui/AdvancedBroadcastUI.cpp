#include "AdvancedBroadcastUI.h"
#include "common/Logger.h"
#include "common/Utils.h"
#include <obs-module.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>

// Dektec ASI driver includes (assuming driver is installed)
#ifdef DEKTEC_ASI_SUPPORT
#include <DTAPI.h>
#include <DtapiTypes.h>
#endif

// FFmpeg includes for H.264 encoding
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

AdvancedBroadcastUI::AdvancedBroadcastUI()
    : BroadcastComplianceUI()
    , m_current_statmux_method(StatMuxMethod::TRADITIONAL)
    , m_x264_integration_enabled(false)
    , m_experimental_features_enabled(false)
    , m_current_scene("default")
{
    Logger::info("AdvancedBroadcastUI: Initializing advanced broadcast UI");
    
    // Initialize ASI ports (4 ports for Dektec 2154)
    m_asi_ports.resize(4);
    for (int i = 0; i < 4; ++i) {
        m_asi_ports[i].port_number = i;
        m_asi_ports[i].active = false;
        m_asi_ports[i].packets_sent = 0;
        m_asi_ports[i].bytes_sent = 0;
        
        // Default ASI configuration
        m_asi_ports[i].config.device_path = "/dev/dtapi" + std::to_string(i);
        m_asi_ports[i].config.port_number = i;
        m_asi_ports[i].config.bitrate = 19392658; // Default ATSC bitrate
        m_asi_ports[i].config.output_format = "ATSC";
        m_asi_ports[i].config.enable_asi_output = false;
        m_asi_ports[i].config.asi_mode = "continuous";
        m_asi_ports[i].config.asi_clock_rate = 270000000; // SMPTE 310M
        m_asi_ports[i].config.enable_stuffing = true;
    }
    
    // Initialize advanced stats
    m_advanced_stats.last_update = std::chrono::system_clock::now();
    m_advanced_stats.current_method = StatMuxMethod::TRADITIONAL;
    m_advanced_stats.statmux_efficiency = 0.0;
    m_advanced_stats.total_allocated_bitrate = 0;
    m_advanced_stats.total_available_bitrate = 0;
    m_advanced_stats.current_scene = "default";
    m_advanced_stats.scene_changes_count = 0;
}

AdvancedBroadcastUI::~AdvancedBroadcastUI() {
    Logger::info("AdvancedBroadcastUI: Shutting down advanced broadcast UI");
    
    // Stop all ASI transmissions
    for (int i = 0; i < 4; ++i) {
        stop_asi_transmission(i);
    }
    
    // Stop all social media streams
    for (auto& stream : m_social_streams) {
        if (stream.active) {
            stream.active = false;
            if (stream.streaming_thread && stream.streaming_thread->joinable()) {
                stream.streaming_thread->join();
            }
        }
    }
    
    // Stop report generator thread
    if (m_report_generator_thread && m_report_generator_thread->joinable()) {
        m_report_generator_thread->join();
    }
}

bool AdvancedBroadcastUI::initialize(std::shared_ptr<StatMuxEngine> statmux_engine,
                                   std::shared_ptr<ModernMPEG2Encoder> encoder,
                                   std::shared_ptr<ATSCCompliance> compliance) {
    Logger::info("AdvancedBroadcastUI: Initializing with core components");
    
    // Initialize base class
    if (!connect_encoder(encoder) || !connect_compliance_checker(compliance)) {
        Logger::error("AdvancedBroadcastUI: Failed to initialize base components");
        return false;
    }
    
    // Store StatMux engine reference
    m_statmux_engine = statmux_engine;
    
    // Initialize subsystems
    initialize_asi_hardware();
    initialize_social_streaming();
    initialize_statmux_integration();
    initialize_scene_management();
    initialize_scheduling();
    initialize_reporting();
    
    Logger::info("AdvancedBroadcastUI: Initialization complete");
    return true;
}

// =========================
// ASI TRANSMITTER PANEL
// =========================

obs_properties* AdvancedBroadcastUI::create_asi_properties() {
    Logger::debug("AdvancedBroadcastUI: Creating ASI properties");
    
    obs_properties* props = obs_properties_create();
    
    // ASI Hardware Detection
    obs_property_t* hw_group = obs_properties_add_group(props, "asi_hardware", 
        "ASI Hardware Configuration", OBS_GROUP_NORMAL, nullptr);
    obs_properties_t* hw_props = obs_property_group_content(hw_group);
    
    // Hardware detection button
    obs_properties_add_button(hw_props, "detect_asi_hardware", "Detect ASI Hardware",
        [](obs_properties* props, obs_property* property, void* data) -> bool {
            auto* ui = static_cast<AdvancedBroadcastUI*>(data);
            return ui->detect_asi_hardware();
        });
    
    // ASI Port Configuration
    obs_property_t* port_list = obs_properties_add_list(hw_props, "asi_port_select",
        "ASI Port", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    
    for (int i = 0; i < 4; ++i) {
        std::string port_name = "Port " + std::to_string(i);
        obs_property_list_add_int(port_list, port_name.c_str(), i);
    }
    
    obs_property_set_modified_callback(port_list, on_asi_port_selected);
    
    // Port Configuration Group
    obs_property_t* config_group = obs_properties_add_group(props, "asi_config",
        "Port Configuration", OBS_GROUP_NORMAL, nullptr);
    obs_properties_t* config_props = obs_property_group_content(config_group);
    
    // Output Format
    obs_property_t* format_list = obs_properties_add_list(config_props, "asi_output_format",
        "Output Format", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    obs_property_list_add_string(format_list, "ATSC", "ATSC");
    obs_property_list_add_string(format_list, "QAM", "QAM");
    obs_property_list_add_string(format_list, "DVB-S", "DVB-S");
    obs_property_list_add_string(format_list, "DVB-T", "DVB-T");
    
    // Bitrate
    obs_properties_add_int(config_props, "asi_bitrate", "Bitrate (bps)", 
        1000000, 50000000, 1000);
    
    // ASI Mode
    obs_property_t* mode_list = obs_properties_add_list(config_props, "asi_mode",
        "ASI Mode", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    obs_property_list_add_string(mode_list, "Continuous", "continuous");
    obs_property_list_add_string(mode_list, "Burst", "burst");
    obs_property_list_add_string(mode_list, "Gapped", "gapped");
    
    // Clock Rate
    obs_properties_add_int(config_props, "asi_clock_rate", "Clock Rate (Hz)",
        270000000, 270000000, 1);
    
    // Enable stuffing
    obs_properties_add_bool(config_props, "asi_enable_stuffing", "Enable Null Packet Stuffing");
    
    // Control Buttons
    obs_property_t* control_group = obs_properties_add_group(props, "asi_control",
        "ASI Control", OBS_GROUP_NORMAL, nullptr);
    obs_properties_t* control_props = obs_property_group_content(control_group);
    
    obs_properties_add_button(control_props, "asi_start", "Start Transmission",
        on_asi_start_clicked);
    obs_properties_add_button(control_props, "asi_stop", "Stop Transmission",
        on_asi_stop_clicked);
    
    // Status Display
    obs_property_t* status_group = obs_properties_add_group(props, "asi_status",
        "ASI Status", OBS_GROUP_NORMAL, nullptr);
    obs_properties_t* status_props = obs_property_group_content(status_group);
    
    obs_properties_add_text(status_props, "asi_status_text", "Status", OBS_TEXT_INFO);
    obs_properties_add_text(status_props, "asi_packets_sent", "Packets Sent", OBS_TEXT_INFO);
    obs_properties_add_text(status_props, "asi_bytes_sent", "Bytes Sent", OBS_TEXT_INFO);
    obs_properties_add_text(status_props, "asi_throughput", "Throughput", OBS_TEXT_INFO);
    
    return props;
}

bool AdvancedBroadcastUI::configure_asi_port(int port, const ASIConfig& config) {
    if (port < 0 || port >= 4) {
        Logger::error("AdvancedBroadcastUI: Invalid ASI port number: " + std::to_string(port));
        return false;
    }
    
    if (!validate_asi_config(config)) {
        Logger::error("AdvancedBroadcastUI: Invalid ASI configuration");
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_asi_mutex);
    
    // Stop transmission if active
    if (m_asi_ports[port].active) {
        stop_asi_transmission(port);
    }
    
    // Update configuration
    m_asi_ports[port].config = config;
    
    // Configure hardware
    if (!configure_asi_hardware(port, config)) {
        Logger::error("AdvancedBroadcastUI: Failed to configure ASI hardware for port " + std::to_string(port));
        return false;
    }
    
    Logger::info("AdvancedBroadcastUI: ASI port " + std::to_string(port) + " configured for " + config.output_format);
    return true;
}

bool AdvancedBroadcastUI::start_asi_transmission(int port) {
    if (port < 0 || port >= 4) {
        Logger::error("AdvancedBroadcastUI: Invalid ASI port number: " + std::to_string(port));
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_asi_mutex);
    
    if (m_asi_ports[port].active) {
        Logger::warn("AdvancedBroadcastUI: ASI port " + std::to_string(port) + " already active");
        return true;
    }
    
    // Start transmission thread
    m_asi_ports[port].active = true;
    m_asi_ports[port].packets_sent = 0;
    m_asi_ports[port].bytes_sent = 0;
    m_asi_ports[port].last_packet_time = std::chrono::system_clock::now();
    
    m_asi_ports[port].transmission_thread = std::make_unique<std::thread>(
        &AdvancedBroadcastUI::asi_transmission_thread, this, port);
    
    Logger::info("AdvancedBroadcastUI: ASI transmission started on port " + std::to_string(port));
    return true;
}

bool AdvancedBroadcastUI::stop_asi_transmission(int port) {
    if (port < 0 || port >= 4) {
        Logger::error("AdvancedBroadcastUI: Invalid ASI port number: " + std::to_string(port));
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_asi_mutex);
    
    if (!m_asi_ports[port].active) {
        Logger::warn("AdvancedBroadcastUI: ASI port " + std::to_string(port) + " not active");
        return true;
    }
    
    // Stop transmission
    m_asi_ports[port].active = false;
    
    if (m_asi_ports[port].transmission_thread && m_asi_ports[port].transmission_thread->joinable()) {
        m_asi_ports[port].transmission_thread->join();
    }
    
    Logger::info("AdvancedBroadcastUI: ASI transmission stopped on port " + std::to_string(port));
    return true;
}

void AdvancedBroadcastUI::asi_transmission_thread(int port) {
    Logger::info("AdvancedBroadcastUI: ASI transmission thread started for port " + std::to_string(port));
    
#ifdef DEKTEC_ASI_SUPPORT
    // Dektec ASI API implementation
    DtDevice device;
    DtOutput output;
    
    try {
        // Open device
        DTAPI_RESULT result = device.AttachToSerial(0); // First device
        if (result != DTAPI_OK) {
            Logger::error("AdvancedBroadcastUI: Failed to attach to Dektec device");
            return;
        }
        
        // Configure output port
        result = output.AttachToPort(&device, port);
        if (result != DTAPI_OK) {
            Logger::error("AdvancedBroadcastUI: Failed to attach to ASI port " + std::to_string(port));
            return;
        }
        
        // Set ASI mode
        result = output.SetIoConfig(DTAPI_IOCONFIG_ASI);
        if (result != DTAPI_OK) {
            Logger::error("AdvancedBroadcastUI: Failed to set ASI mode");
            return;
        }
        
        // Set bitrate
        result = output.SetTsRateBps(m_asi_ports[port].config.bitrate);
        if (result != DTAPI_OK) {
            Logger::error("AdvancedBroadcastUI: Failed to set bitrate");
            return;
        }
        
        // Main transmission loop
        while (m_asi_ports[port].active) {
            // Get encoded data from StatMux engine
            if (m_statmux_engine) {
                // Request data for this port's format
                std::vector<uint8_t> transport_data;
                
                // Get appropriate stream based on output format
                if (m_asi_ports[port].config.output_format == "ATSC") {
                    // Get ATSC stream data
                    transport_data = get_atsc_stream_data(port);
                } else if (m_asi_ports[port].config.output_format == "QAM") {
                    // Get QAM stream data
                    transport_data = get_qam_stream_data(port);
                }
                
                if (!transport_data.empty()) {
                    // Send data to ASI port
                    result = output.Write(transport_data.data(), transport_data.size());
                    if (result == DTAPI_OK) {
                        m_asi_ports[port].packets_sent += transport_data.size() / 188; // TS packet size
                        m_asi_ports[port].bytes_sent += transport_data.size();
                        m_asi_ports[port].last_packet_time = std::chrono::system_clock::now();
                    }
                }
            }
            
            // Small delay to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        // Cleanup
        output.Detach();
        device.Detach();
        
    } catch (const std::exception& e) {
        Logger::error("AdvancedBroadcastUI: ASI transmission error: " + std::string(e.what()));
    }
    
#else
    // Simulation mode when Dektec driver not available
    Logger::info("AdvancedBroadcastUI: Running in ASI simulation mode");
    
    while (m_asi_ports[port].active) {
        // Simulate transmission
        m_asi_ports[port].packets_sent += 100; // Simulate 100 packets
        m_asi_ports[port].bytes_sent += 18800; // 100 * 188 bytes
        m_asi_ports[port].last_packet_time = std::chrono::system_clock::now();
        
        // Update statistics
        std::lock_guard<std::mutex> stats_lock(m_stats_mutex);
        m_advanced_stats.asi_packets_sent[port] = m_asi_ports[port].packets_sent;
        m_advanced_stats.asi_bytes_sent[port] = m_asi_ports[port].bytes_sent;
        m_advanced_stats.asi_port_status[port] = true;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Update final statistics
    std::lock_guard<std::mutex> stats_lock(m_stats_mutex);
    m_advanced_stats.asi_port_status[port] = false;
    
#endif
    
    Logger::info("AdvancedBroadcastUI: ASI transmission thread finished for port " + std::to_string(port));
}

// Private helper methods

void AdvancedBroadcastUI::initialize_asi_hardware() {
    Logger::info("AdvancedBroadcastUI: Initializing ASI hardware");
    
    // Detect ASI hardware
    if (!detect_asi_hardware()) {
        Logger::warn("AdvancedBroadcastUI: No ASI hardware detected, running in simulation mode");
    }
}

bool AdvancedBroadcastUI::detect_asi_hardware() {
#ifdef DEKTEC_ASI_SUPPORT
    try {
        DtDevice device;
        DTAPI_RESULT result = device.AttachToSerial(0);
        
        if (result == DTAPI_OK) {
            Logger::info("AdvancedBroadcastUI: Dektec ASI hardware detected");
            device.Detach();
            return true;
        }
    } catch (const std::exception& e) {
        Logger::error("AdvancedBroadcastUI: Error detecting ASI hardware: " + std::string(e.what()));
    }
#endif
    
    return false;
}

bool AdvancedBroadcastUI::configure_asi_hardware(int port, const ASIConfig& config) {
    Logger::info("AdvancedBroadcastUI: Configuring ASI hardware for port " + std::to_string(port));
    
    // Hardware configuration would be implemented here
    // For now, just validate the configuration
    return validate_asi_config(config);
}

bool AdvancedBroadcastUI::validate_asi_config(const ASIConfig& config) {
    // Validate port number
    if (config.port_number < 0 || config.port_number >= 4) {
        Logger::error("AdvancedBroadcastUI: Invalid ASI port number: " + std::to_string(config.port_number));
        return false;
    }
    
    // Validate bitrate
    if (config.bitrate < 1000000 || config.bitrate > 50000000) {
        Logger::error("AdvancedBroadcastUI: Invalid ASI bitrate: " + std::to_string(config.bitrate));
        return false;
    }
    
    // Validate output format
    if (config.output_format != "ATSC" && config.output_format != "QAM" &&
        config.output_format != "DVB-S" && config.output_format != "DVB-T") {
        Logger::error("AdvancedBroadcastUI: Invalid ASI output format: " + config.output_format);
        return false;
    }
    
    // Validate ASI mode
    if (config.asi_mode != "continuous" && config.asi_mode != "burst" && config.asi_mode != "gapped") {
        Logger::error("AdvancedBroadcastUI: Invalid ASI mode: " + config.asi_mode);
        return false;
    }
    
    return true;
}

// UI callback handlers

bool AdvancedBroadcastUI::on_asi_port_selected(obs_properties* props, obs_property* property, obs_data* settings) {
    // Update UI based on selected port
    int port = (int)obs_data_get_int(settings, "asi_port_select");
    
    // This would update the configuration UI to show the selected port's settings
    Logger::debug("AdvancedBroadcastUI: ASI port " + std::to_string(port) + " selected");
    
    return true;
}

bool AdvancedBroadcastUI::on_asi_start_clicked(obs_properties* props, obs_property* property, void* data) {
    auto* ui = static_cast<AdvancedBroadcastUI*>(data);
    
    // Get current port from UI
    obs_data_t* settings = obs_properties_get_settings(props);
    int port = (int)obs_data_get_int(settings, "asi_port_select");
    
    bool result = ui->start_asi_transmission(port);
    
    obs_data_release(settings);
    return result;
}

bool AdvancedBroadcastUI::on_asi_stop_clicked(obs_properties* props, obs_property* property, void* data) {
    auto* ui = static_cast<AdvancedBroadcastUI*>(data);
    
    // Get current port from UI
    obs_data_t* settings = obs_properties_get_settings(props);
    int port = (int)obs_data_get_int(settings, "asi_port_select");
    
    bool result = ui->stop_asi_transmission(port);
    
    obs_data_release(settings);
    return result;
}

// Placeholder methods for ATSC/QAM stream data
std::vector<uint8_t> AdvancedBroadcastUI::get_atsc_stream_data(int port) {
    // This would interface with the StatMux engine to get ATSC formatted data
    // For now, return empty vector
    return std::vector<uint8_t>();
}

std::vector<uint8_t> AdvancedBroadcastUI::get_qam_stream_data(int port) {
    // This would interface with the StatMux engine to get QAM formatted data
    // For now, return empty vector
    return std::vector<uint8_t>();
}