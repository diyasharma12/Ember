#include "core/comm.h"

#include <unistd.h>

namespace core {

long FDComm::Read(char* buf, long count) { return ::read(fd_, buf, count); }

long FDComm::Write(const char* buf, long count) { return ::write(fd_, buf, count); }

}  // namespace core
