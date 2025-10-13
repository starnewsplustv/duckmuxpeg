#pragma once

#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <queue>
#include <functional>
#include <map>
#include <optional>

#include "common/Config.h"
#include "common/Logger.h"
#include "common/CircularBuffer.h"
#include "InputHandler.h"
#include "OutputHandler.h"
#include "BitrateController.h"
#include "StreamProcessor.h"
#include "QualityAnalyzer.h"
#include "TransportStream.h"
#include "PlaylistManager.h"
#include "TrafficAccountant.h"
#include "LiveSourceScheduler.h"
#include "PlatformManager.h"

struct StreamStats {
    std::string streamId;
    int currentBitrate;
    int targetBitrate;
    int minBitrate;
    int maxBitrate;
    double quality;
    double complexity;
    size_t frameCount;
    size_t droppedFrames;
    double averageFrameTime;
    std::chrono::high_resolution_clock::time_point lastUpdate;
};

struct MuxStats {
    size_t totalInputStreams;
    size_t activeStreams;
    int totalBitrate;
    int availableBitrate;
    double muxEfficiency;
    size_t totalFrames;
    size_t droppedFrames;
    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::milliseconds uptime;
};

class StatMuxEngine {
public:
    explicit StatMuxEngine(const Config& config);
    ~StatMuxEngine();
    
    // Core engine operations
    bool initialize();
    bool start();
    bool stop();
    bool isRunning() const;
    
    // Stream management
    bool addInputStream(const InputConfig& config);
    bool removeInputStream(const std::string& streamId);
    bool addOutputStream(const OutputConfig& config);
    bool removeOutputStream(const std::string& streamId);
    
    // Runtime control
    bool updateBitrate(const std::string& streamId, int newBitrate);
    bool pauseStream(const std::string& streamId);
    bool resumeStream(const std::string& streamId);
    bool adjustQuality(const std::string& streamId, double qualityFactor);
    
    // Statistics and monitoring
    std::vector<StreamStats> getStreamStats() const;
    MuxStats getMuxStats() const;
    bool enableStatistics(bool enable);
    
    // Configuration updates
    bool updateConfig(const StatMuxConfig& config);
    void setOBSIntegration(bool enabled);
    
    // Event callbacks
    typedef std::function<void(const std::string&, const std::string&)> EventCallback;
    void setEventCallback(EventCallback callback);
    
    // Advanced features
    bool enableAdaptiveGOP(bool enable);
    bool enableSceneChangeDetection(bool enable);
    bool setMaxConcurrentStreams(int maxStreams);
    
private:
    // Core components
    std::unique_ptr<InputHandler> m_inputHandler;
    std::unique_ptr<OutputHandler> m_outputHandler;
    std::unique_ptr<BitrateController> m_bitrateController;
    std::unique_ptr<StreamProcessor> m_streamProcessor;
    std::unique_ptr<QualityAnalyzer> m_qualityAnalyzer;
    std::unique_ptr<TransportStream> m_transportStream;
    
    // Configuration
    Config m_config;
    StatMuxConfig m_statmuxConfig;
    
    // Threading
    std::thread m_mainThread;
    std::thread m_statisticsThread;
    std::atomic<bool> m_running;
    std::atomic<bool> m_initialized;
    
    // Synchronization
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    
    // Statistics
    std::atomic<bool> m_statisticsEnabled;
    mutable std::mutex m_statsLock;
    std::vector<StreamStats> m_streamStats;
    MuxStats m_muxStats;
    
    // Event handling
    EventCallback m_eventCallback;
    std::queue<std::pair<std::string, std::string>> m_eventQueue;
    std::mutex m_eventMutex;
    
    // Internal methods
    void mainLoop();
    void statisticsLoop();
    void processFrame(const VideoFrame& frame, const std::string& streamId);
    void updateStreamStatistics();
    void handleStreamEvent(const std::string& event, const std::string& streamId);
    void optimizeBitrates();
    bool validateConfiguration() const;
    void managePlayout();
    std::string activateStreamBySource(const std::string& sourceUrl);
    void pauseOtherStreams(const std::string& activeStreamId);
    void updateMultiPlatformOutputs();

    // Stream management internals
    struct StreamContext {
        std::string id;
        InputConfig inputConfig;
        std::unique_ptr<VideoFrameBuffer> frameBuffer;
        std::unique_ptr<AudioFrameBuffer> audioBuffer;
        std::atomic<bool> active;
        std::atomic<bool> paused;
        std::atomic<int> currentBitrate;
        std::atomic<double> quality;
        std::chrono::high_resolution_clock::time_point lastFrameTime;
        size_t frameCount;
        size_t droppedFrames;
    };
    
    std::vector<std::unique_ptr<StreamContext>> m_streamContexts;
    std::mutex m_streamMutex;
    
    // Output management
    struct OutputContext {
        std::string id;
        OutputConfig config;
        std::unique_ptr<std::thread> outputThread;
        std::atomic<bool> active;
    };
    
    std::vector<std::unique_ptr<OutputContext>> m_outputContexts;
    std::mutex m_outputMutex;

    // Performance monitoring
    std::chrono::high_resolution_clock::time_point m_lastOptimization;
    std::chrono::milliseconds m_optimizationInterval;

    // Advanced playout components
    std::unique_ptr<PlaylistManager> m_playlistManager;
    std::unique_ptr<TrafficAccountant> m_trafficAccountant;
    std::unique_ptr<LiveSourceScheduler> m_liveScheduler;
    std::unique_ptr<PlatformManager> m_platformManager;
    std::optional<PlaylistRuntimeItem> m_activePlaylistItem;
    std::optional<LiveSlotState> m_activeLiveSlot;
    std::chrono::system_clock::time_point m_activeItemStart;
    bool m_multiPlatformSynchronized;
    std::map<std::string, std::string> m_platformOutputMap;

    // Resource management
    void cleanup();
    bool allocateResources();
    void releaseResources();
};