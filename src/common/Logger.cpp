#include "Logger.h"
#include <iostream>
#include <unistd.h>

// Static member definitions
std::mutex Logger::m_mutex;
LogLevel Logger::m_level = LogLevel::INFO;
std::string Logger::m_appName = "DuckMuxPeg";
std::ofstream Logger::m_logFile;
bool Logger::m_useFile = false;
bool Logger::m_initialized = false;

void Logger::initialize(const std::string& appName, LogLevel level, const std::string& logFile) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_appName = appName;
    m_level = level;
    m_initialized = true;
    
    if (!logFile.empty()) {
        m_logFile.open(logFile, std::ios::app);
        if (m_logFile.is_open()) {
            m_useFile = true;
        }
    }
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warn(const std::string& message) {
    log(LogLevel::WARN, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::fatal(const std::string& message) {
    log(LogLevel::FATAL, message);
}

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_level = level;
}

LogLevel Logger::getLevel() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_level;
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_useFile && m_logFile.is_open()) {
        m_logFile.close();
    }
    m_initialized = false;
}

void Logger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized || level < m_level) {
        return;
    }
    
    std::string timestamp = getCurrentTime();
    std::string levelStr = levelToString(level);
    
    std::ostringstream oss;
    oss << "[" << timestamp << "] [" << m_appName << "] [" << levelStr << "] " << message;
    
    std::string logLine = oss.str();
    
    // Console output with colors
    bool isTerminal = isatty(STDOUT_FILENO);
    if (isTerminal) {
        std::cout << getColorCode(level) << logLine << getResetCode() << std::endl;
    } else {
        std::cout << logLine << std::endl;
    }
    
    // File output
    if (m_useFile && m_logFile.is_open()) {
        m_logFile << logLine << std::endl;
        m_logFile.flush();
    }
}

std::string Logger::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    return oss.str();
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

std::string Logger::getColorCode(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "\033[36m";  // Cyan
        case LogLevel::INFO:  return "\033[32m";  // Green
        case LogLevel::WARN:  return "\033[33m";  // Yellow
        case LogLevel::ERROR: return "\033[31m";  // Red
        case LogLevel::FATAL: return "\033[35m";  // Magenta
        default: return "";
    }
}

std::string Logger::getResetCode() {
    return "\033[0m";
}