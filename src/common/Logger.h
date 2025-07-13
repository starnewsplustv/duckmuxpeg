#pragma once

#include <string>
#include <mutex>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4
};

class Logger {
public:
    static void initialize(const std::string& appName, 
                          LogLevel level = LogLevel::INFO, 
                          const std::string& logFile = "");
    
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);
    static void fatal(const std::string& message);
    
    static void setLevel(LogLevel level);
    static LogLevel getLevel();
    
    static void shutdown();

private:
    static std::mutex m_mutex;
    static LogLevel m_level;
    static std::string m_appName;
    static std::ofstream m_logFile;
    static bool m_useFile;
    static bool m_initialized;
    
    static void log(LogLevel level, const std::string& message);
    static std::string getCurrentTime();
    static std::string levelToString(LogLevel level);
    static std::string getColorCode(LogLevel level);
    static std::string getResetCode();
};

// Convenience macros
#define LOG_DEBUG(msg) Logger::debug(msg)
#define LOG_INFO(msg) Logger::info(msg)
#define LOG_WARN(msg) Logger::warn(msg)
#define LOG_ERROR(msg) Logger::error(msg)
#define LOG_FATAL(msg) Logger::fatal(msg)