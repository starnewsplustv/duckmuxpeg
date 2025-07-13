#include "Utils.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <regex>
#include <cmath>
#include <random>
#include <thread>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace Utils {
    
    static std::string lastError = "";
    
    // String utilities
    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(' ');
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(' ');
        return str.substr(first, (last - first + 1));
    }
    
    std::vector<std::string> split(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(str);
        
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        
        return tokens;
    }
    
    std::string join(const std::vector<std::string>& parts, const std::string& delimiter) {
        std::ostringstream oss;
        for (size_t i = 0; i < parts.size(); ++i) {
            if (i > 0) oss << delimiter;
            oss << parts[i];
        }
        return oss.str();
    }
    
    bool startsWith(const std::string& str, const std::string& prefix) {
        return str.length() >= prefix.length() && 
               str.compare(0, prefix.length(), prefix) == 0;
    }
    
    bool endsWith(const std::string& str, const std::string& suffix) {
        return str.length() >= suffix.length() && 
               str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
    }
    
    std::string toLowerCase(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }
    
    std::string toUpperCase(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        return result;
    }
    
    // URL utilities
    ParsedURL parseURL(const std::string& url) {
        ParsedURL parsed;
        
        std::regex urlRegex(R"(^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?)",
                           std::regex::extended);
        std::smatch matches;
        
        if (std::regex_match(url, matches, urlRegex)) {
            parsed.scheme = matches[2].str();
            std::string authority = matches[4].str();
            parsed.path = matches[5].str();
            parsed.query = matches[7].str();
            parsed.fragment = matches[9].str();
            
            // Parse host and port from authority
            if (!authority.empty()) {
                size_t colonPos = authority.find(':');
                if (colonPos != std::string::npos) {
                    parsed.host = authority.substr(0, colonPos);
                    try {
                        parsed.port = std::stoi(authority.substr(colonPos + 1));
                    } catch (...) {
                        parsed.port = 0;
                    }
                } else {
                    parsed.host = authority;
                    parsed.port = 0;
                }
            }
        }
        
        return parsed;
    }
    
    bool isValidURL(const std::string& url) {
        ParsedURL parsed = parseURL(url);
        return !parsed.scheme.empty() && !parsed.host.empty();
    }
    
    // Time utilities
    std::string getCurrentTimeString() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }
    
    std::string formatDuration(std::chrono::milliseconds duration) {
        auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration % std::chrono::hours(1));
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration % std::chrono::minutes(1));
        auto ms = duration % std::chrono::seconds(1);
        
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(2) << hours.count() << ":"
            << std::setw(2) << minutes.count() << ":"
            << std::setw(2) << seconds.count() << "."
            << std::setw(3) << ms.count();
        
        return oss.str();
    }
    
    uint64_t getCurrentTimestamp() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
    
    // File utilities
    bool fileExists(const std::string& filename) {
        struct stat buffer;
        return (stat(filename.c_str(), &buffer) == 0);
    }
    
    std::string getFileExtension(const std::string& filename) {
        size_t pos = filename.find_last_of('.');
        if (pos != std::string::npos) {
            return filename.substr(pos + 1);
        }
        return "";
    }
    
    std::string getFileName(const std::string& filepath) {
        size_t pos = filepath.find_last_of('/');
        if (pos != std::string::npos) {
            return filepath.substr(pos + 1);
        }
        return filepath;
    }
    
    std::string getDirectoryPath(const std::string& filepath) {
        size_t pos = filepath.find_last_of('/');
        if (pos != std::string::npos) {
            return filepath.substr(0, pos);
        }
        return "";
    }
    
    bool createDirectory(const std::string& path) {
        return mkdir(path.c_str(), 0755) == 0;
    }
    
    // Bitrate utilities
    std::string formatBitrate(int bitrate) {
        if (bitrate >= 1000000) {
            return std::to_string(bitrate / 1000000) + " Mbps";
        } else if (bitrate >= 1000) {
            return std::to_string(bitrate / 1000) + " kbps";
        } else {
            return std::to_string(bitrate) + " bps";
        }
    }
    
    int parseBitrate(const std::string& bitrateStr) {
        std::string str = toLowerCase(trim(bitrateStr));
        
        double value = 0.0;
        try {
            value = std::stod(str);
        } catch (...) {
            return 0;
        }
        
        if (endsWith(str, "mbps")) {
            return static_cast<int>(value * 1000000);
        } else if (endsWith(str, "kbps")) {
            return static_cast<int>(value * 1000);
        } else if (endsWith(str, "bps")) {
            return static_cast<int>(value);
        }
        
        return static_cast<int>(value);
    }
    
    // Network utilities
    bool isValidIPAddress(const std::string& ip) {
        struct sockaddr_in sa;
        return inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr)) != 0;
    }
    
    bool isValidPort(int port) {
        return port > 0 && port <= 65535;
    }
    
    std::string getLocalIPAddress() {
        struct ifaddrs *ifaddr, *ifa;
        char host[NI_MAXHOST];
        
        if (getifaddrs(&ifaddr) == -1) {
            return "";
        }
        
        for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == nullptr) continue;
            
            if (ifa->ifa_addr->sa_family == AF_INET) {
                int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                                   host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
                if (s == 0 && std::string(host) != "127.0.0.1") {
                    freeifaddrs(ifaddr);
                    return std::string(host);
                }
            }
        }
        
        freeifaddrs(ifaddr);
        return "";
    }
    
    // Video utilities
    VideoResolution parseResolution(const std::string& resStr) {
        auto parts = split(resStr, 'x');
        if (parts.size() == 2) {
            try {
                int width = std::stoi(parts[0]);
                int height = std::stoi(parts[1]);
                return VideoResolution(width, height);
            } catch (...) {
                // Fall through to invalid resolution
            }
        }
        return VideoResolution(0, 0);
    }
    
    std::string formatResolution(const VideoResolution& res) {
        return std::to_string(res.width) + "x" + std::to_string(res.height);
    }
    
    // Math utilities
    double lerp(double a, double b, double t) {
        return a + t * (b - a);
    }
    
    double clamp(double value, double min, double max) {
        return std::max(min, std::min(value, max));
    }
    
    int roundToNearest(double value) {
        return static_cast<int>(std::round(value));
    }
    
    // Codec utilities
    bool isValidCodec(const std::string& codec) {
        std::string lowerCodec = toLowerCase(codec);
        return lowerCodec == "h264" || lowerCodec == "h265" || lowerCodec == "vp8" || 
               lowerCodec == "vp9" || lowerCodec == "av1" || lowerCodec == "mpeg2";
    }
    
    std::string getCodecMimeType(const std::string& codec) {
        std::string lowerCodec = toLowerCase(codec);
        if (lowerCodec == "h264") return "video/avc";
        if (lowerCodec == "h265") return "video/hevc";
        if (lowerCodec == "vp8") return "video/vp8";
        if (lowerCodec == "vp9") return "video/vp9";
        if (lowerCodec == "av1") return "video/av1";
        if (lowerCodec == "mpeg2") return "video/mpeg2";
        return "video/unknown";
    }
    
    // Error handling
    std::string getLastErrorString() {
        return lastError;
    }
    
    void setLastError(const std::string& error) {
        lastError = error;
    }
    
    // System utilities
    int getNumCPUCores() {
        return std::thread::hardware_concurrency();
    }
    
    uint64_t getAvailableMemory() {
        struct sysinfo info;
        if (sysinfo(&info) == 0) {
            return info.freeram * info.mem_unit;
        }
        return 0;
    }
    
    std::string getSystemInfo() {
        std::ostringstream oss;
        oss << "CPU cores: " << getNumCPUCores() << ", ";
        oss << "Available memory: " << (getAvailableMemory() / (1024 * 1024)) << " MB";
        return oss.str();
    }
    
    // Hexadecimal utilities
    std::string bytesToHex(const std::vector<uint8_t>& bytes) {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (uint8_t byte : bytes) {
            oss << std::setw(2) << static_cast<int>(byte);
        }
        return oss.str();
    }
    
    std::vector<uint8_t> hexToBytes(const std::string& hex) {
        std::vector<uint8_t> bytes;
        for (size_t i = 0; i < hex.length(); i += 2) {
            std::string byteString = hex.substr(i, 2);
            uint8_t byte = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
            bytes.push_back(byte);
        }
        return bytes;
    }
    
    // Random utilities
    std::string generateRandomString(size_t length) {
        const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, chars.size() - 1);
        
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += chars[dis(gen)];
        }
        return result;
    }
    
    int generateRandomNumber(int min, int max) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(min, max);
        return dis(gen);
    }
}