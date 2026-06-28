#pragma once

#include "common.h"
#include <string>
#include <vector>

namespace picker {

struct AiConfig {
    bool enabled = false;
    std::string apiKey;
    std::string endpoint = "https://api.deepseek.com/v1/chat/completions";
    std::string model = "deepseek-chat";
    std::string extraPrompt;
};

class AiClient {
public:
    explicit AiClient(AiConfig cfg);

    bool analyze(const RunReport& report, std::string& outReason, std::string& outError) const;

    const AiConfig& config() const { return cfg_; }

private:
    AiConfig cfg_;
};

}
