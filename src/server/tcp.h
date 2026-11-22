#pragma once

namespace server {

// Runs the bedis server's accept/event loop. Returns a non-zero value on
// setup failure (socket/bind/listen); otherwise runs forever.
int RunServer();

}  // namespace server
