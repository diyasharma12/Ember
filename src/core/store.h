#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include "core/obj.h"

namespace core {

using ObjPtr = std::shared_ptr<Obj>;

extern std::unordered_map<std::string, ObjPtr> store;
extern std::unordered_map<Obj*, uint64_t> expires;
extern std::mutex storeMutex;

ObjPtr NewObj(const std::string& value, int64_t durationMs, uint8_t oType, uint8_t oEnc);

void SetExpiry(const ObjPtr& obj, int64_t durationMs);
bool hasExpired(const ObjPtr& obj);
std::pair<uint64_t, bool> getExpiry(const ObjPtr& obj);

void Put(const std::string& k, const ObjPtr& obj);
ObjPtr Get(const std::string& k);
bool Del(const std::string& k);

}  // namespace core
