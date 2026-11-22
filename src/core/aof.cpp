#include "core/aof.h"

#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <unordered_map>

#include "config/config.h"
#include "core/resp.h"
#include "core/store.h"

namespace core {

static void dumpKey(std::ofstream& fp, const std::string& key, const ObjPtr& obj) {
    std::string cmd = "SET " + key + " " + obj->Value;

    std::vector<std::string> tokens;
    std::istringstream iss(cmd);
    std::string tok;
    while (iss >> tok) tokens.push_back(tok);

    std::string encoded = encodeArray(tokens);
    fp.write(encoded.data(), static_cast<std::streamsize>(encoded.size()));
}

void DumpAllAOF() {
    std::ofstream fp(config::AOFFile, std::ios::out | std::ios::app | std::ios::binary);
    if (!fp.is_open()) {
        std::cerr << "error: could not open AOF file" << std::endl;
        return;
    }

    std::unordered_map<std::string, ObjPtr> copyStore;
    {
        std::lock_guard<std::mutex> lock(storeMutex);
        copyStore = store;
    }

    std::cout << "writing AOF file at " << config::AOFFile << std::endl;
    for (const auto& [k, obj] : copyStore) {
        dumpKey(fp, k, obj);
    }
    std::cout << "AOF write complete" << std::endl;
}

}  // namespace core
