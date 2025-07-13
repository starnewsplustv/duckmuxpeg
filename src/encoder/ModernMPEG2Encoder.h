#pragma once

#include <memory>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>
#include <queue>
#include <functional>
#include <map>

#include "common/Config.h"
#include "common/CircularBuffer.h"
#include "LookaheadAnalyzer.h"
#include "RateController.h"
#include "MotionEstimation.h"
#include "AdaptiveQuantizer.h"
#include "SceneChangeDetector.h"
#include "ParallelEncoder.h"
#include "RAMBufferManager.h"

// Modern MPEG-2 encoder configuration
struct ModernMPEG2Config {
    // Basic encoding parameters
    int width;
    int height;
    double framerate;
    int bitrate;
    int gop_size;
    int max_b_frames;
    
    // 2025 modern features
    bool enable_lookahead;
    int lookahead_frames;
    bool enable_adaptive_quantization;
    bool enable_scene_change_detection;
    bool enable_parallel_encoding;
    bool enable_hardware_acceleration;
    
    // Rate control
    enum RateControlMode {
        RC_CBR,      // Constant Bitrate
        RC_VBR,      // Variable Bitrate
        RC_CRF,      // Constant Rate Factor
        RC_ABR,      // Average Bitrate
        RC_CVBR      // Constrained Variable Bitrate
    } rate_control_mode;
    
    double crf_value;
    int vbr_maxrate;
    int vbr_bufsize;
    
    // Motion estimation
    enum MotionEstimationLevel {
        ME_FAST,
        ME_MEDIUM,
        ME_SLOW,
        ME_VERYSLOW,
        ME_PLACEBO
    } motion_estimation;
    
    // Multi-threading
    int thread_count;
    int thread_lookahead;
    int thread_slices;
    
    // x264-inspired features for MPEG-2
    bool enable_weighted_prediction;
    bool enable_mixed_references;
    bool enable_fast_pskip;
    bool enable_dct_decimate;
    
    // Adaptive features
    bool enable_mbtree;
    bool enable_aq;
    double aq_strength;
    int aq_mode;
    
    // Denoising and preprocessing
    bool enable_noise_reduction;
    double nr_intra;
    double nr_inter;
    
    // Psychovisual optimizations
    double psy_rd;
    double psy_trellis;
    
    // Buffer management
    bool enable_ram_buffer;
    size_t ram_buffer_size_mb;
    bool enable_ramdrive;
    std::string ramdrive_path;
    
    // NUMA optimization
    bool enable_numa_awareness;
    int numa_node;
};

struct EncodingStats {
    uint64_t frames_encoded;
    uint64_t bytes_encoded;
    double current_fps;
    double average_bitrate;
    double current_quantizer;
    double psnr_y;
    double psnr_u;
    double psnr_v;
    double ssim;
    std::chrono::milliseconds encoding_time;
    size_t memory_usage;
    int active_threads;
};

class ModernMPEG2Encoder {
public:
    explicit ModernMPEG2Encoder(const ModernMPEG2Config& config);
    ~ModernMPEG2Encoder();
    
    // Core encoding interface
    bool initialize();
    bool encode_frame(const VideoFrame& input_frame, std::vector<uint8_t>& output_data);
    bool flush(std::vector<std::vector<uint8_t>>& output_packets);
    bool shutdown();
    
    // Modern features
    bool enable_lookahead_analysis(bool enabled);
    bool set_parallel_threads(int thread_count);
    bool configure_rate_control(const ModernMPEG2Config::RateControlMode mode, double value);
    bool enable_adaptive_quantization(bool enabled, double strength);
    
    // Real-time adjustments
    bool adjust_bitrate(int new_bitrate);
    bool adjust_quality(double quality_factor);
    bool force_keyframe();
    bool set_roi(int x, int y, int width, int height, double quality_offset);
    
    // Buffer management
    bool configure_ram_buffer(size_t size_mb);
    bool setup_ramdrive(const std::string& path, size_t size_mb);
    
    // Statistics and monitoring
    EncodingStats get_stats() const;
    std::vector<double> get_frame_metrics() const;
    bool enable_detailed_stats(bool enabled);
    
    // Presets (x264-inspired)
    bool apply_preset(const std::string& preset_name);
    bool apply_tune(const std::string& tune_name);
    
    // Advanced encoding modes
    bool enable_two_pass_encoding(const std::string& stats_file);
    bool enable_multi_pass_encoding(int passes);
    
    // Hardware acceleration
    bool enable_gpu_acceleration(const std::string& device);
    bool enable_fpga_acceleration();
    
    // Broadcast compliance
    bool configure_for_broadcast(const std::string& standard); // "ATSC", "DVB", "ISDB"
    bool validate_compliance();
    
private:
    ModernMPEG2Config m_config;
    
    // Core components
    std::unique_ptr<LookaheadAnalyzer> m_lookahead;
    std::unique_ptr<RateController> m_rate_controller;
    std::unique_ptr<MotionEstimation> m_motion_estimator;
    std::unique_ptr<AdaptiveQuantizer> m_adaptive_quantizer;
    std::unique_ptr<SceneChangeDetector> m_scene_detector;
    std::unique_ptr<ParallelEncoder> m_parallel_encoder;
    std::unique_ptr<RAMBufferManager> m_buffer_manager;
    
    // Threading
    std::vector<std::thread> m_worker_threads;
    std::queue<VideoFrame> m_input_queue;
    std::queue<std::vector<uint8_t>> m_output_queue;
    std::mutex m_input_mutex;
    std::mutex m_output_mutex;
    std::condition_variable m_input_cv;
    std::condition_variable m_output_cv;
    std::atomic<bool> m_encoding_active;
    
    // Statistics
    mutable std::mutex m_stats_mutex;
    EncodingStats m_stats;
    std::vector<double> m_frame_metrics;
    std::atomic<bool> m_detailed_stats_enabled;
    
    // Internal methods
    void encoding_worker();
    void process_frame_parallel(const VideoFrame& frame);
    bool setup_encoder_context();
    bool configure_advanced_options();
    bool optimize_for_cpu_architecture();
    bool setup_numa_affinity();
    void update_statistics(const VideoFrame& frame, const std::vector<uint8_t>& output);
    
    // Hardware acceleration support
    struct HWAccelContext;
    std::unique_ptr<HWAccelContext> m_hw_context;
    
    // Multi-pass encoding
    struct MultiPassContext;
    std::unique_ptr<MultiPassContext> m_multipass_context;
    
    // SIMD optimizations
    void detect_cpu_features();
    bool m_has_avx2;
    bool m_has_avx512;
    bool m_has_neon;
    
    // Memory pools for zero-copy operation
    class MemoryPool;
    std::unique_ptr<MemoryPool> m_memory_pool;
    
    // Error handling
    std::string m_last_error;
    void set_error(const std::string& error);
    
public:
    // Preset definitions
    static const std::map<std::string, ModernMPEG2Config> PRESETS;
    static const std::map<std::string, std::function<void(ModernMPEG2Config&)>> TUNES;
    
    // Factory methods
    static std::unique_ptr<ModernMPEG2Encoder> create_for_broadcast();
    static std::unique_ptr<ModernMPEG2Encoder> create_for_streaming();
    static std::unique_ptr<ModernMPEG2Encoder> create_for_archive();
};