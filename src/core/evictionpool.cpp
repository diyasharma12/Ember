#include "core/evictionpool.h"

#include "core/eviction.h"

namespace core {

int ePoolSizeMax = 16;
EvictionPool ePool;

void EvictionPool::Push(const std::string& key, uint32_t lastAccessedAt) {
    if (keyset.count(key)) {
        return;
    }

    if (static_cast<int>(pool.size()) == ePoolSizeMax &&
        lastAccessedAt >= pool.back()->lastAccessedAt) {
        return;
    }

    // Find the insertion point that keeps the pool sorted in descending order
    // of idle time (the most idle key stays at index 0, ready for Pop()).
    size_t idx = 0;
    while (idx < pool.size() &&
           getIdleTime(pool[idx]->lastAccessedAt) > getIdleTime(lastAccessedAt)) {
        idx++;
    }

    PoolItem* item = new PoolItem{key, lastAccessedAt};
    keyset[key] = item;

    if (static_cast<int>(pool.size()) < ePoolSizeMax) {
        pool.push_back(nullptr);
    } else {
        // The pool is full: drop the least idle (last) entry to make room.
        PoolItem* lastElement = pool.back();
        keyset.erase(lastElement->key);
        delete lastElement;
    }

    // Shift elements right by one, the same way Redis uses memmove, to open
    // up the slot at idx.
    for (size_t i = pool.size() - 1; i > idx; i--) {
        pool[i] = pool[i - 1];
    }
    pool[idx] = item;
}

PoolItem* EvictionPool::Pop() {
    if (pool.empty()) {
        return nullptr;
    }

    PoolItem* item = pool.front();
    pool.erase(pool.begin());
    keyset.erase(item->key);
    return item;
}

}  // namespace core
