#include "ai.h"
#include "http.h"
#include "logger.h"

#include <sstream>
#include <functional>

namespace picker {

namespace {

std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out.push_back(c);
                }
        }
    }
    return out;
}

std::string buildPayload(const AiConfig& cfg, const RunReport& report) {
    std::ostringstream names;
    for (std::size_t i = 0; i < report.picks.size(); ++i) {
        if (i) names << "、";
        names << report.picks[i].person.fields.count("姓名") ?
                  report.picks[i].person.fields.at("姓名") :
                  (report.picks[i].person.fields.empty() ? "?" : report.picks[i].person.fields.begin()->second);
    }

    std::ostringstream user;
    user << "本次从 " << report.totalCount << " 人中随机抽取了 "
         << report.picks.size() << " 位同学: " << names.str() << "。";
    if (!cfg.extraPrompt.empty()) {
        user << "\n附加要求: " << cfg.extraPrompt;
    }
    user << "\n请用中文给出 50-100 字左右的轻松俏皮推荐理由。";

    std::ostringstream payload;
    payload << "{\"model\":\"" << jsonEscape(cfg.model) << "\","
            << "\"messages\":["
            << "{\"role\":\"system\",\"content\":\"你是一个幽默的中文助手，擅长用轻松口吻点评随机抽签结果。\"},"
            << "{\"role\":\"user\",\"content\":\"" << jsonEscape(user.str()) << "\"}"
            << "],\"max_tokens\":300}";
    return payload.str();
}

std::string extractContent(const std::string& json) {
    auto pos = json.find("\"content\":");
    if (pos == std::string::npos) return {};
    pos = json.find('"', pos + 10);
    if (pos == std::string::npos) return {};
    auto end = json.find('"', pos + 1);
    while (end != std::string::npos && json[end - 1] == '\\') {
        end = json.find('"', end + 1);
    }
    if (end == std::string::npos) return {};
    std::string raw = json.substr(pos + 1, end - pos - 1);
    std::string out;
    out.reserve(raw.size());
    for (std::size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] == '\\' && i + 1 < raw.size()) {
            char nx = raw[i + 1];
            switch (nx) {
                case 'n': out.push_back('\n'); ++i; break;
                case 'r': out.push_back('\r'); ++i; break;
                case 't': out.push_back('\t'); ++i; break;
                case '"': out.push_back('"');  ++i; break;
                case '\\': out.push_back('\\'); ++i; break;
                case 'u': {
                    if (i + 5 < raw.size()) {
                        unsigned code = 0;
                        std::sscanf(raw.c_str() + i + 2, "%4x", &code);
                        if (code < 0x80) out.push_back(static_cast<char>(code));
                        else if (code < 0x800) {
                            out.push_back(static_cast<char>(0xC0 | (code >> 6)));
                            out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                        } else {
                            out.push_back(static_cast<char>(0xE0 | (code >> 12)));
                            out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
                            out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                        }
                        i += 5;
                    }
                    break;
                }
                default: out.push_back(nx); ++i; break;
            }
        } else {
            out.push_back(raw[i]);
        }
    }
    return out;
}

}

AiClient::AiClient(AiConfig cfg) : cfg_(std::move(cfg)) {}

bool AiClient::analyze(const RunReport& report, std::string& outReason, std::string& outError) const {
    if (!cfg_.enabled) { outError = "AI 未启用"; return false; }
    if (cfg_.apiKey.empty()) { outError = "缺少 API Key"; return false; }
    if (report.picks.empty()) { outError = "没有抽中任何人"; return false; }

    HttpClient client;
    std::string payload = buildPayload(cfg_, report);
    std::map<std::string, std::string> headers{
        {"Authorization", "Bearer " + cfg_.apiKey},
        {"Content-Type", "application/json"}
    };

    Log::info("AI 请求: {}", cfg_.endpoint);
    auto resp = client.post(cfg_.endpoint, payload, headers, 60000);
    if (!resp.ok()) {
        outError = "HTTP 调用失败: " + resp.errorMessage;
        Log::error(outError);
        return false;
    }
    if (resp.statusCode / 100 != 2) {
        outError = "AI 服务返回错误 " + std::to_string(resp.statusCode) + ": " + resp.body.substr(0, 200);
        Log::error(outError);
        return false;
    }
    outReason = extractContent(resp.body);
    if (outReason.empty()) {
        outError = "无法解析 AI 返回内容";
        return false;
    }
    return true;
}

}
