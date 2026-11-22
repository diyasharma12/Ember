#include "core/store.h"

#include <chrono>

#include "config/config.h"
#include "core/eviction.h"
#include "core/stats.h"

namespace core {

std::unordered_map<std::string, ObjPtr> store;
std::unordered_map<Obj*, uint64_t> expires;
std::mutex storeMutex;

static uint64_t nowMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
}

ObjPtr NewObj(const std::string& value, int64_t durationMs, uint8_t oType, uint8_t oEnc) {
    auto obj = std::make_shared<Obj>();
    obj->TypeEncoding = oType | oEnc;
    obj->Value = value;

    if (durationMs > 0) {
        SetExpiry(obj, durationMs);
    }

    return obj;
}

void SetExpiry(const ObjPtr& obj, int64_t durationMs) {
    expires[obj.get()] = nowMs() + static_cast<uint64_t>(durationMs);
}

bool hasExpired(const ObjPtr& obj) {
    auto it = expires.find(obj.get());
    if (it == expires.end()) return false;
    return it->second <= nowMs();
}

std::pair<uint64_t, bool> getExpiry(const ObjPtr& obj) {
    auto it = expires.find(obj.get());
    if (it == expires.end()) return {0, false};
    return {it->second, true};
}

void Put(const std::string& k, const ObjPtr& obj) {
    storeMutex.lock();
    if (static_cast<int>(store.size()) > config::KeysLimit) {
        storeMutex.unlock();
        evict();
        storeMutex.lock();
    }
    obj->LastAccessAt = getCurrentClock();
    store[k] = obj;

    UpdateDBStat(KEY_METRIC, static_cast<int>(store.size()));
    storeMutex.unlock();
}

ObjPtr Get(const std::string& k) {
    auto it = store.find(k);
    if (it == store.end()) return nullptr;

    ObjPtr v = it->second;
    if (hasExpired(v)) {
        Del(k);
        return nullptr;
    }
    return v;
}

bool Del(const std::string& k) {
    auto it = store.find(k);
    if (it != store.end()) {
        ObjPtr obj = it->second;

        storeMutex.lock();
        store.erase(it);
        expires.erase(obj.get());
        storeMutex.unlock();

        UpdateDBStat(KEY_METRIC, static_cast<int>(store.size()));

        return true;
    }
    return false;
}

}  // namespace core
