#pragma once

#include <cstdint>
#include <string>
#include <utility>

namespace core {

struct Obj {
    uint8_t TypeEncoding;
    std::string Value;

    // Keeps track of the time when the object was last accessed at. Redis uses 24
    // bits to store this value. We use a plain uint32_t here instead of a bitfield
    // packed with type/encoding, matching the original tradeoff of simplicity over
    // compactness.
    uint32_t LastAccessAt = 0;
};

extern const uint8_t OBJ_TYPE_STRING;
extern const uint8_t OBJ_ENCODING_RAW;
extern const uint8_t OBJ_ENCODING_INT;
extern const uint8_t OBJ_ENCODING_EMBSTR;

// Returns {type, encoding} for a string value.
std::pair<uint8_t, uint8_t> getTypeEncoding(const std::string& value);

}  // namespace core
