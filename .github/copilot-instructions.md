# Copilot Instructions — Lumina

## Project Vision

Lumina is a **CXL-aware in-memory key-value store with speculative operation execution**. It intelligently tiers data across local DRAM and CXL-attached memory, and hides CXL latency by reordering independent operations (application-level out-of-order execution).

Target: HotStorage / ATC / EuroSys paper submission.

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Debug build:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

## Architecture

### Core Design Principles
- **Shared-Nothing, Thread-per-Core** — no mutexes, no cross-thread sharing. Each core owns its partition.
- **io_uring Event Loop** — Linux io_uring for async I/O (no epoll/kqueue). Batched syscalls via SQ/CQ.
- **Binary RPC** — fixed 9-byte binary protocol: `[1B opcode][4B key_len][4B val_len][key][value]`
- **Zero-Copy Pipeline** — network buffers mapped directly to C++ structs via `std::span`/`std::string_view`.
- **Tiered Memory** — hot data on local DRAM, cold data on CXL memory (`/dev/dax` mmap).
- **Speculative OoO Execution** — when a request hits CXL memory, fire async fetch and process independent requests while waiting.

### Key Mechanism: Speculative Operation Execution
```
Traditional: [req1: CXL GET] ─── wait 200ns ───→ [respond] → [req2]
Lumina OoO:  [req1: CXL GET fire] → [req2: DRAM GET process] → [req1 arrives, respond]
```
This is the core novelty — application-level out-of-order execution to hide CXL latency.

### Memory Tiering Strategy
- Application-level placement (NOT OS-level page tiering like TPP)
- Per-key access frequency tracking (lightweight decaying counter)
- Background async migration: demote (DRAM→CXL) / promote (CXL→DRAM)
- Lock-free handoff via SPSC ring buffer between event loop and migration thread

### Directory Layout
- `include/` — public headers, organized by subsystem (e.g., `include/network/`)
- `src/` — implementation files; `main.cpp` is the entry point

## Tech Stack

| Component | Choice |
|-----------|--------|
| Language | C++20 |
| Platform | Linux (kernel 6.x+) |
| I/O | io_uring (liburing) |
| Local memory | Custom slab allocator (cache-line aligned) |
| CXL memory | DAX/mmap on /dev/dax (QEMU emulation for dev) |
| Hash map | Open-addressing, Robin Hood hashing, lock-free reads (CAS updates) |
| Concurrency | Shared-nothing, thread-per-core |
| Benchmarks | google/benchmark, YCSB workloads, perf, flame graphs |

## Development Roadmap (12 weeks)

```
Week  1-2:   [Phase 0] io_uring TCP echo server
Week  3-5:   [Phase 1] Lock-free hash map + benchmarks
Week  6-7:   [Phase 2] Binary protocol + working KV store (baseline)
Week  8-9:   [Phase 3] CXL memory tiering + TPP comparison
Week 10-12:  [Phase 4] Speculative OoO execution (core novelty)
```

## Conventions

### C++ style
- **C++20 standard** — use concepts, ranges, `std::span`, structured bindings where they improve clarity.
- **Namespace**: all code lives under `namespace lumina { }`.
- **RAII everywhere** — resources (FDs, memory, io_uring) must be wrapped in RAII classes with deleted copy ctors/assignment and explicit move semantics using `std::exchange`.
- **`[[nodiscard]]`** on accessors and factory functions.
- **`noexcept`** on move operations and trivial accessors.
- **Errors**: throw `std::system_error` with `errno` for syscall failures during construction; use error codes or return values on the hot path (never throw in the event loop).
- **Cache-line alignment**: use `alignas(64)` on hot data structures to avoid false sharing.

### Naming
- Classes: `PascalCase` (e.g., `LuminaSocket`, `IoRing`)
- Methods/functions: `snake_case`
- Private members: trailing underscore (`fd_`, `ring_`)
- Files: `PascalCase.hpp` / `PascalCase.cpp` matching the primary class

### Git commits
Follow [Conventional Commits](https://www.conventionalcommits.org/):
```
<type>(<scope>): <imperative summary>
```
Types: `feat`, `fix`, `docs`, `refactor`, `perf`, `test`, `chore`, `build`, `ci`

## Research Context

### Related Work (know these for positioning)
- **TPP (ASPLOS '23)** — OS-level page tiering for CXL. Our baseline to beat.
- **Pond (ASPLOS '23)** — CXL memory pooling, no app semantics.
- **Suyeon Lee / KAI (arXiv:2512.04449)** — Async back-streaming protocol for CXL computational memory. Complementary (protocol layer), not competing.
- **CXL-SpecKV (2025)** — FPGA-based speculative prefetch. Hardware approach vs our software approach.
- **Garnet (Microsoft)** — Thread-per-core KV store, no CXL.
- **DragonflyDB** — Multi-threaded Redis replacement, no CXL.
- **FASTER (Microsoft)** — Log-structured hash store. Inspiration for lock-free updates.

### Novelty Claim
No existing system combines a high-performance lock-free KV store with application-informed CXL memory tiering AND speculative operation reordering at the application layer. OS-level tiering is coarse (page-grained, reactive); we are fine-grained (key-level, proactive) and hide residual CXL latency via OoO execution.

## References
- liburing: https://github.com/axboe/liburing
- QEMU CXL: https://www.qemu.org/docs/master/system/devices/cxl.html
- Linux DAX: https://docs.kernel.org/filesystems/dax.html
- FASTER: https://github.com/microsoft/FASTER
- DragonflyDB: https://github.com/dragonflydb/dragonfly
