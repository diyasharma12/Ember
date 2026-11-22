#pragma once

#include <cstdint>

namespace core {

extern uint32_t LRU_CLOCK_MAX;

uint32_t getCurrentClock();
uint32_t getIdleTime(uint32_t lastAccessAt);

void evict();

}  // namespace core
