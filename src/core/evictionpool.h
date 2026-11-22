#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace core {

struct PoolItem {
    std::string key;
    uint32_t lastAccessedAt;
};

// TODO: When last accessed at of object changes, update the poolItem
// corresponding to that.
class EvictionPool {
   public:
    std::vector<PoolItem*> pool;
    std::unordered_map<std::string, PoolItem*> keyset;

    void Push(const std::string& key, uint32_t lastAccessedAt);
    PoolItem* Pop();
};

extern int ePoolSizeMax;
extern EvictionPool ePool;

}  // namespace core
