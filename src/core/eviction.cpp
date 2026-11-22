#include "core/eviction.h"

#include <ctime>
#include <vector>

#include "config/config.h"
#include "core/evictionpool.h"
#include "core/store.h"

namespace core {

uint32_t LRU_CLOCK_MAX = 0x00FFFFFF;

uint32_t getCurrentClock() {
    return static_cast<uint32_t>(std::time(nullptr)) & LRU_CLOCK_MAX;
}

// Simplest eviction strategy: evict the first key found while walking the
// store (unordered_map iteration order is effectively arbitrary, like Go's
// map iteration).
static void evictFirst() {
    if (!store.empty()) {
        Del(store.begin()->first);
    }
}

static void evictAllkeysRandom() {
    int64_t numberOfKeys = static_cast<int64_t>(config::EvictionRatio * config::KeysLimit);

    // Collect the keys to delete first: erasing from an unordered_map while
    // iterating it is undefined behaviour in C++, unlike ranging over a Go map.
    std::vector<std::string> toDelete;
    for (const auto& [k, obj] : store) {
        (void)obj;
        toDelete.push_back(k);
        if (--numberOfKeys <= 0) break;
    }

    for (const auto& k : toDelete) {
        Del(k);
    }
}

uint32_t getIdleTime(uint32_t lastAccessAt) {
    uint32_t c = getCurrentClock();
    if (c >= lastAccessAt) {
        return c - lastAccessAt;
    }
    return (LRU_CLOCK_MAX - lastAccessAt) + c;
}

static void populateEvictionPool() {
    int sampleSize = 5;
    for (const auto& [k, obj] : store) {
        ePool.Push(k, obj->LastAccessAt);
        sampleSize--;
        if (sampleSize == 0) break;
    }
}

// TODO: no need to populate every time. should populate only when the number
// of keys to evict is less than what we have in the pool.
static void evictAllkeysLRU() {
    int evictCount = static_cast<int>(config::EvictionRatio * config::KeysLimit);
    if (static_cast<int>(ePool.pool.size()) < evictCount) {
        populateEvictionPool();
    }

    for (int i = 0; i < evictCount && !ePool.pool.empty(); i++) {
        PoolItem* item = ePool.Pop();
        if (item == nullptr) {
            return;
        }
        Del(item->key);
        delete item;
    }
}

void evict() {
    if (config::EvictionStratery == "simple-string") {
        evictFirst();
    } else if (config::EvictionStratery == "allkeys-random") {
        evictAllkeysRandom();
    } else if (config::EvictionStratery == "allkeys-lru") {
        evictAllkeysLRU();
    } else {
        evictFirst();
    }
}

}  // namespace core
