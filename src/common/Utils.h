#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <cstdint>

namespace Utils {
    // String utilities
    std::string trim(const std::string& str);
    std::vector<std::string> split(const std::string& str, char delimiter);
    std::string join(const std::vector<std::string>& parts, const std::string& delimiter);
    bool startsWith(const std::string& str, const std::string& prefix);
    bool endsWith(const std::string& str, const std::string& suffix);
    std::string toLowerCase(const std::string& str);
    std::string toUpperCase(const std::string& str);
    
    // URL utilities
    struct ParsedURL {
        std::string scheme;
        std::string host;
        int port;
        std::string path;
        std::string query;
        std::string fragment;
    };
    
    ParsedURL parseURL(const std::string& url);
    bool isValidURL(const std::string& url);
    
    // Time utilities
    std::string getCurrentTimeString();
    std::string formatDuration(std::chrono::milliseconds duration);
    uint64_t getCurrentTimestamp();
    
    // File utilities
    bool fileExists(const std::string& filename);
    std::string getFileExtension(const std::string& filename);
    std::string getFileName(const std::string& filepath);
    std::string getDirectoryPath(const std::string& filepath);
    bool createDirectory(const std::string& path);
    
    // Bitrate utilities
    std::string formatBitrate(int bitrate);
    int parseBitrate(const std::string& bitrateStr);
    
    // Network utilities
    bool isValidIPAddress(const std::string& ip);
    bool isValidPort(int port);
    std::string getLocalIPAddress();
    
    // Video utilities
    struct VideoResolution {
        int width;
        int height;
        
        VideoResolution(int w = 0, int h = 0) : width(w), height(h) {}
        bool isValid() const { return width > 0 && height > 0; }
        double aspectRatio() const { return (height > 0) ? (double)width / height : 0.0; }
    };
    
    VideoResolution parseResolution(const std::string& resStr);
    std::string formatResolution(const VideoResolution& res);
    
    // Math utilities
    double lerp(double a, double b, double t);
    double clamp(double value, double min, double max);
    int roundToNearest(double value);
    
    // Codec utilities
    bool isValidCodec(const std::string& codec);
    std::string getCodecMimeType(const std::string& codec);
    
    // Error handling
    std::string getLastErrorString();
    void setLastError(const std::string& error);
    
    // System utilities
    int getNumCPUCores();
    uint64_t getAvailableMemory();
    std::string getSystemInfo();
    
    // Hexadecimal utilities
    std::string bytesToHex(const std::vector<uint8_t>& bytes);
    std::vector<uint8_t> hexToBytes(const std::string& hex);
    
    // Random utilities
    std::string generateRandomString(size_t length);
    int generateRandomNumber(int min, int max);
}