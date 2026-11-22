# Ember

A Redis-compatible in-memory key-value store written from scratch in C++17 — a single-threaded,
`poll()`-based TCP server, a hand-written RESP protocol parser, TTL key expiry, approximate LRU
eviction, and AOF persistence with a `fork()`-based background rewrite.

It speaks real RESP, so a real `redis-cli` or any Redis client library can talk to it directly for
the commands it supports.

## Why this exists

I'd used Redis before as a client — `LPUSH`/`RPOP` queues for matchmaking — but only ever from the
outside. This project is about understanding the other side of that socket: how a single-threaded
event loop stays fast, how a wire protocol gets parsed byte by byte, and why Redis makes the
specific trade-offs it does (approximate LRU instead of true LRU, `fork()` instead of a lock,
sampling instead of scanning). It's built on what I learned from a Go tutorial series and the book
*Build Your Own Redis with C/C++*, ported to C++ with a number of real bugs from that reference
implementation found and fixed along the way.

## Architecture

```
        CLIENTS (redis-cli, nc, any RESP client)
                      │ TCP
                      ▼
   NETWORK LAYER — socket/bind/listen, poll() over all client fds,
   non-blocking accept/read/write, per-connection read+write buffers
                      │
                      ▼
   RESP PARSER — decodes *, $, :, +, - and inline commands;
   handles partial reads and pipelining
                      │
                      ▼
   COMMAND DISPATCHER — validates input, dispatches to a handler,
   unknown commands get a real error, never a crash
          │              │                 │
          ▼              ▼                 ▼
       STORE          EXPIRY           EVICTION
   unordered_map    lazy (on GET)    24-bit access clock,
   key -> Obj       + active sweep   sampled eviction pool
   (value + TTL +   on the poll()
    access clock)   timeout
          │
          ▼
   PERSISTENCE — every write appendable to the AOF log; full replay
   on startup; BGWRITEAOF forks a child for a copy-on-write rewrite,
   so the live store never needs a mutex
```

## Build & run

Requires CMake ≥ 3.16 and a C++17 compiler.

```bash
cmake -B build
cmake --build build

./build/bedis --host 0.0.0.0 --port 7379
```

Flags: `--host` (default `0.0.0.0`), `--port` (default `7379`). Both also accept `-host`/`-port`
and `--flag=value` forms.

### Try it

```bash
redis-cli -p 7379 SET foo bar EX 10
redis-cli -p 7379 GET foo
redis-cli -p 7379 TTL foo
```

or, with no client installed:

```bash
printf '*1\r\n$4\r\nPING\r\n' | nc -w1 localhost 7379
```

## Supported commands

| Command | Syntax | Description |
|---|---|---|
| `PING` | `PING [message]` | Health check; echoes `message` if given, else `PONG`. |
| `SET` | `SET key value [EX seconds]` | Set a string value, with an optional TTL. |
| `GET` | `GET key` | Get a value, or a nil reply if missing/expired. |
| `DEL` | `DEL key [key ...]` | Delete one or more keys; returns the count actually deleted. |
| `EXPIRE` | `EXPIRE key seconds` | Set/replace a key's TTL. |
| `TTL` | `TTL key` | Seconds remaining, `-1` if no TTL, `-2` if the key doesn't exist. |
| `INCR` | `INCR key` | Atomically increment an integer-encoded value (creates it at 0 if missing). |
| `INFO` | `INFO` | Basic keyspace stats. |
| `BGWRITEAOF` | `BGWRITEAOF` | Triggers a background AOF rewrite. |

Anything else returns a real `ERR unknown command '<name>'` — it is never silently treated as
another command.

## Design notes

- **Approximate LRU, not true LRU.** A true LRU (hashmap + doubly linked list) is O(1) per access,
  but it costs two extra pointers per key and turns every `GET` into a pointer-relinking write.
  Ember instead tracks a coarse access clock per key, samples a handful of candidates into a small
  fixed-size eviction pool, and evicts from that — the same trade-off real Redis makes, and for the
  same reasons.
- **Expiry by sampling, not scanning.** An active sweep samples 20 keys with a TTL at a time,
  repeating while more than 25% of the sample turned out expired, instead of scanning the whole
  keyspace — bounded, amortized cleanup work instead of an O(n) pause, run off the server's `poll()`
  timeout so it fires even when the server is otherwise idle.
- **`fork()` + copy-on-write for persistence, not a background thread.** `BGWRITEAOF` forks a child
  process that writes a fresh AOF from its own frozen, copy-on-write view of memory. The live store
  never needs a mutex for this — the OS's virtual memory system does the isolation, not application
  code.
- **`poll()`, not `epoll`/`kqueue`.** `epoll` is Linux-only and `kqueue` is BSD/macOS-only; `poll()`
  is portable across both and the connection counts here never approach the scale where that
  trade-off would matter.

## Benchmarks

Measured with the real `redis-benchmark` tool against an `-O2` build, client and server on the same
machine, 100,000 requests, 50 concurrent connections:

| Command | Result |
|---|---|
| `PING` (inline) | ~113,000 req/sec |
| `PING` (RESP array) | ~112,000 req/sec |
| `SET` | ~114,000 req/sec |
| `GET` | ~114,000 req/sec |
| `INCR` | ~114,000 req/sec |

Reproduce it yourself:

```bash
redis-benchmark -h 127.0.0.1 -p 7379 -n 100000 -c 50 -q
```

These numbers are from one machine, one run, with client and server sharing CPU cores and no AOF
fsync active during the test — treat them as a rough order of magnitude, not a guarantee. Full
methodology and caveats are in the field manual.

## Known limitations

- String values only — no lists, hashes, sets, or sorted sets yet.
- `poll()`-based I/O doesn't scale the way `epoll`/`kqueue` would at tens of thousands of
  connections.
- AOF only — no RDB snapshotting.
- No replication, clustering, or pub/sub.
- `std::unordered_map` handles hashing/resizing, not Redis's incremental-rehashing `dict.c`.

## Credits

Learned Redis's internals from a Go tutorial series on YouTube and the book *Build Your Own Redis
with C/C++*, then rebuilt it from scratch in C++.
