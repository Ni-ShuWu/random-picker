#include "cli.h"
#include "gui.h"
#include "logger.h"

#include <iostream>
#include <cstdio>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define _WINSOCKAPI_
#include <windows.h>
#endif

int main(int argc, char** argv) {
#if defined(_WIN32)
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    using namespace picker;

    try {
        CliOptions opts = ArgParser::parse(argc, argv);

        bool wantsCli = opts.noGui ||
                        !opts.filePath.empty() ||
                        opts.showHelp ||
                        opts.showVersion ||
                        argc > 1;

#if defined(_WIN32)
        if (!wantsCli) {
            GuiOptions g;
            g.initialFile = opts.filePath;
            g.verbose = opts.verbose;
            g.ai = opts.ai;
            int rc = runGui(g);
            return rc;
        }
#endif

        return CliRunner::run(opts);
    } catch (const std::exception& ex) {
        std::cerr << "参数解析错误: " << ex.what() << "\n";
        return 2;
    }
}
