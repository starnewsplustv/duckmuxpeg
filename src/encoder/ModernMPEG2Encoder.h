#pragma once

#include <atomic>
#include <condition_variable>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "common/CircularBuffer.h"

struct ModernMPEG2Config {
    int width = 1920;
    int height = 1080;
    double framerate = 59.94;
    int bitrate = 8000; // kbps
    int gop_size = 60;
    int max_b_frames = 2;

    bool enable_lookahead = true;
    int lookahead_frames = 12;
    bool enable_adaptive_quantization = true;
    bool enable_scene_change_detection = true;
    bool enable_parallel_encoding = true;
    bool enable_hardware_acceleration = false;

    enum class RateControlMode {
        CBR,
        VBR,
        CRF,
        ABR,
        CVBR
    } rate_control_mode = RateControlMode::CBR;

    double crf_value = 18.0;
    int vbr_maxrate = 12000; // kbps
    int vbr_bufsize = 16000; // kbps

    int thread_count = static_cast<int>(std::thread::hardware_concurrency());
    int thread_lookahead = 2;

    bool enable_weighted_prediction = false;
    bool enable_fast_pskip = true;

    bool enable_ram_buffer = false;
    size_t ram_buffer_size_mb = 0;
    bool enable_ramdrive = false;
    std::string ramdrive_path;

    bool enable_numa_awareness = false;
    int numa_node = -1;
};

struct EncodingStats {
    uint64_t frames_encoded = 0;
    uint64_t bytes_encoded = 0;
    double current_fps = 0.0;
    double average_bitrate = 0.0;
    double current_quantizer = 0.0;
    double psnr_y = 0.0;
    double psnr_u = 0.0;
    double psnr_v = 0.0;
    double ssim = 0.0;
    std::chrono::milliseconds encoding_time{0};
    size_t memory_usage = 0;
    int active_threads = 0;
};

class ModernMPEG2Encoder {
public:
    explicit ModernMPEG2Encoder(const ModernMPEG2Config& config);
    ~ModernMPEG2Encoder();

    bool initialize();
    bool encode_frame(const VideoFrame& input_frame, std::vector<uint8_t>& output_data);
    bool flush(std::vector<std::vector<uint8_t>>& output_packets);
    bool shutdown();

    bool enable_lookahead_analysis(bool enabled);
    bool set_parallel_threads(int thread_count);
    bool configure_rate_control(ModernMPEG2Config::RateControlMode mode, double value);
    bool enable_adaptive_quantization(bool enabled, double strength);

    bool adjust_bitrate(int new_bitrate);
    bool adjust_quality(double quality_factor);
    bool force_keyframe();
    bool set_roi(int x, int y, int width, int height, double quality_offset);

    bool configure_ram_buffer(size_t size_mb);
    bool setup_ramdrive(const std::string& path, size_t size_mb);

    EncodingStats get_stats() const;
    std::vector<double> get_frame_metrics() const;
    bool enable_detailed_stats(bool enabled);

    bool apply_preset(const std::string& preset_name);
    bool apply_tune(const std::string& tune_name);

    bool enable_two_pass_encoding(const std::string& stats_file);
    bool enable_multi_pass_encoding(int passes);

    bool enable_gpu_acceleration(const std::string& device);
    bool enable_fpga_acceleration();

    bool configure_for_broadcast(const std::string& standard);
    bool validate_compliance();

    static const std::map<std::string, ModernMPEG2Config> PRESETS;

    static std::unique_ptr<ModernMPEG2Encoder> create_for_broadcast();
    static std::unique_ptr<ModernMPEG2Encoder> create_for_streaming();
    static std::unique_ptr<ModernMPEG2Encoder> create_for_archive();

private:
    ModernMPEG2Config m_config;

    mutable std::mutex m_mutex;
    std::queue<VideoFrame> m_pending_frames;
    std::queue<std::vector<uint8_t>> m_completed_packets;
    std::condition_variable m_pending_cv;
    std::condition_variable m_completed_cv;

    std::atomic<bool> m_initialized;
    std::atomic<bool> m_encoding_active;
    std::atomic<bool> m_detailed_stats_enabled;

    bool m_force_next_keyframe;
    int m_parallel_threads;
    double m_rate_control_value;
    std::string m_stats_file;
    int m_pass_count;
    std::string m_hw_device;
    bool m_fpga_enabled;
    std::string m_broadcast_standard;

    EncodingStats m_stats;
    std::vector<double> m_recent_metrics;
    std::chrono::steady_clock::time_point m_start_time;

    void reset_state();
    void update_statistics(const VideoFrame& frame, const std::vector<uint8_t>& output);
    std::vector<uint8_t> synthesize_packet(const VideoFrame& frame, bool keyframe_hint);
};
