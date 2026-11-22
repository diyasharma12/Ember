#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace core {

extern const std::string RESP_NIL;
extern const std::string RESP_OK;
extern const std::string RESP_ONE;
extern const std::string RESP_ZERO;
extern const std::string RESP_MINUS_1;
extern const std::string RESP_MINUS_2;

// A decoded RESP value. Mirrors the dynamic `interface{}` used on the Go side:
// a value is either a simple/bulk string, an integer, an error, or an array of
// further RespValues.
struct RespValue {
    enum class Type { String, Int, Error, Array };

    Type type;
    std::string str;              // valid when type == String || type == Error
    int64_t num = 0;               // valid when type == Int
    std::vector<RespValue> arr;    // valid when type == Array
};

// Decodes one RESP value starting at data[offset]. Returns the value and the
// number of bytes consumed.
std::pair<RespValue, size_t> decodeOne(const std::string& data, size_t offset);

// Decodes every complete RESP value found in data, in order.
std::vector<RespValue> decode(const std::string& data);

// Encoders. These replace Go's single reflective `Encode(value interface{},
// isSimple bool)` with explicit, type-safe entry points.
std::string encodeSimpleString(const std::string& v);
std::string encodeBulkString(const std::string& v);
std::string encodeInt(int64_t v);
std::string encodeError(const std::string& msg);
std::string encodeArray(const std::vector<std::string>& values);

}  // namespace core
