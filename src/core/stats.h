#pragma once

#include <string>
#include <unordered_map>

namespace core {

extern std::unordered_map<std::string, int> KeyStats;

extern const std::string KEY_METRIC;

void UpdateDBStat(const std::string& metric, int value);

}  // namespace core
