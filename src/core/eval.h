#pragma once

#include "core/cmd.h"
#include "core/comm.h"

namespace core {

void EvalAndRespond(const BedisCmds& cmds, IComm& c);

}  // namespace core
