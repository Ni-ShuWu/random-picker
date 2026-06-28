#pragma once

#include "common.h"
#include "ai.h"
#include <string>
#include <vector>

namespace picker {

struct CliOptions {
    std::string filePath;
    std::size_t count = 1;
    bool showHelp = false;
    bool showVersion = false;
    bool noGui = false;
    bool verbose = false;
    bool noLogo = false;
    bool aiEnabled = false;
    AiConfig ai;
    std::string outputFile;
    std::uint64_t seed = 0;
    bool seedGiven = false;
};

class ArgParser {
public:
    static CliOptions parse(int argc, char** argv);
    static void printHelp(const std::string& exeName);
    static void printVersion();
};

class Reporter {
public:
    static std::string toText(const RunReport& rep, const TableData& table);
    static bool saveCsv(const std::string& path, const RunReport& rep, const TableData& table);
    static bool saveText(const std::string& path, const std::string& text);
};

class CliRunner {
public:
    static int run(const CliOptions& opts);
};

}
