#include "core/validation.h"

namespace core {

uint8_t getType(uint8_t te) { return te & 0b11110000; }

uint8_t getEncoding(uint8_t te) { return te & 0b00001111; }

std::optional<std::string> assertType(uint8_t te, uint8_t t) {
    if (getType(te) != t) {
        return "the operation is not permitted on this type";
    }
    return std::nullopt;
}

std::optional<std::string> assertEncoding(uint8_t te, uint8_t e) {
    if (getEncoding(te) != e) {
        return "the operation is not permitted on this encoding";
    }
    return std::nullopt;
}

}  // namespace core
