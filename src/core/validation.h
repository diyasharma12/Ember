#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace core {

uint8_t getType(uint8_t te);
uint8_t getEncoding(uint8_t te);

// Returns an error message if the assertion fails, std::nullopt otherwise.
std::optional<std::string> assertType(uint8_t te, uint8_t t);
std::optional<std::string> assertEncoding(uint8_t te, uint8_t e);

}  // namespace core
