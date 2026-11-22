#include "core/obj.h"

#include <cstdlib>

namespace core {

const uint8_t OBJ_TYPE_STRING = uint8_t(0 << 4);
const uint8_t OBJ_ENCODING_RAW = uint8_t(0);
const uint8_t OBJ_ENCODING_INT = uint8_t(1);
const uint8_t OBJ_ENCODING_EMBSTR = uint8_t(8);

static bool isInt64(const std::string& value) {
    if (value.empty()) return false;

    char* end = nullptr;
    errno = 0;
    long long v = std::strtoll(value.c_str(), &end, 10);
    (void)v;
    return errno == 0 && end == value.c_str() + value.size();
}

std::pair<uint8_t, uint8_t> getTypeEncoding(const std::string& value) {
    uint8_t oType = OBJ_TYPE_STRING;

    if (isInt64(value)) {
        return {oType, OBJ_ENCODING_INT};
    }

    if (value.size() <= 44) {
        return {oType, OBJ_ENCODING_EMBSTR};
    }

    return {oType, OBJ_ENCODING_RAW};
}

}  // namespace core
