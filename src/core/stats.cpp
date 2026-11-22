#include "core/stats.h"

namespace core {

std::unordered_map<std::string, int> KeyStats;

const std::string KEY_METRIC = "Keys";

void UpdateDBStat(const std::string& metric, int value) { KeyStats[metric] = value; }

}  // namespace core
