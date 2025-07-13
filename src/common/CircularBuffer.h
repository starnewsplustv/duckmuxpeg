#pragma once

#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <chrono>

template<typename T>
class CircularBuffer {
public:
    explicit CircularBuffer(size_t capacity);
    ~CircularBuffer();
    
    // Write operations
    bool write(const T& item);
    bool write(T&& item);
    bool writeWithTimeout(const T& item, std::chrono::milliseconds timeout);
    
    // Read operations
    bool read(T& item);
    bool readWithTimeout(T& item, std::chrono::milliseconds timeout);
    
    // Peek operations (non-destructive read)
    bool peek(T& item);
    bool peekAt(size_t index, T& item);
    
    // Status operations
    size_t size() const;
    size_t capacity() const;
    bool empty() const;
    bool full() const;
    
    // Control operations
    void clear();
    void reset();
    void setBlocking(bool blocking);
    
    // Statistics
    size_t getWriteCount() const;
    size_t getReadCount() const;
    size_t getDropCount() const;

private:
    std::vector<T> m_buffer;
    size_t m_capacity;
    size_t m_head;
    size_t m_tail;
    std::atomic<size_t> m_size;
    
    mutable std::mutex m_mutex;
    std::condition_variable m_readCondition;
    std::condition_variable m_writeCondition;
    
    std::atomic<bool> m_blocking;
    std::atomic<size_t> m_writeCount;
    std::atomic<size_t> m_readCount;
    std::atomic<size_t> m_dropCount;
    
    size_t nextIndex(size_t index) const;
};

// Video frame data structure for use with CircularBuffer
struct VideoFrame {
    std::vector<uint8_t> data;
    size_t width;
    size_t height;
    size_t pitch;
    std::chrono::high_resolution_clock::time_point timestamp;
    uint64_t frameNumber;
    bool isKeyFrame;
    
    VideoFrame() : width(0), height(0), pitch(0), frameNumber(0), isKeyFrame(false) {}
    VideoFrame(size_t w, size_t h, size_t p) : width(w), height(h), pitch(p), frameNumber(0), isKeyFrame(false) {
        data.resize(h * p);
        timestamp = std::chrono::high_resolution_clock::now();
    }
};

// Audio frame data structure
struct AudioFrame {
    std::vector<uint8_t> data;
    size_t sampleRate;
    size_t channels;
    size_t samplesPerFrame;
    std::chrono::high_resolution_clock::time_point timestamp;
    uint64_t frameNumber;
    
    AudioFrame() : sampleRate(0), channels(0), samplesPerFrame(0), frameNumber(0) {}
    AudioFrame(size_t sr, size_t ch, size_t spf) : sampleRate(sr), channels(ch), samplesPerFrame(spf), frameNumber(0) {
        data.resize(spf * ch * sizeof(float));
        timestamp = std::chrono::high_resolution_clock::now();
    }
};

// Convenience typedefs
using VideoFrameBuffer = CircularBuffer<VideoFrame>;
using AudioFrameBuffer = CircularBuffer<AudioFrame>;