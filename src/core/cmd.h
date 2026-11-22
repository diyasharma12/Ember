#pragma once

#include <string>
#include <vector>

namespace core {

struct BedisCmd {
    std::string Cmd;
    std::vector<std::string> Args;
};

using BedisCmds = std::vector<BedisCmd>;

}  // namespace core
