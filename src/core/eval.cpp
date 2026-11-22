#include "core/eval.h"

#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <sstream>
#include <thread>

#include "core/aof.h"
#include "core/obj.h"
#include "core/resp.h"
#include "core/stats.h"
#include "core/store.h"
#include "core/validation.h"

namespace core {

static bool parseInt64(const std::string& s, int64_t& out) {
    if (s.empty()) return false;

    char* end = nullptr;
    errno = 0;
    long long v = std::strtoll(s.c_str(), &end, 10);
    if (errno != 0 || end != s.c_str() + s.size()) return false;

    out = v;
    return true;
}

static std::string evalPING(const std::vector<std::string>& args) {
    if (args.size() >= 2) {
        return encodeError("(error) ERR wrong number of arguments for 'ping' command");
    }

    if (args.empty()) {
        return encodeSimpleString("PONG");
    }

    return encodeBulkString(args[0]);
}

static std::string evalSET(const std::vector<std::string>& args) {
    if (args.size() <= 1) {
        return encodeError("(error) ERR wrong number of arguments for 'set' command");
    }

    const std::string& key = args[0];
    const std::string& value = args[1];
    auto [oType, oEnc] = getTypeEncoding(value);
    int64_t exDurationMs = -1;

    for (size_t i = 2; i < args.size(); i++) {
        if (args[i] == "EX" || args[i] == "ex") {
            i++;
            if (i == args.size()) {
                return encodeError("(error) ERR syntax error");
            }

            int64_t exDurationSec;
            if (!parseInt64(args[i], exDurationSec)) {
                return encodeError("(error) ERR value is not an integer or out of range");
            }

            exDurationMs = exDurationSec * 1000;
        } else {
            return encodeError("(error) ERR syntax error");
        }
    }

    Put(key, NewObj(value, exDurationMs, oType, oEnc));
    return RESP_OK;
}

static std::string evalGET(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        return encodeError("(error) ERR wrong number of arguments for 'get' command");
    }

    ObjPtr obj = Get(args[0]);
    if (obj == nullptr) {
        return RESP_NIL;
    }

    if (hasExpired(obj)) {
        return RESP_NIL;
    }

    return encodeBulkString(obj->Value);
}

static std::string evalTTL(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        return encodeError("(error) ERR wrong number of arguments for 'ttl' command");
    }

    ObjPtr obj = Get(args[0]);
    if (obj == nullptr) {
        return RESP_MINUS_2;
    }

    auto [exp, isExpirySet] = getExpiry(obj);
    if (!isExpirySet) {
        return RESP_MINUS_1;
    }

    uint64_t nowMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());

    if (exp < nowMs) {
        return RESP_MINUS_2;
    }

    uint64_t duration = exp - nowMs;
    return encodeInt(static_cast<int64_t>(duration / 1000));
}

static std::string evalDEL(const std::vector<std::string>& args) {
    if (args.size() < 1) {
        return encodeError("(error) ERR wrong number of arguments for 'del' command");
    }

    int countDel = 0;
    for (const auto& key : args) {
        if (Del(key)) countDel++;
    }

    return encodeInt(countDel);
}

static std::string evalEXPIRE(const std::vector<std::string>& args) {
    if (args.size() <= 1) {
        return encodeError("(error) ERR wrong number of arguments for 'expire' command");
    }

    const std::string& key = args[0];
    int64_t exDurationSec;
    if (!parseInt64(args[1], exDurationSec)) {
        return encodeError("(error) ERR value is not an integer or out of range");
    }

    ObjPtr obj = Get(key);
    if (obj == nullptr) {
        return RESP_ZERO;
    }

    SetExpiry(obj, exDurationSec * 1000);

    return RESP_ONE;
}

static std::string evalBGWRITEAOF() {
    std::thread(DumpAllAOF).detach();
    return RESP_OK;
}

static std::string evalINCR(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        return encodeError("(error) ERR wrong number of arguments for 'INCR' command");
    }

    const std::string& key = args[0];
    ObjPtr obj = Get(key);
    if (obj == nullptr) {
        obj = NewObj("0", -1, OBJ_TYPE_STRING, OBJ_ENCODING_INT);
        Put(key, obj);
    }

    if (auto err = assertType(obj->TypeEncoding, OBJ_TYPE_STRING)) {
        return encodeError(*err);
    }

    if (auto err = assertEncoding(obj->TypeEncoding, OBJ_ENCODING_INT)) {
        return encodeError(*err);
    }

    int64_t i = std::strtoll(obj->Value.c_str(), nullptr, 10);
    i++;
    obj->Value = std::to_string(i);

    return encodeInt(i);
}

static std::string evalINFO() {
    std::ostringstream buff;

    buff << "# Keyspace\r\n";
    for (const auto& kv : KeyStats) {
        (void)kv;
        buff << "db0:keys=" << KeyStats[KEY_METRIC] << ",expires=0,avg_ttl=0\r\n";
    }

    return encodeBulkString(buff.str());
}

void EvalAndRespond(const BedisCmds& cmds, IComm& c) {
    std::ostringstream buf;

    for (const auto& cmd : cmds) {
        if (cmd.Cmd == "PING") {
            buf << evalPING(cmd.Args);
        } else if (cmd.Cmd == "SET") {
            buf << evalSET(cmd.Args);
        } else if (cmd.Cmd == "GET") {
            buf << evalGET(cmd.Args);
        } else if (cmd.Cmd == "TTL") {
            buf << evalTTL(cmd.Args);
        } else if (cmd.Cmd == "DEL") {
            buf << evalDEL(cmd.Args);
        } else if (cmd.Cmd == "EXPIRE") {
            buf << evalEXPIRE(cmd.Args);
        } else if (cmd.Cmd == "BGWRITEAOF") {
            buf << evalBGWRITEAOF();
        } else if (cmd.Cmd == "INCR") {
            buf << evalINCR(cmd.Args);
        } else if (cmd.Cmd == "INFO") {
            buf << evalINFO();
        } else {
            buf << encodeError("ERR unknown command '" + cmd.Cmd + "'");
        }
    }

    std::string out = buf.str();
    c.Write(out.data(), static_cast<long>(out.size()));
}

}  // namespace core
