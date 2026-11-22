#include "config/config.h"

namespace config {

std::string Host = "0.0.0.0";
int Port = 7379;
int KeysLimit = 100;

double EvictionRatio = 0.04;

std::string EvictionStratery = "allkeys-lru";
std::string AOFFile = "./bedis-main.aof";

}  // namespace config
