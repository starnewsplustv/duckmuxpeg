#pragma once

#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <memory>

#include "common/CircularBuffer.h"

struct FrameAnalysis {
    uint64_t frame_number;
    double complexity;
    double motion_activity;
    double texture_complexity;
    double temporal_complexity;
    bool is_scene_change;
    bool should_be_keyframe;
    double recommended_quantizer;
    std::vector<double> block_complexities;
    
    // Motion vectors for lookahead
    struct MotionVector {
        int16_t x, y;
        double cost;
    };
    std::vector<MotionVector> motion_vectors;
    
    // Spatial analysis
    double spatial_variance;
    double edge_density;
    double noise_level;
    
    // Temporal analysis
    double temporal_variance;
    double motion_magnitude;
    bool is_fade_in;
    bool is_fade_out;
    
    // Rate control hints
    double predicted_bits;
    double quality_boost_factor;
    int recommended_qp_delta;
};

class LookaheadAnalyzer {
public:
    explicit LookaheadAnalyzer(int lookahead_frames, int thread_count = 0);
    ~LookaheadAnalyzer();
    
    bool initialize(int width, int height);
    bool analyze_frame(const VideoFrame& frame);
    std::vector<FrameAnalysis> get_analysis_batch(size_t count);
    FrameAnalysis get_next_analysis();
    
    // Configuration
    bool set_lookahead_distance(int frames);
    bool enable_scene_change_detection(bool enabled);
    bool enable_motion_analysis(bool enabled);
    bool enable_texture_analysis(bool enabled);
    
    // Advanced features
    bool enable_mbtree_analysis(bool enabled);
    bool enable_weighted_prediction_analysis(bool enabled);
    bool configure_complexity_weighting(double motion_weight, double texture_weight);
    
    // Statistics
    size_t get_queue_size() const;
    double get_average_complexity() const;
    bool is_processing() const;
    
private:
    struct AnalysisContext {
        int width;
        int height;
        int lookahead_frames;
        int thread_count;
        bool scene_change_enabled;
        bool motion_analysis_enabled;
        bool texture_analysis_enabled;
        bool mbtree_enabled;
        bool weighted_prediction_enabled;
        
        // Weighting factors
        double motion_weight;
        double texture_weight;
        double temporal_weight;
    };
    
    AnalysisContext m_context;
    
    // Threading
    std::vector<std::thread> m_worker_threads;
    std::queue<VideoFrame> m_input_queue;
    std::queue<FrameAnalysis> m_output_queue;
    std::mutex m_input_mutex;
    std::mutex m_output_mutex;
    std::condition_variable m_input_cv;
    std::condition_variable m_output_cv;
    std::atomic<bool> m_processing;
    
    // Frame storage for temporal analysis
    std::vector<VideoFrame> m_frame_buffer;
    std::mutex m_buffer_mutex;
    
    // Analysis methods
    void analysis_worker();
    FrameAnalysis analyze_frame_internal(const VideoFrame& frame, size_t frame_index);
    
    // Complexity analysis
    double calculate_spatial_complexity(const VideoFrame& frame);
    double calculate_temporal_complexity(const VideoFrame& current, const VideoFrame& previous);
    double calculate_texture_complexity(const VideoFrame& frame);
    double calculate_motion_activity(const VideoFrame& current, const VideoFrame& previous);
    
    // Scene change detection
    bool detect_scene_change(const VideoFrame& current, const VideoFrame& previous);
    double calculate_histogram_difference(const VideoFrame& frame1, const VideoFrame& frame2);
    
    // Motion estimation for lookahead
    std::vector<FrameAnalysis::MotionVector> estimate_motion_vectors(
        const VideoFrame& current, const VideoFrame& reference);
    
    // Advanced analysis
    bool should_force_keyframe(const FrameAnalysis& analysis, 
                              const std::vector<FrameAnalysis>& previous_analyses);
    double calculate_recommended_quantizer(const FrameAnalysis& analysis);
    
    // Psychovisual analysis
    double calculate_perceptual_complexity(const VideoFrame& frame);
    std::vector<double> calculate_block_complexities(const VideoFrame& frame);
    
    // Noise analysis
    double estimate_noise_level(const VideoFrame& frame);
    
    // Fade detection
    std::pair<bool, bool> detect_fades(const VideoFrame& current, 
                                      const std::vector<VideoFrame>& history);
    
    // Statistics tracking
    mutable std::mutex m_stats_mutex;
    double m_total_complexity;
    uint64_t m_frames_analyzed;
    
    // Memory management
    void cleanup_old_frames();
    static constexpr size_t MAX_FRAME_BUFFER_SIZE = 100;
};