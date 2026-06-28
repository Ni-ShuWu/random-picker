#pragma once

#include "common.h"
#include "ai.h"
#include <string>

namespace picker {

struct GuiOptions {
    std::string initialFile;
    bool verbose = false;
    AiConfig ai;
};

int runGui(const GuiOptions& opts);

}
