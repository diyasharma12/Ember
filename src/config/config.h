#pragma once

#include <string>

namespace config {

extern std::string Host;
extern int Port;
extern int KeysLimit;

extern double EvictionRatio;

extern std::string EvictionStratery;
extern std::string AOFFile;

}  // namespace config
