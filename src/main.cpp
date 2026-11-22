#include <iostream>
#include <string>

#include "config/config.h"
#include "server/tcp.h"

namespace {

// Minimal stand-in for Go's flag package: supports `-flag value`,
// `--flag value` and `--flag=value` for --host/-host and --port/-port.
void setupFlags(int argc, char** argv) {
    auto isFlag = [](const std::string& arg, const std::string& name) {
        return arg == "-" + name || arg == "--" + name;
    };

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        std::string flagName;
        std::string inlineValue;
        bool hasInlineValue = false;

        auto eq = arg.find('=');
        if (eq != std::string::npos && (arg.rfind("--", 0) == 0 || arg.rfind('-', 0) == 0)) {
            flagName = arg.substr(0, eq);
            inlineValue = arg.substr(eq + 1);
            hasInlineValue = true;
        } else {
            flagName = arg;
        }

        if (isFlag(flagName, "host")) {
            if (hasInlineValue) {
                config::Host = inlineValue;
            } else if (i + 1 < argc) {
                config::Host = argv[++i];
            }
        } else if (isFlag(flagName, "port")) {
            std::string value = hasInlineValue ? inlineValue : (i + 1 < argc ? argv[++i] : "");
            if (!value.empty()) {
                try {
                    config::Port = std::stoi(value);
                } catch (const std::exception&) {
                    std::cerr << "invalid port: " << value << std::endl;
                }
            }
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    setupFlags(argc, argv);

    std::cout << "starting the server :-)" << std::endl;
    return server::RunServer();
}
