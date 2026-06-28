#include "logger.h"

namespace picker {

void Log::openLogFile(const std::string& path) {
    std::lock_guard<std::mutex> lk(detail::logMutex());
    detail::logFileStream().open(path, std::ios::out | std::ios::app);
}

void Log::closeLogFile() {
    std::lock_guard<std::mutex> lk(detail::logMutex());
    if (detail::logFileStream().is_open()) detail::logFileStream().close();
}

}
