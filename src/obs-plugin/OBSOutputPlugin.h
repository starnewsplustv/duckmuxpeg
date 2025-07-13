#pragma once

#include <memory>
#include <string>
#include <atomic>
#include <mutex>

// Forward declarations for OBS types
struct obs_output;
struct obs_data;
struct obs_encoder;
struct obs_output_info;

#include "statmux/StatMuxEngine.h"
#include "common/Config.h"
#include "common/CircularBuffer.h"

class OBSOutputPlugin {
public:
    OBSOutputPlugin();
    ~OBSOutputPlugin();
    
    // OBS plugin interface
    static bool create(obs_output* output, obs_data* settings);
    static void destroy(obs_output* output);
    static bool start(obs_output* output);
    static void stop(obs_output* output, uint64_t ts);
    static void raw_video(obs_output* output, struct video_data* frame);
    static void raw_audio(obs_output* output, struct audio_data* frame);
    static void encoded_packet(obs_output* output, struct encoder_packet* packet);
    
    // Configuration and control
    bool initialize(const std::string& configPath);
    bool setStatMuxEngine(std::shared_ptr<StatMuxEngine> engine);
    
    // OBS integration
    bool registerWithOBS();
    void unregisterFromOBS();
    
    // Deep integration features
    bool enableSceneAwareness(bool enabled);
    bool enableAdaptiveBitrate(bool enabled);
    bool enableQualityOptimization(bool enabled);
    
    // Status and monitoring
    bool isActive() const;
    std::string getStatus() const;
    
private:
    std::shared_ptr<StatMuxEngine> m_statmuxEngine;
    std::unique_ptr<Config> m_config;
    std::atomic<bool> m_active;
    std::atomic<bool> m_sceneAware;
    std::atomic<bool> m_adaptiveBitrate;
    std::atomic<bool> m_qualityOptimization;
    
    // Frame processing
    std::unique_ptr<VideoFrameBuffer> m_videoBuffer;
    std::unique_ptr<AudioFrameBuffer> m_audioBuffer;
    
    // Threading
    std::mutex m_processingMutex;
    
    // Internal methods
    void processVideoFrame(const struct video_data* frame);
    void processAudioFrame(const struct audio_data* frame);
    void processEncodedPacket(const struct encoder_packet* packet);
    
    // OBS integration helpers
    void updateOBSProperties();
    void handleSceneChange(const std::string& sceneName);
    
    // Instance management
    static std::mutex s_instanceMutex;
    static OBSOutputPlugin* s_instance;
    
    obs_output* m_obsOutput;
    obs_output_info* m_outputInfo;
};

// C interface for OBS
extern "C" {
    bool obs_module_load(void);
    void obs_module_unload(void);
    void obs_module_set_pointer(obs_module_t* module);
    void obs_module_get_string(const char* property, const char** str);
}