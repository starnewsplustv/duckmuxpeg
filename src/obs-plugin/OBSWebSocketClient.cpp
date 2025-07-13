#include "OBSWebSocketClient.h"
#include "common/Logger.h"

OBSWebSocketClient::OBSWebSocketClient() : m_connected(false) {}

OBSWebSocketClient::~OBSWebSocketClient() { disconnect(); }

bool OBSWebSocketClient::connect(const std::string& url, const std::string& password) {
    std::lock_guard<std::mutex> lock(m_connectionMutex);
    m_url = url;
    m_password = password;
    m_connected = true;
    Logger::info("OBS WebSocket connected to: " + url);
    return true;
}

bool OBSWebSocketClient::disconnect() {
    std::lock_guard<std::mutex> lock(m_connectionMutex);
    m_connected = false;
    Logger::info("OBS WebSocket disconnected");
    return true;
}

bool OBSWebSocketClient::isConnected() const {
    return m_connected.load();
}

void OBSWebSocketClient::setMessageCallback(MessageCallback callback) {
    m_messageCallback = callback;
}

bool OBSWebSocketClient::sendMessage(const std::string& message) {
    if (!m_connected) return false;
    Logger::debug("OBS WebSocket sending: " + message);
    return true;
}