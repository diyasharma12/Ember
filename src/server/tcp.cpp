#include "server/tcp.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cctype>
#include <chrono>
#include <iostream>
#include <vector>

#include "config/config.h"
#include "core/comm.h"
#include "core/eval.h"
#include "core/expire.h"
#include "core/resp.h"

namespace server {

namespace {

bool toArrayString(const std::vector<core::RespValue>& arr, std::vector<std::string>& out) {
    out.clear();
    out.reserve(arr.size());
    for (const auto& v : arr) {
        if (v.type != core::RespValue::Type::String) return false;
        out.push_back(v.str);
    }
    return true;
}

// Reads and decodes whatever RESP-encoded commands are currently available on
// conn. Returns false if the connection should be closed (EOF, read error, or
// malformed input).
bool readCommands(core::IComm& conn, core::BedisCmds& cmds) {
    char buf[512];
    long n = conn.Read(buf, sizeof(buf));
    if (n <= 0) return false;

    std::string data(buf, static_cast<size_t>(n));

    std::vector<core::RespValue> values;
    try {
        values = core::decode(data);
    } catch (const std::exception&) {
        return false;
    }

    cmds.clear();
    for (const auto& value : values) {
        if (value.type != core::RespValue::Type::Array) return false;

        std::vector<std::string> tokens;
        if (!toArrayString(value.arr, tokens) || tokens.empty()) return false;

        core::BedisCmd cmd;
        cmd.Cmd = tokens[0];
        for (auto& c : cmd.Cmd) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        cmd.Args.assign(tokens.begin() + 1, tokens.end());

        cmds.push_back(std::move(cmd));
    }

    return true;
}

void respond(const core::BedisCmds& cmds, core::IComm& conn) { core::EvalAndRespond(cmds, conn); }

}  // namespace

static int no_of_clients = 0;
static const std::chrono::seconds cronFrequency(1);

int RunServer() {
    std::cout << "Connecting to bedis server on " << config::Host << ":" << config::Port
              << std::endl;

    const int max_clients = 20000;

    // This creates a socket for us to use.
    int serverFD = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFD < 0) return -1;

    int flags = fcntl(serverFD, F_GETFL, 0);
    fcntl(serverFD, F_SETFL, flags | O_NONBLOCK);

    int reuse = 1;
    setsockopt(serverFD, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // Binds the ip to the socket we created.
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(config::Port));
    if (inet_pton(AF_INET, config::Host.c_str(), &addr.sin_addr) != 1) {
        close(serverFD);
        return -1;
    }

    if (bind(serverFD, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(serverFD);
        return -1;
    }

    // Start listening on the socket we just created.
    if (listen(serverFD, max_clients) < 0) {
        close(serverFD);
        return -1;
    }

    // pollfd list we monitor for readability. Index 0 is always the listening
    // socket; this plays the role epoll played in the original, but poll() is
    // portable across macOS/Linux instead of being Linux-only.
    std::vector<pollfd> fds;
    fds.push_back(pollfd{serverFD, POLLIN, 0});

    auto lastCronExecTime = std::chrono::steady_clock::now();

    for (;;) {
        // This is how we auto-clean expired keys in a single-threaded event
        // loop: every time control flow reaches this point we do a cleanup
        // job if the cron interval has passed.
        if (std::chrono::steady_clock::now() >= lastCronExecTime + cronFrequency) {
            core::DeleteExpiredKeys();
            lastCronExecTime = std::chrono::steady_clock::now();
        }

        // Block for up to 1s so the cron check above still runs periodically
        // even when idle.
        int nevents = poll(fds.data(), fds.size(), 1000);
        if (nevents < 0) continue;

        std::vector<size_t> toClose;

        for (size_t i = 0; i < fds.size(); i++) {
            if (fds[i].revents == 0) continue;

            if (fds[i].fd == serverFD) {
                // Drain every pending connection: poll (like epoll in
                // level-triggered mode) only reports one event even if
                // several connections are queued.
                for (;;) {
                    int fd = accept(serverFD, nullptr, nullptr);
                    if (fd < 0) break;

                    no_of_clients++;
                    int cflags = fcntl(fd, F_GETFL, 0);
                    fcntl(fd, F_SETFL, cflags | O_NONBLOCK);

                    fds.push_back(pollfd{fd, POLLIN, 0});
                }
            } else {
                core::FDComm comm(fds[i].fd);
                core::BedisCmds cmds;

                if (!readCommands(comm, cmds)) {
                    close(fds[i].fd);
                    no_of_clients--;
                    toClose.push_back(i);
                    continue;
                }

                respond(cmds, comm);
            }
        }

        for (auto it = toClose.rbegin(); it != toClose.rend(); ++it) {
            fds.erase(fds.begin() + static_cast<long>(*it));
        }
    }
}

}  // namespace server
