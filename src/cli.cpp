#include "cli.h"
#include "reader.h"
#include "picker.h"
#include "logger.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <chrono>
#include <cstring>

namespace picker {

namespace {
constexpr const char* kVersionString = "随机抽签 v1.0.0";
constexpr const char* kAsciiLogo =
R"(  ____                 _               ____  _      _              )" "\n"
R"( |  _ \ __ _ _ __   __| | ___  _ __  |  _ \(_)___| |_ __ _ _ __  )" "\n"
R"( | |_) / _` | '_ \ / _` |/ _ \| '__| | |_) | / __| __/ _` | '_ \ )" "\n"
R"( |  _ < (_| | | | | (_| | (_) | |    |  _ <| \__ \ || (_| | | | |)" "\n"
R"( |_| \_\__,_|_| |_|\__,_|\___/|_|    |_| \_\_|___/\__\__,_|_| |_|)" "\n";
}

void ArgParser::printVersion() {
    std::cout << kVersionString << "\n";
    std::cout << "构建: C++17 / MinGW-w64\n";
    std::cout << "随机数: std::mt19937_64 + std::random_device\n";
}

void ArgParser::printHelp(const std::string& exeName) {
    std::cout << "用法: " << exeName << " [选项]\n\n"
              << "选项:\n"
              << "  --file <路径>        指定表格文件 (支持 .csv / .xlsx / .tsv / .txt)\n"
              << "  --count <数字>       抽取人数 (默认 1)\n"
              << "  --output <路径>      将结果保存到文件\n"
              << "  --seed <数字>        指定随机数种子 (默认每次运行都不同)\n"
               << "  --with-ai            启用 AI 增强模式\n"
               << "  --api-key <密钥>     AI API 密钥 (默认使用 DeepSeek)\n"
               << "  --api-endpoint <URL> 完整 API 地址 (默认 https://api.deepseek.com/v1/chat/completions)\n"
               << "  --api-baseurl <URL>  API 基础地址 (如 https://api.deepseek.com，自动补全路径)\n"
               << "  --api-model <名称>   AI 模型名 (默认 deepseek-chat)\n"
               << "  --ai-prompt <文本>   附加 AI 提示\n"
              << "  --no-gui             命令行模式 (无 GUI)\n"
              << "  --verbose            详细日志\n"
              << "  --no-logo            不显示 ASCII 标志\n"
              << "  --version, -v        显示版本信息\n"
              << "  --help, -h           显示本帮助\n"
              << "\n示例:\n"
              << "  " << exeName << " --file students.xlsx --count 3\n"
               << "  " << exeName << " --file names.csv --count 5 --with-ai --api-key sk-xxx\n"
               << "  " << exeName << " --file names.csv --count 3 --with-ai --api-key sk-xxx --api-baseurl https://api.deepseek.com\n";
}

CliOptions ArgParser::parse(int argc, char** argv) {
    CliOptions opts;
    std::string exeName = (argc > 0 && argv && argv[0]) ? argv[0] : "random_picker";

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto needValue = [&](const std::string& name) -> std::string {
            if (i + 1 >= argc) throw std::runtime_error("参数 " + name + " 需要值");
            return argv[++i];
        };

        if (a == "--file") opts.filePath = needValue(a);
        else if (a == "--count") {
            std::string v = needValue(a);
            opts.count = static_cast<std::size_t>(std::stoul(v));
        } else if (a == "--output") opts.outputFile = needValue(a);
        else if (a == "--seed") {
            std::string v = needValue(a);
            opts.seed = std::stoull(v);
            opts.seedGiven = true;
        } else if (a == "--with-ai") opts.aiEnabled = true;
        else if (a == "--api-key") opts.ai.apiKey = needValue(a);
        else if (a == "--api-endpoint") opts.ai.endpoint = needValue(a);
        else if (a == "--api-baseurl") {
            std::string base = needValue(a);
            if (!base.empty()) {
                if (base.back() == '/') base.pop_back();
                opts.ai.endpoint = base + "/v1/chat/completions";
            }
        }
        else if (a == "--api-model") opts.ai.model = needValue(a);
        else if (a == "--ai-prompt") opts.ai.extraPrompt = needValue(a);
        else if (a == "--no-gui") opts.noGui = true;
        else if (a == "--verbose") opts.verbose = true;
        else if (a == "--no-logo") opts.noLogo = true;
        else if (a == "--version" || a == "-v") opts.showVersion = true;
        else if (a == "--help" || a == "-h") opts.showHelp = true;
        else throw std::runtime_error("未知参数: " + a);
    }
    opts.ai.enabled = opts.aiEnabled;
    return opts;
}

std::string Reporter::toText(const RunReport& rep, const TableData& table) {
    std::ostringstream oss;
    oss << "========== 随机抽签结果 ==========\n";
    oss << "源文件: " << rep.sourceFile << "\n";
    oss << "抽取时间: " << timestampString(rep.runTime) << "\n";
    oss << "总人数: " << rep.totalCount << "\n";
    oss << "抽取数量: " << rep.picks.size() << " (请求: " << rep.requestedCount << ")\n";
    if (rep.aiEnabled) {
        oss << "AI 增强: 已启用\n";
    }
    oss << "----------------------------------\n";

    for (std::size_t i = 0; i < rep.picks.size(); ++i) {
        const auto& p = rep.picks[i].person;
        oss << "[" << (i + 1) << "] " << table.nameOf(p) << "\n";
        for (std::size_t c = 0; c < table.headers.size(); ++c) {
            if (c == table.nameColumnIndex) continue;
            auto it = p.fields.find(table.headers[c]);
            if (it == p.fields.end() || it->second.empty()) continue;
            oss << "    " << table.headers[c] << ": " << it->second << "\n";
        }
        if (rep.aiEnabled && !rep.picks[i].aiReason.empty()) {
            oss << "    AI 推荐: " << rep.picks[i].aiReason << "\n";
        }
    }

    if (rep.aiEnabled && !rep.aiSummary.empty()) {
        oss << "----------------------------------\n";
        oss << "AI 综合点评: " << rep.aiSummary << "\n";
    }
    oss << "==================================\n";
    return oss.str();
}

bool Reporter::saveText(const std::string& path, const std::string& text) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f << text;
    return f.good();
}

bool Reporter::saveCsv(const std::string& path, const RunReport& rep, const TableData& table) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f << "序号,";
    for (std::size_t i = 0; i < table.headers.size(); ++i) {
        if (i) f << ",";
        f << escapeCSV(table.headers[i]);
    }
    if (rep.aiEnabled) f << ",AI推荐";
    f << "\n";

    for (std::size_t i = 0; i < rep.picks.size(); ++i) {
        f << (i + 1) << ",";
        for (std::size_t c = 0; c < table.headers.size(); ++c) {
            if (c) f << ",";
            auto it = rep.picks[i].person.fields.find(table.headers[c]);
            f << escapeCSV(it == rep.picks[i].person.fields.end() ? std::string() : it->second);
        }
        if (rep.aiEnabled) {
            f << "," << escapeCSV(rep.picks[i].aiReason);
        }
        f << "\n";
    }
    return f.good();
}

int CliRunner::run(const CliOptions& opts) {
    if (opts.showHelp) {
        ArgParser::printHelp("random_picker");
        return 0;
    }
    if (opts.showVersion) {
        ArgParser::printVersion();
        return 0;
    }

    if (opts.verbose) Log::setLevel(LogLevel::Debug);

    if (!opts.noLogo) {
        std::cout << kAsciiLogo << "\n";
        std::cout << kVersionString << "\n\n";
    }

    if (opts.filePath.empty()) {
        std::cerr << "错误: 缺少 --file 参数。\n\n";
        ArgParser::printHelp("random_picker");
        return 2;
    }
    if (opts.count == 0) {
        std::cerr << "错误: --count 必须大于 0。\n";
        return 2;
    }

    try {
        Log::info("读取文件: {}", opts.filePath);
        auto reader = ReaderFactory::create(opts.filePath);
        TableData table = reader->read(opts.filePath);

        if (table.empty()) {
            std::cerr << "错误: 表格中没有有效数据行。\n";
            return 3;
        }
        Log::info("读取到 {} 条记录", table.size());

        RandomPicker picker;
        if (opts.seedGiven) picker.reseed(opts.seed);
        Log::info("随机种子: {}", picker.currentSeed());

        std::vector<Person> picked = picker.sample(table.rows, opts.count);

        RunReport rep;
        rep.sourceFile = opts.filePath;
        rep.totalCount = table.size();
        rep.requestedCount = opts.count;
        rep.runTime = std::chrono::system_clock::now();
        rep.aiEnabled = opts.aiEnabled;
        auto now = rep.runTime;
        for (auto& p : picked) {
            PickResult r;
            r.person = std::move(p);
            r.timestamp = now;
            rep.picks.push_back(std::move(r));
        }

        if (opts.aiEnabled) {
            AiClient ai(opts.ai);
            std::string err;
            if (ai.analyze(rep, rep.aiSummary, err)) {
                Log::info("AI 分析完成");
            } else {
                std::cerr << "AI 分析失败: " << err << "\n";
                rep.aiEnabled = false;
            }
        }

        std::string text = Reporter::toText(rep, table);
        std::cout << "\n" << text << std::flush;

        if (!opts.outputFile.empty()) {
            bool ok = false;
            std::string lower = toLower(opts.outputFile);
            if (lower.size() >= 4 && lower.substr(lower.size() - 4) == ".csv") {
                ok = Reporter::saveCsv(opts.outputFile, rep, table);
            } else {
                ok = Reporter::saveText(opts.outputFile, text);
            }
            if (ok) {
                Log::info("结果已保存到: {}", opts.outputFile);
            } else {
                std::cerr << "警告: 无法写入输出文件: " << opts.outputFile << "\n";
            }
        }

        Log::info("抽签完成");
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "运行失败: " << ex.what() << "\n";
        Log::error("运行失败: {}", ex.what());
        return 1;
    }
}

}
