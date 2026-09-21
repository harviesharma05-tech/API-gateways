# Asynchronous HTTP API Gateway — Prototype
### Team ID: DSCPP-III-2026-T035 | Phase-I Working Prototype
### Pure C++ + hand-written Data Structures (no STL containers)

## What changed from a "normal" C++ prototype
Every data structure here is written from scratch — no `std::vector`,
`std::map`, `std::queue`, `std::unordered_map`, or smart pointers.
Only plain C++ language features, `new`/`delete`, and `<thread>`/`<mutex>`
(needed to actually demonstrate the OS synchronization concept) are used.

## Build & Run
```bash
g++ -std=c++17 -O2 -Wall -Wextra -Iinclude -pthread -o gateway src/main.cpp
./gateway
```
Requires a C++17 compiler (g++ 9+ / clang 10+). No external libraries,
no CMake, no third-party framework — just the compiler.

## File structure
```
api_gateway_pure/
├── include/
│   ├── DynamicArray.h    Hand-written resizable array (like building std::vector yourself)
│   ├── HashTable.h        Hash table with separate chaining (own linked list + hash fn)
│   ├── HashRing.h          Consistent hashing on a sorted array + binary search
│   ├── RequestQueue.h      Circular array-based queue (own front/rear/wraparound)
│   ├── TokenBucket.h       Rate limiter (OOP encapsulation)
│   ├── Worker.h            Worker base class + ReadWorker/WriteWorker (inheritance, polymorphism)
│   ├── ThreadPool.h        Fixed thread pool pulling from the circular queue
│   └── HealthChecker.h     Background monitoring thread + failover
├── src/
│   └── main.cpp            Wires everything together with manual new/delete
└── README.md
```

## DSA concepts — all hand-implemented (maps to slide 6's table)
| Concept | Where | How it's built |
|---|---|---|
| Resizable array | `DynamicArray.h` | Manual `new[]`/`delete[]`, doubles capacity when full — same logic you'd write for a C array-based list |
| Linked list | `HashTable.h` (`ChainNode`) | Singly linked list used for collision chaining in each bucket |
| Hash table | `HashTable.h` | Array of 101 buckets, own hash function (djb2-style), separate chaining, O(1) average lookup |
| Hashing + sorted array + binary search | `HashRing.h` | Own FNV-1a + avalanche-mixed hash function; ring kept sorted manually in a `DynamicArray`; `lowerBound()` is a hand-written binary search (same idea as `std::map`, but built from scratch) |
| Circular queue | `RequestQueue.h` | Fixed-size array with `front`/`rear`/`count`, wraps around with modulo — classic circular buffer, not `std::queue` |

## OOP (C++) concepts
| Concept | Where |
|---|---|
| Encapsulation | `TokenBucket` — private state, only reachable through `allowRequest()` |
| Inheritance | `ReadWorker`, `WriteWorker` derive from `Worker` |
| Polymorphism | `handleRequest()` is `virtual`, overridden per worker type, called through a base `Worker*` |
| Manual memory management | Workers are created with `new` in `main.cpp` and explicitly `delete`d at shutdown — no smart pointers |
| Templates | `DynamicArray<T>` and `HashTable<V>` are generic, reused for different types (`Worker*`, `TokenBucket`, `bool`, `std::thread`) |

## Operating Systems concepts
| Concept | Where |
|---|---|
| Producer–consumer synchronization | `RequestQueue` — `mutex` + `condition_variable` between the request dispatcher and the thread pool |
| Thread pool | `ThreadPool.h` — fixed threads pull from the shared circular queue instead of spawning one thread per request |
| Background monitoring thread | `HealthChecker.h` — independent thread checks worker health every 300ms and triggers failover |
| Atomic flags | `std::atomic<bool>` / `std::atomic<int>` for lock-free health/load state |

## What the demo actually proves when you run it
1. Two different clients hash to two different workers (consistent hashing
   is really distributing load, not round-robin)
2. The same client always lands on the same worker across multiple
   requests — the "stickiness" that makes consistent hashing useful
3. A client exceeding its token bucket gets rejected in real time
4. When `Worker-B` is marked unhealthy, the background health-checker
   thread detects it, removes it from the ring exactly once, and every
   request that *used to* route to Worker-B is correctly re-routed to a
   healthy worker — this is the "automatic failover" from slide 5
5. Clean shutdown: every `new` has a matching `delete`, no leaks

## Verified, not just claimed
- Compiled with `-Wall -Wextra`: **zero warnings**
- Also compiled and run under **AddressSanitizer + UndefinedBehaviorSanitizer**
  (`-fsanitize=address,undefined`): **zero errors, zero leaks** — this
  specifically checks the manual `new`/`delete` and raw pointer usage
  throughout the hash table, hash ring, and worker lifecycle

## Honest limitations (say this if asked — don't oversell)
- Still simulates requests in-process; doesn't open a real socket yet
  (that's Phase-II — actual async networking, e.g. with `epoll`)
- No zero-copy `string_view` parsing yet — requests are simple structs
- DBMS/data storage module (slide 6, "Data Storage") isn't implemented
  in this prototype — only load balancing, gateway logic, and thread pool
- Rate limiter and worker state are in-memory only, no persistence

## Next steps for Phase II
- Real async socket listener (`epoll`/raw sockets) instead of simulated requests
- Zero-copy HTTP request parsing with `std::string_view`
- Connect a database layer for request/worker metrics
- Real health-check pings (TCP/HTTP) to workers instead of a manual flag
