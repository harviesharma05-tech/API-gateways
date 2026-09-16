# Asynchronous HTTP API Gateway — Prototype (Phase I)

Team ID: DSCPP-III-2026-T035

## What this is
A minimal C++ prototype demonstrating the **dynamic load balancing** core
of the gateway — picking the healthy backend worker with the smallest
queue size. This is the Phase-I initial module referenced in the slides.

## Build & Run

```bash
g++ -std=c++17 -O2 -o gateway src/main.cpp
./gateway
```

## Next steps (Phase II)
- Wire this selection logic into an actual async HTTP listener
- Add Consistent Hash Ring for routing
- Add Token-Bucket rate limiter
- Add background health-check thread + failover
- Add thread-pool for concurrent request handling
