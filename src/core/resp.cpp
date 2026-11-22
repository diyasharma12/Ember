#include "core/resp.h"

#include <sstream>

namespace core {

const std::string RESP_NIL = "$-1\r\n";
const std::string RESP_OK = "+OK\r\n";
const std::string RESP_ONE = ":1\r\n";
const std::string RESP_ZERO = ":0\r\n";
const std::string RESP_MINUS_1 = ":-1\r\n";
const std::string RESP_MINUS_2 = ":-2\r\n";

// Reads a decimal length prefix starting at offset, up to (and including) the
// terminating "\r\n". Returns {length, bytesConsumed}.
static std::pair<int, size_t> readLength(const std::string& data, size_t offset) {
    size_t pos = offset;
    int length = 0;

    while (pos < data.size() && data[pos] >= '0' && data[pos] <= '9') {
        length = length * 10 + (data[pos] - '0');
        pos++;
    }

    if (pos >= data.size()) {
        return {0, 0};
    }

    return {length, (pos - offset) + 2};
}

static std::pair<RespValue, size_t> readSimpleString(const std::string& data, size_t offset) {
    size_t pos = offset + 1;
    while (data[pos] != '\r') pos++;

    RespValue v;
    v.type = RespValue::Type::String;
    v.str = data.substr(offset + 1, pos - (offset + 1));
    return {v, (pos - offset) + 2};
}

static std::pair<RespValue, size_t> readError(const std::string& data, size_t offset) {
    auto [v, consumed] = readSimpleString(data, offset);
    v.type = RespValue::Type::Error;
    return {v, consumed};
}

static std::pair<RespValue, size_t> readInt64(const std::string& data, size_t offset) {
    size_t pos = offset + 1;
    int64_t value = 0;
    while (data[pos] != '\r') {
        value = value * 10 + (data[pos] - '0');
        pos++;
    }

    RespValue v;
    v.type = RespValue::Type::Int;
    v.num = value;
    return {v, (pos - offset) + 2};
}

static std::pair<RespValue, size_t> readBulkString(const std::string& data, size_t offset) {
    size_t pos = offset + 1;

    auto [len, delta] = readLength(data, pos);
    pos += delta;

    RespValue v;
    v.type = RespValue::Type::String;
    v.str = data.substr(pos, len);

    return {v, (pos - offset) + len + 2};
}

static std::pair<RespValue, size_t> readArray(const std::string& data, size_t offset) {
    size_t pos = offset + 1;

    auto [count, delta] = readLength(data, pos);
    pos += delta;

    RespValue v;
    v.type = RespValue::Type::Array;
    v.arr.reserve(count);

    for (int i = 0; i < count; i++) {
        auto [element, elementDelta] = decodeOne(data, pos);
        v.arr.push_back(std::move(element));
        pos += elementDelta;
    }

    return {v, pos - offset};
}

std::pair<RespValue, size_t> decodeOne(const std::string& data, size_t offset) {
    if (offset >= data.size()) {
        throw std::runtime_error("No data");
    }

    switch (data[offset]) {
        case '+':
            return readSimpleString(data, offset);
        case '-':
            return readError(data, offset);
        case ':':
            return readInt64(data, offset);
        case '$':
            return readBulkString(data, offset);
        case '*':
            return readArray(data, offset);
    }

    return {RespValue{}, 0};
}

std::vector<RespValue> decode(const std::string& data) {
    if (data.empty()) {
        throw std::runtime_error("No data");
    }

    std::vector<RespValue> values;
    size_t index = 0;
    while (index < data.size()) {
        auto [value, delta] = decodeOne(data, index);
        index += delta;
        values.push_back(std::move(value));
    }

    return values;
}

std::string encodeSimpleString(const std::string& v) {
    return "+" + v + "\r\n";
}

std::string encodeBulkString(const std::string& v) {
    std::ostringstream oss;
    oss << "$" << v.size() << "\r\n" << v << "\r\n";
    return oss.str();
}

std::string encodeInt(int64_t v) {
    return ":" + std::to_string(v) + "\r\n";
}

std::string encodeError(const std::string& msg) {
    return "-" + msg + "\r\n";
}

std::string encodeArray(const std::vector<std::string>& values) {
    std::ostringstream oss;
    oss << "*" << values.size() << "\r\n";
    for (const auto& val : values) {
        oss << encodeBulkString(val);
    }
    return oss.str();
}

}  // namespace core
