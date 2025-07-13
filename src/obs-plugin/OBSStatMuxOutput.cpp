#include "OBSStatMuxOutput.h"
#include "common/Logger.h"
#include <thread>

OBSStatMuxOutput::OBSStatMuxOutput(std::shared_ptr<StatMuxEngine> engine) : m_statmuxEngine(engine), m_active(false) {}

OBSStatMuxOutput::~OBSStatMuxOutput() { stop(); }

bool OBSStatMuxOutput::initialize(const std::string& outputUrl) {
    m_outputUrl = outputUrl;
    m_videoOutputBuffer = std::make_unique<VideoFrameBuffer>(100);
    m_audioOutputBuffer = std::make_unique<AudioFrameBuffer>(100);
    Logger::info("OBS StatMux Output initialized: " + outputUrl);
    return true;
}

bool OBSStatMuxOutput::start() {
    m_active = true;
    m_outputThread = std::thread(&OBSStatMuxOutput::outputLoop, this);
    Logger::info("OBS StatMux Output started");
    return true;
}

bool OBSStatMuxOutput::stop() {
    m_active = false;
    if (m_outputThread.joinable()) {
        m_outputThread.join();
    }
    Logger::info("OBS StatMux Output stopped");
    return true;
}

bool OBSStatMuxOutput::processVideoFrame(const VideoFrame& frame) {
    std::lock_guard<std::mutex> lock(m_outputMutex);
    return m_videoOutputBuffer->write(frame);
}

bool OBSStatMuxOutput::processAudioFrame(const AudioFrame& frame) {
    std::lock_guard<std::mutex> lock(m_outputMutex);
    return m_audioOutputBuffer->write(frame);
}

bool OBSStatMuxOutput::isActive() const {
    return m_active.load();
}

std::string OBSStatMuxOutput::getStatus() const {
    return "OBS StatMux Output: " + (m_active ? "Active" : "Inactive");
}

void OBSStatMuxOutput::outputLoop() {
    while (m_active) {
        // Process output frames
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}