#pragma once

#include <memory>
#include <vector>
#include <string>
#include <atomic>
#include <mutex>
#include <thread>
#include <fstream>
#include <unordered_map>
#include <future>
#include <queue>
#include <functional>

#include "common/CircularBuffer.h"

enum class BufferType {
    RAM_ONLY,
    RAMDRIVE,
    HYBRID,
    NUMA_AWARE
};

struct BufferStats {
    size_t total_size_mb;
    size_t used_size_mb;
    size_t free_size_mb;
    double utilization_percent;
    uint64_t read_operations;
    uint64_t write_operations;
    uint64_t read_bytes;
    uint64_t write_bytes;
    double average_read_speed_mbps;
    double average_write_speed_mbps;
    std::chrono::milliseconds average_access_time;
    size_t cache_hits;
    size_t cache_misses;
    double cache_hit_ratio;
};

class RAMBufferManager {
public:
    explicit RAMBufferManager(BufferType type = BufferType::RAM_ONLY);
    ~RAMBufferManager();
    
    // Initialization
    bool initialize(size_t total_size_mb);
    bool setup_ramdrive(const std::string& mount_point, size_t size_mb);
    bool configure_numa_awareness(int preferred_node = -1);
    
    // Buffer operations
    bool allocate_buffer(const std::string& buffer_id, size_t size_mb);
    bool deallocate_buffer(const std::string& buffer_id);
    bool resize_buffer(const std::string& buffer_id, size_t new_size_mb);
    
    // Data operations
    bool write_frame(const std::string& buffer_id, const VideoFrame& frame);
    bool read_frame(const std::string& buffer_id, VideoFrame& frame);
    bool write_data(const std::string& buffer_id, const void* data, size_t size);
    bool read_data(const std::string& buffer_id, void* data, size_t size);
    
    // Advanced operations
    bool write_frame_async(const std::string& buffer_id, const VideoFrame& frame);
    std::future<bool> read_frame_async(const std::string& buffer_id);
    bool prefetch_frames(const std::string& buffer_id, size_t count);
    bool flush_buffer(const std::string& buffer_id);
    
    // Memory mapping for zero-copy
    void* map_buffer(const std::string& buffer_id, size_t offset, size_t size);
    bool unmap_buffer(const std::string& buffer_id, void* ptr);
    
    // Cache management
    bool enable_write_cache(bool enabled, size_t cache_size_mb = 256);
    bool enable_read_cache(bool enabled, size_t cache_size_mb = 256);
    bool configure_cache_policy(const std::string& policy); // "LRU", "LFU", "FIFO"
    
    // Compression
    bool enable_compression(const std::string& algorithm = "lz4"); // "lz4", "zstd", "none"
    bool set_compression_level(int level);
    
    // Statistics and monitoring
    BufferStats get_stats() const;
    BufferStats get_buffer_stats(const std::string& buffer_id) const;
    std::vector<std::string> get_buffer_list() const;
    
    // Performance tuning
    bool set_io_priority(int priority);
    bool enable_direct_io(bool enabled);
    bool configure_prefetch_strategy(const std::string& strategy);
    
    // NUMA optimization
    bool pin_to_numa_node(int node);
    std::vector<int> get_numa_topology() const;
    bool optimize_numa_allocation();
    
    // Ramdrive management
    bool create_ramdrive(const std::string& path, size_t size_mb);
    bool destroy_ramdrive(const std::string& path);
    bool resize_ramdrive(const std::string& path, size_t new_size_mb);
    std::vector<std::string> list_ramdrives() const;
    
    // Error handling
    std::string get_last_error() const;
    bool is_healthy() const;
    
private:
    struct BufferEntry {
        std::string id;
        size_t size_mb;
        void* memory_ptr;
        std::string file_path;
        bool is_mapped;
        std::mutex access_mutex;
        
        // Statistics
        uint64_t read_count;
        uint64_t write_count;
        std::chrono::high_resolution_clock::time_point last_access;
        
        // NUMA info
        int numa_node;
        bool numa_allocated;
    };
    
    BufferType m_buffer_type;
    size_t m_total_size_mb;
    std::string m_ramdrive_path;
    
    // Buffer management
    std::unordered_map<std::string, std::unique_ptr<BufferEntry>> m_buffers;
    std::mutex m_buffers_mutex;
    
    // Memory allocation
    void* m_base_memory;
    size_t m_allocated_size;
    std::vector<bool> m_allocation_map;
    std::mutex m_allocation_mutex;
    
    // Cache system
    struct CacheEntry {
        std::string buffer_id;
        size_t offset;
        std::vector<uint8_t> data;
        std::chrono::high_resolution_clock::time_point last_access;
        size_t access_count;
    };
    
    std::unordered_map<std::string, CacheEntry> m_read_cache;
    std::unordered_map<std::string, CacheEntry> m_write_cache;
    std::mutex m_cache_mutex;
    bool m_read_cache_enabled;
    bool m_write_cache_enabled;
    size_t m_cache_size_mb;
    std::string m_cache_policy;
    
    // Compression
    bool m_compression_enabled;
    std::string m_compression_algorithm;
    int m_compression_level;
    
    // Statistics
    mutable std::mutex m_stats_mutex;
    BufferStats m_stats;
    
    // Threading for async operations
    std::vector<std::thread> m_worker_threads;
    std::queue<std::function<void()>> m_task_queue;
    std::mutex m_task_mutex;
    std::condition_variable m_task_cv;
    std::atomic<bool> m_workers_active;
    
    // NUMA support
    int m_numa_node;
    bool m_numa_aware;
    std::vector<int> m_numa_nodes;
    
    // Internal methods
    bool allocate_memory_block(size_t size, void** ptr, int numa_node = -1);
    bool deallocate_memory_block(void* ptr, size_t size);
    
    bool setup_ramdrive_internal(const std::string& path, size_t size_mb);
    bool create_file_backed_buffer(const std::string& buffer_id, size_t size_mb);
    
    // Cache management
    void evict_cache_entries();
    bool cache_read(const std::string& key, std::vector<uint8_t>& data);
    void cache_write(const std::string& key, const std::vector<uint8_t>& data);
    
    // Compression helpers
    std::vector<uint8_t> compress_data(const std::vector<uint8_t>& input);
    std::vector<uint8_t> decompress_data(const std::vector<uint8_t>& input);
    
    // NUMA helpers
    bool detect_numa_topology();
    int get_optimal_numa_node(size_t size);
    bool bind_memory_to_node(void* ptr, size_t size, int node);
    
    // Performance monitoring
    void update_statistics(const std::string& operation, size_t bytes, 
                          std::chrono::milliseconds duration);
    
    // Worker thread management
    void start_worker_threads(size_t thread_count = 4);
    void stop_worker_threads();
    void worker_thread_function();
    
    // Error handling
    mutable std::string m_last_error;
    void set_error(const std::string& error);
    
    // Platform-specific implementations
    #ifdef __linux__
    bool setup_hugepages(size_t size);
    bool create_tmpfs_ramdrive(const std::string& path, size_t size_mb);
    #endif
    
    #ifdef _WIN32
    bool create_ramdisk_windows(const std::string& path, size_t size_mb);
    #endif
};