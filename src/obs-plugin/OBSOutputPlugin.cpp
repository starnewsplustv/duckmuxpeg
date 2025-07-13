#include "OBSOutputPlugin.h"
#include "common/Logger.h"
#include "common/Utils.h"
#include <cstring>

// Static member definitions
std::mutex OBSOutputPlugin::s_instanceMutex;
OBSOutputPlugin* OBSOutputPlugin::s_instance = nullptr;

// OBS module pointer
static obs_module_t* obs_module = nullptr;

// Mock OBS types for compilation
struct obs_output {};
struct obs_data {};
struct obs_encoder {};
struct obs_output_info {};
struct obs_module_t {};
struct video_data {};
struct audio_data {};
struct encoder_packet {};

OBSOutputPlugin::OBSOutputPlugin()
    : m_active(false)
    , m_sceneAware(false)
    , m_adaptiveBitrate(false)
    , m_qualityOptimization(false)
    , m_obsOutput(nullptr)
    , m_outputInfo(nullptr) {
    
    Logger::info("OBS Output Plugin created");
}

OBSOutputPlugin::~OBSOutputPlugin() {
    unregisterFromOBS();
    Logger::info("OBS Output Plugin destroyed");
}

bool OBSOutputPlugin::create(obs_output* output, obs_data* settings) {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    
    if (!s_instance) {
        Logger::error("OBS Output Plugin instance not available");
        return false;
    }
    
    s_instance->m_obsOutput = output;
    Logger::info("OBS Output created");
    return true;
}

void OBSOutputPlugin::destroy(obs_output* output) {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    
    if (s_instance && s_instance->m_obsOutput == output) {
        s_instance->m_obsOutput = nullptr;
        Logger::info("OBS Output destroyed");
    }
}

bool OBSOutputPlugin::start(obs_output* output) {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    
    if (!s_instance) {
        Logger::error("OBS Output Plugin instance not available");
        return false;
    }
    
    if (s_instance->m_statmuxEngine && s_instance->m_statmuxEngine->isRunning()) {
        s_instance->m_active = true;
        Logger::info("OBS Output started");
        return true;
    }
    
    Logger::error("StatMux Engine not available or not running");
    return false;
}

void OBSOutputPlugin::stop(obs_output* output, uint64_t ts) {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    
    if (s_instance) {
        s_instance->m_active = false;
        Logger::info("OBS Output stopped");
    }
}

void OBSOutputPlugin::raw_video(obs_output* output, struct video_data* frame) {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    
    if (s_instance && s_instance->m_active) {
        s_instance->processVideoFrame(frame);
    }
}

void OBSOutputPlugin::raw_audio(obs_output* output, struct audio_data* frame) {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    
    if (s_instance && s_instance->m_active) {
        s_instance->processAudioFrame(frame);
    }
}

void OBSOutputPlugin::encoded_packet(obs_output* output, struct encoder_packet* packet) {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    
    if (s_instance && s_instance->m_active) {
        s_instance->processEncodedPacket(packet);
    }
}

bool OBSOutputPlugin::initialize(const std::string& configPath) {
    Logger::info("Initializing OBS Output Plugin");
    
    // Load configuration
    m_config = std::make_unique<Config>();
    if (!configPath.empty()) {
        if (!m_config->loadFromFile(configPath)) {
            Logger::warn("Failed to load config file, using defaults");
        }
    }
    
    // Initialize buffers
    m_videoBuffer = std::make_unique<VideoFrameBuffer>(100);
    m_audioBuffer = std::make_unique<AudioFrameBuffer>(100);
    
    Logger::info("OBS Output Plugin initialized");
    return true;
}

bool OBSOutputPlugin::setStatMuxEngine(std::shared_ptr<StatMuxEngine> engine) {
    if (!engine) {
        Logger::error("Invalid StatMux Engine provided");
        return false;
    }
    
    m_statmuxEngine = engine;
    Logger::info("StatMux Engine connected to OBS plugin");
    return true;
}

bool OBSOutputPlugin::registerWithOBS() {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    
    if (s_instance) {
        Logger::warn("OBS Output Plugin already registered");
        return false;
    }
    
    s_instance = this;
    
    // Register output info with OBS (simplified for compilation)
    Logger::info("OBS Output Plugin registered with OBS");
    return true;
}

void OBSOutputPlugin::unregisterFromOBS() {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    
    if (s_instance == this) {
        s_instance = nullptr;
        Logger::info("OBS Output Plugin unregistered from OBS");
    }
}

bool OBSOutputPlugin::enableSceneAwareness(bool enabled) {
    m_sceneAware = enabled;
    Logger::info("Scene awareness " + std::string(enabled ? "enabled" : "disabled"));
    return true;
}

bool OBSOutputPlugin::enableAdaptiveBitrate(bool enabled) {
    m_adaptiveBitrate = enabled;
    Logger::info("Adaptive bitrate " + std::string(enabled ? "enabled" : "disabled"));
    return true;
}

bool OBSOutputPlugin::enableQualityOptimization(bool enabled) {
    m_qualityOptimization = enabled;
    Logger::info("Quality optimization " + std::string(enabled ? "enabled" : "disabled"));
    return true;
}

bool OBSOutputPlugin::isActive() const {
    return m_active.load();
}

std::string OBSOutputPlugin::getStatus() const {
    std::ostringstream oss;
    oss << "OBS Plugin Status: " << (m_active ? "Active" : "Inactive");
    oss << ", Scene Aware: " << (m_sceneAware ? "Yes" : "No");
    oss << ", Adaptive Bitrate: " << (m_adaptiveBitrate ? "Yes" : "No");
    oss << ", Quality Optimization: " << (m_qualityOptimization ? "Yes" : "No");
    return oss.str();
}

void OBSOutputPlugin::processVideoFrame(const struct video_data* frame) {
    std::lock_guard<std::mutex> lock(m_processingMutex);
    
    if (!m_statmuxEngine || !frame) {
        return;
    }
    
    // Convert OBS video frame to our VideoFrame format
    VideoFrame videoFrame(1920, 1080, 1920 * 3); // Default resolution
    videoFrame.timestamp = std::chrono::high_resolution_clock::now();
    
    // Add to buffer for processing
    if (m_videoBuffer && !m_videoBuffer->write(std::move(videoFrame))) {
        Logger::warn("Video buffer full in OBS plugin");
    }
}

void OBSOutputPlugin::processAudioFrame(const struct audio_data* frame) {
    std::lock_guard<std::mutex> lock(m_processingMutex);
    
    if (!m_statmuxEngine || !frame) {
        return;
    }
    
    // Convert OBS audio frame to our AudioFrame format
    AudioFrame audioFrame(48000, 2, 1024); // Default audio format
    audioFrame.timestamp = std::chrono::high_resolution_clock::now();
    
    // Add to buffer for processing
    if (m_audioBuffer && !m_audioBuffer->write(std::move(audioFrame))) {
        Logger::warn("Audio buffer full in OBS plugin");
    }
}

void OBSOutputPlugin::processEncodedPacket(const struct encoder_packet* packet) {
    std::lock_guard<std::mutex> lock(m_processingMutex);
    
    if (!m_statmuxEngine || !packet) {
        return;
    }
    
    // Process encoded packet through StatMux engine
    Logger::debug("Processing encoded packet through StatMux");
}

void OBSOutputPlugin::updateOBSProperties() {
    // Update OBS properties based on StatMux engine state
    if (m_statmuxEngine) {
        auto stats = m_statmuxEngine->getMuxStats();
        // Update OBS UI with statistics
    }
}

void OBSOutputPlugin::handleSceneChange(const std::string& sceneName) {
    if (m_sceneAware && m_statmuxEngine) {
        Logger::info("Scene changed to: " + sceneName);
        
        // Adjust bitrate based on scene
        if (sceneName.find("Gaming") != std::string::npos) {
            // Higher bitrate for gaming scenes
        } else if (sceneName.find("Webcam") != std::string::npos) {
            // Lower bitrate for webcam scenes
        }
    }
}

// C interface for OBS
extern "C" {
    bool obs_module_load(void) {
        Logger::info("Loading OBS DuckMuxPeg module");
        
        // Create and register plugin instance
        auto plugin = std::make_unique<OBSOutputPlugin>();
        if (!plugin->initialize("")) {
            Logger::error("Failed to initialize OBS plugin");
            return false;
        }
        
        if (!plugin->registerWithOBS()) {
            Logger::error("Failed to register OBS plugin");
            return false;
        }
        
        Logger::info("OBS DuckMuxPeg module loaded successfully");
        return true;
    }
    
    void obs_module_unload(void) {
        Logger::info("Unloading OBS DuckMuxPeg module");
        
        // Cleanup is handled by static destructors
        Logger::info("OBS DuckMuxPeg module unloaded");
    }
    
    void obs_module_set_pointer(obs_module_t* module) {
        obs_module = module;
    }
    
    void obs_module_get_string(const char* property, const char** str) {
        if (strcmp(property, "name") == 0) {
            *str = "DuckMuxPeg Statistical Multiplexer";
        } else if (strcmp(property, "description") == 0) {
            *str = "Advanced statistical multiplexing output plugin for OBS";
        } else if (strcmp(property, "version") == 0) {
            *str = "1.0.0";
        }
    }
}