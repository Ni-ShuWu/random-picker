#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <functional>

namespace picker {

using StringMap = std::unordered_map<std::string, std::string>;

struct Person {
    StringMap fields;
    std::size_t sourceRow = 0;
};

struct TableData {
    std::vector<std::string> headers;
    std::vector<Person> rows;
    std::size_t nameColumnIndex = static_cast<std::size_t>(-1);
    std::string sourceFile;

    bool empty() const noexcept { return rows.empty(); }
    std::size_t size() const noexcept { return rows.size(); }
    std::string nameOf(const Person& p) const;
};

struct PickResult {
    Person person;
    std::size_t pickIndex = 0;
    std::chrono::system_clock::time_point timestamp;
    std::string aiReason;
};

struct RunReport {
    std::string sourceFile;
    std::size_t totalCount = 0;
    std::size_t requestedCount = 0;
    std::chrono::system_clock::time_point runTime;
    std::vector<PickResult> picks;
    bool aiEnabled = false;
    std::string aiSummary;
};

inline std::string trim(const std::string& s) {
    auto begin = s.begin();
    auto end = s.rbegin();
    while (begin != s.end() && std::isspace(static_cast<unsigned char>(*begin))) ++begin;
    while (end != s.rend() && std::isspace(static_cast<unsigned char>(*end))) ++end;
    return std::string(begin, end.base());
}

inline std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

inline std::string timestampString(std::chrono::system_clock::time_point tp) {
    auto t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

inline std::string escapeCSV(const std::string& v) {
    bool needQuote = v.find_first_of(",\"\n\r") != std::string::npos;
    if (!needQuote) return v;
    std::string out;
    out.reserve(v.size() + 2);
    out.push_back('"');
    for (char c : v) {
        if (c == '"') out.push_back('"');
        out.push_back(c);
    }
    out.push_back('"');
    return out;
}

}
