#pragma once

#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <future>
#include <memory>
#include <functional>
#include <map>
#include <chrono>

#include "common/CircularBuffer.h"

// Thread pool architecture optimized for high core count systems
class ParallelEncoder {
public:
    enum class ThreadStrategy {
        FRAME_PARALLEL,     // Each thread encodes complete frames
        SLICE_PARALLEL,     // Threads encode slices within frames
        PIPELINE_PARALLEL,  // Pipeline stages across threads
        HYBRID_PARALLEL,    // Combination approach
        NUMA_AWARE         // NUMA-optimized distribution
    };
    
    enum class LoadBalancing {
        ROUND_ROBIN,
        WORK_STEALING,
        COMPLEXITY_BASED,
        ADAPTIVE,
        NUMA_LOCALITY
    };
    
    struct ParallelConfig {
        int max_threads;
        int frame_threads;
        int slice_threads;
        int lookahead_threads;
        ThreadStrategy strategy;
        LoadBalancing load_balancing;
        
        // NUMA configuration
        bool numa_aware;
        std::vector<int> numa_nodes;
        bool bind_threads_to_cores;
        
        // Performance tuning
        int thread_priority;
        size_t stack_size_mb;
        bool use_hyperthreading;
        bool enable_turbo_boost;
        
        // Queue configurations
        size_t input_queue_size;
        size_t output_queue_size;
        size_t work_queue_size;
        
        // Synchronization
        bool enable_lock_free_queues;
        bool use_atomic_operations;
        int sync_interval_frames;
    };
    
    explicit ParallelEncoder(const ParallelConfig& config);
    ~ParallelEncoder();
    
    // Initialization and configuration
    bool initialize();
    bool configure_threads(int thread_count);
    bool set_thread_strategy(ThreadStrategy strategy);
    bool enable_numa_awareness(bool enabled);
    
    // Encoding operations
    std::future<std::vector<uint8_t>> encode_frame_async(const VideoFrame& frame);
    bool encode_frame_parallel(const VideoFrame& frame, std::vector<uint8_t>& output);
    bool encode_slice_parallel(const VideoFrame& frame, int slice_count, std::vector<uint8_t>& output);
    
    // Batch processing
    std::vector<std::future<std::vector<uint8_t>>> encode_batch_async(
        const std::vector<VideoFrame>& frames);
    bool encode_batch_parallel(const std::vector<VideoFrame>& frames,
                              std::vector<std::vector<uint8_t>>& outputs);
    
    // Work distribution
    bool submit_encoding_task(std::function<void()> task);
    bool submit_priority_task(std::function<void()> task, int priority);
    void wait_for_completion();
    void wait_for_frame_completion(uint64_t frame_number);
    
    // Thread management
    bool start_threads();
    bool stop_threads();
    bool pause_threads();
    bool resume_threads();
    
    // Performance monitoring
    struct ThreadStats {
        int thread_id;
        int cpu_core;
        int numa_node;
        uint64_t frames_processed;
        uint64_t total_processing_time_ms;
        double average_frame_time_ms;
        double cpu_utilization;
        size_t memory_usage_mb;
        uint64_t cache_misses;
        uint64_t context_switches;
    };
    
    struct ParallelStats {
        std::vector<ThreadStats> thread_stats;
        double overall_throughput_fps;
        double parallel_efficiency;
        double load_balance_factor;
        size_t peak_memory_usage_mb;
        int active_threads;
        uint64_t total_frames_processed;
        std::chrono::milliseconds total_encoding_time;
    };
    
    ParallelStats get_parallel_stats() const;
    ThreadStats get_thread_stats(int thread_id) const;
    
    // Load balancing
    bool enable_work_stealing(bool enabled);
    bool configure_load_balancing(LoadBalancing strategy);
    bool redistribute_work();
    
    // CPU affinity and NUMA
    bool set_thread_affinity(int thread_id, int cpu_core);
    bool bind_thread_to_numa_node(int thread_id, int numa_node);
    std::vector<int> get_optimal_cpu_assignment() const;
    
    // Memory management
    bool configure_memory_pools(size_t pool_size_mb);
    bool enable_zero_copy_buffers(bool enabled);
    bool prefetch_memory_patterns();
    
private:
    ParallelConfig m_config;
    
    // Thread management
    std::vector<std::thread> m_worker_threads;
    std::vector<std::thread> m_lookahead_threads;
    std::atomic<bool> m_threads_active;
    std::atomic<bool> m_shutdown_requested;
    
    // Work queues
    struct EncodingTask {
        uint64_t frame_number;
        VideoFrame frame;
        std::promise<std::vector<uint8_t>> result_promise;
        int priority;
        std::chrono::steady_clock::time_point submit_time;
        int target_thread_id; // For affinity-based distribution
    };
    
    // Different queue types for different strategies
    std::queue<EncodingTask> m_frame_queue;
    std::vector<std::queue<EncodingTask>> m_thread_queues; // Per-thread queues
    std::priority_queue<EncodingTask, std::vector<EncodingTask>, 
                       std::function<bool(const EncodingTask&, const EncodingTask&)>> m_priority_queue;
    
    // Synchronization
    std::mutex m_queue_mutex;
    std::condition_variable m_queue_cv;
    std::vector<std::mutex> m_thread_mutexes;
    std::vector<std::condition_variable> m_thread_cvs;
    
    // Work stealing support
    std::atomic<bool> m_work_stealing_enabled;
    std::vector<std::atomic<int>> m_thread_work_counts;
    
    // NUMA support
    struct NumaNode {
        int node_id;
        std::vector<int> cpu_cores;
        std::vector<int> thread_ids;
        size_t memory_size_mb;
        bool has_local_memory;
    };
    
    std::vector<NumaNode> m_numa_nodes;
    std::map<int, int> m_thread_to_numa;
    
    // Statistics tracking
    mutable std::mutex m_stats_mutex;
    std::vector<ThreadStats> m_thread_stats;
    std::atomic<uint64_t> m_total_frames_processed;
    std::chrono::steady_clock::time_point m_start_time;
    
    // Memory pools
    class MemoryPool {
    public:
        MemoryPool(size_t pool_size, size_t block_size);
        ~MemoryPool();
        
        void* allocate();
        void deallocate(void* ptr);
        size_t get_available_blocks() const;
        
    private:
        std::vector<uint8_t> m_pool;
        std::vector<bool> m_allocation_map;
        std::mutex m_pool_mutex;
        size_t m_block_size;
        size_t m_total_blocks;
    };
    
    std::vector<std::unique_ptr<MemoryPool>> m_memory_pools;
    
    // Thread worker functions
    void frame_parallel_worker(int thread_id);
    void slice_parallel_worker(int thread_id);
    void pipeline_parallel_worker(int thread_id);
    void hybrid_parallel_worker(int thread_id);
    void numa_aware_worker(int thread_id);
    
    // Work distribution algorithms
    int get_next_available_thread();
    int get_least_loaded_thread();
    int get_complexity_based_thread(const VideoFrame& frame);
    int get_numa_optimal_thread(const VideoFrame& frame);
    
    // Work stealing implementation
    bool steal_work(int stealing_thread_id);
    EncodingTask* try_steal_from_thread(int target_thread_id);
    
    // CPU and NUMA detection
    bool detect_cpu_topology();
    bool detect_numa_topology();
    std::vector<int> get_cpu_cores_for_numa_node(int numa_node) const;
    
    // Performance optimization
    bool optimize_thread_scheduling();
    bool tune_thread_priorities();
    bool configure_cpu_governor();
    
    // Synchronization helpers
    void wait_for_sync_point(uint64_t frame_number);
    void signal_frame_completion(uint64_t frame_number);
    
    // Slice-based encoding helpers
    struct SliceInfo {
        int slice_id;
        int start_mb_row;
        int end_mb_row;
        int thread_id;
    };
    
    std::vector<SliceInfo> calculate_slice_distribution(int height, int slice_count);
    bool encode_slice(const VideoFrame& frame, const SliceInfo& slice, 
                     std::vector<uint8_t>& slice_data);
    bool merge_slice_outputs(const std::vector<std::vector<uint8_t>>& slice_outputs,
                            std::vector<uint8_t>& final_output);
    
    // Frame dependency management
    struct FrameDependency {
        uint64_t frame_number;
        std::vector<uint64_t> dependent_frames;
        std::atomic<bool> ready_for_encoding;
        std::atomic<int> dependency_count;
    };
    
    std::map<uint64_t, FrameDependency> m_frame_dependencies;
    std::mutex m_dependency_mutex;
    
    void add_frame_dependency(uint64_t frame, uint64_t depends_on);
    void signal_frame_ready(uint64_t frame_number);
    bool is_frame_ready_for_encoding(uint64_t frame_number);
    
    // Cache optimization
    bool optimize_memory_access_patterns();
    bool configure_cache_prefetching();
    void prefetch_frame_data(const VideoFrame& frame);
    
    // Error handling and recovery
    void handle_thread_error(int thread_id, const std::exception& e);
    bool restart_failed_thread(int thread_id);
    void emergency_shutdown();
    
    // Debug and profiling support
    bool m_profiling_enabled;
    void start_profiling();
    void stop_profiling();
    void dump_performance_profile(const std::string& filename);
};