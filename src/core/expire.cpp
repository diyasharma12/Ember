#include "core/expire.h"

#include <iostream>
#include <vector>

#include "core/store.h"

namespace core {

static float expireOnce() {
    int totalChecks = 20;
    int totalDeleted = 0;

    // Collect expired keys first: erasing from an unordered_map while
    // iterating it is undefined behaviour in C++, unlike ranging over a Go
    // map.
    std::vector<std::string> expiredKeys;
    for (const auto& [key, obj] : store) {
        totalChecks--;

        if (hasExpired(obj)) {
            expiredKeys.push_back(key);
        }

        if (totalChecks == 0) break;
    }

    for (const auto& key : expiredKeys) {
        Del(key);
        totalDeleted++;
    }

    return static_cast<float>(totalDeleted) / 20.0f;
}

void DeleteExpiredKeys() {
    for (;;) {
        float frac = expireOnce();
        if (frac < 0.25f) break;
    }

    std::cout << "deleted the undeleted expired keys. Total keys left: " << store.size()
              << std::endl;
}

}  // namespace core
