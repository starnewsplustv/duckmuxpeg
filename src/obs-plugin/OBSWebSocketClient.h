#pragma once

#include <string>
#include <atomic>
#include <mutex>
#include <functional>

class OBSWebSocketClient {
public:
    OBSWebSocketClient();
    ~OBSWebSocketClient();
    
    bool connect(const std::string& url, const std::string& password);
    bool disconnect();
    bool isConnected() const;
    
    typedef std::function<void(const std::string&)> MessageCallback;
    void setMessageCallback(MessageCallback callback);
    
    bool sendMessage(const std::string& message);
    
private:
    std::string m_url;
    std::string m_password;
    std::atomic<bool> m_connected;
    std::mutex m_connectionMutex;
    MessageCallback m_messageCallback;
};