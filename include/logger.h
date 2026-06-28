#pragma once

#include <string>
#include <sstream>
#include <mutex>
#include <fstream>
#include <iostream>
#include <utility>

namespace picker {

enum class LogLevel { Debug = 0, Info = 1, Warn = 2, Error = 3 };

namespace detail {

inline std::mutex& logMutex() {
    static std::mutex m;
    return m;
}
inline LogLevel& levelRef() {
    static LogLevel lvl = LogLevel::Info;
    return lvl;
}
inline std::ofstream& logFileStream() {
    static std::ofstream s;
    return s;
}
inline void writePrefix(std::ostringstream& oss, LogLevel level) {
    const char* tag = "INFO";
    switch (level) {
        case LogLevel::Debug: tag = "DEBUG"; break;
        case LogLevel::Info:  tag = "INFO";  break;
        case LogLevel::Warn:  tag = "WARN";  break;
        case LogLevel::Error: tag = "ERROR"; break;
    }
    oss << '[' << tag << "] ";
}

template <typename T>
inline void writeOne(std::ostringstream& oss, const T& v) { oss << v; }

inline void consumeFmt(std::ostringstream& oss, const std::string& fmt) {
    for (char c : fmt) {
        oss << c;
    }
}

template <typename T, typename... Rest>
void consumeFmt(std::ostringstream& oss, const std::string& fmt,
                T&& first, Rest&&... rest);

template <typename T>
void formatRecursive(std::ostringstream& oss, const std::string& fmt,
                     T&& arg) {
    std::size_t pos = 0;
    while (pos < fmt.size()) {
        if (pos + 1 < fmt.size() && fmt[pos] == '{' && fmt[pos + 1] == '}') {
            writeOne(oss, arg);
            pos += 2;
            while (pos < fmt.size()) oss << fmt[pos++];
            return;
        }
        oss << fmt[pos++];
    }
}

template <typename T, typename... Rest>
void formatRecursive(std::ostringstream& oss, const std::string& fmt,
                     T&& first, Rest&&... rest) {
    std::size_t pos = 0;
    while (pos < fmt.size()) {
        if (pos + 1 < fmt.size() && fmt[pos] == '{' && fmt[pos + 1] == '}') {
            writeOne(oss, first);
            pos += 2;
            std::string rest_fmt = fmt.substr(pos);
            formatRecursive(oss, rest_fmt, std::forward<Rest>(rest)...);
            return;
        }
        oss << fmt[pos++];
    }
}

template <typename T, typename... Rest>
void consumeFmt(std::ostringstream& oss, const std::string& fmt,
                T&& first, Rest&&... rest) {
    formatRecursive(oss, fmt, std::forward<T>(first), std::forward<Rest>(rest)...);
}

}

class Log {
public:
    template <typename... Args>
    static void log(LogLevel level, const std::string& fmt, Args&&... args) {
        if (static_cast<int>(level) < static_cast<int>(detail::levelRef())) return;
        std::ostringstream oss;
        detail::writePrefix(oss, level);
        if constexpr (sizeof...(args) == 0) {
            detail::consumeFmt(oss, fmt);
        } else {
            detail::consumeFmt(oss, fmt, std::forward<Args>(args)...);
        }
        std::string s = oss.str();
        {
            std::lock_guard<std::mutex> lk(detail::logMutex());
            std::ostream& out = (level == LogLevel::Error) ? std::cerr : std::cout;
            out << s << '\n';
            if (detail::logFileStream().is_open()) detail::logFileStream() << s << '\n';
        }
    }

    template <typename... Args>
    static void debug(const std::string& fmt, Args&&... args) {
        log(LogLevel::Debug, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    static void info(const std::string& fmt, Args&&... args) {
        log(LogLevel::Info, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    static void warn(const std::string& fmt, Args&&... args) {
        log(LogLevel::Warn, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    static void error(const std::string& fmt, Args&&... args) {
        log(LogLevel::Error, fmt, std::forward<Args>(args)...);
    }

    static void setLevel(LogLevel level) { detail::levelRef() = level; }
    static LogLevel currentLevel() { return detail::levelRef(); }

    static void openLogFile(const std::string& path);
    static void closeLogFile();
};

}
