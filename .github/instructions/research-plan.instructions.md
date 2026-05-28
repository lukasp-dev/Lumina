# Research Plan — Lumina

## 핵심 아이디어

> KV store event loop에서 CXL 메모리 접근을 기다리는 동안, 독립적인 다음 요청을 투기적으로 실행(OoO)하여 CXL 레이턴시를 숨긴다.

## Novelty Claim

기존 CXL 메모리 티어링은 OS 레벨 (page-grained, reactive, semantics-blind).
Lumina는:
- **Finer granularity** (per-key vs per-page)
- **Lower overhead** (inline tracking vs page fault traps)
- **Proactive placement** (exact access knowledge vs statistical sampling)
- **Latency hiding** via speculative operation reordering (no existing system does this)

## 12-Week Roadmap

```
Week  1-2:   [Phase 0] io_uring TCP echo server
Week  3-5:   [Phase 1] Lock-free hash map + benchmarks
Week  6-7:   [Phase 2] Binary protocol + working KV store (baseline)
Week  8-9:   [Phase 3] CXL memory tiering + TPP comparison
Week 10-12:  [Phase 4] Speculative OoO execution (core novelty)
```

## Phase 0: io_uring TCP Echo Server (Week 1-2)

### Tasks
- [ ] CMakeLists.txt: Linux, C++20, link liburing
- [ ] LuminaSocket.hpp: Linux 호환 확인
- [ ] IoRing RAII wrapper (io_uring 초기화/정리, SQE/CQE 관리)
- [ ] Accept → Read → Echo → Write 루프
- [ ] 벤치마크: connections/sec, RTT

## Phase 1: Lock-Free Hash Map (Week 3-5)

### Tasks
- [ ] Open-addressing, Robin Hood hashing
- [ ] 64-byte cache-line aligned buckets
- [ ] Lock-free read (atomic load), in-place update (CAS)
- [ ] Custom slab allocator (per-thread arena)
- [ ] Concurrent stress test
- [ ] Benchmarks vs std::unordered_map, abseil, folly
- [ ] perf stat + flame graph

## Phase 2: Binary Protocol + Event Loop (Week 6-7)

### Tasks
- [ ] 9-byte binary RPC: [1B opcode][4B key_len][4B val_len][key][value]
- [ ] Zero-copy parsing (std::span)
- [ ] Connection state machine
- [ ] io_uring + hash map integration
- [ ] YCSB benchmark: ops/sec, P50/P99/P999

## Phase 3: CXL Memory Tiering (Week 8-9)

### Tasks
- [ ] QEMU CXL Type 3 device setup (Linux 6.x)
- [ ] /dev/dax mmap userspace access
- [ ] MemoryTier abstraction (DRAMTier / CXLTier)
- [ ] Per-key access counter (8-bit decaying or Count-Min Sketch)
- [ ] Background migration (SPSC ring buffer, lock-free)
- [ ] Benchmark: All-DRAM vs Tiered vs TPP

## Phase 4: Speculative OoO Execution (Week 10-12)

### Core Mechanism
```
Traditional: [req1: CXL GET] ─── wait 200ns ───→ [respond] → [req2]
Lumina OoO:  [req1: CXL GET fire] → [req2: DRAM GET] → [req1 arrives, respond]
```

### Tasks
- [ ] Request dependency analysis (key conflict check)
- [ ] Speculative execution engine (async CXL fetch + independent req processing)
- [ ] Reorder buffer (response ordering)
- [ ] Correctness: RAW hazard detection, same-key ordering
- [ ] Benchmarks: OoO OFF vs ON, various CXL latencies, speedup curves

## Target Venues
- **First target:** HotStorage (short paper, 4-6 pages)
- **Stretch:** ATC, EuroSys, ASPLOS

## Related Work Positioning

| System | What it does | Our advantage |
|--------|-------------|---------------|
| TPP (ASPLOS '23) | OS page-level tiering | We do key-level, proactive |
| Pond (ASPLOS '23) | CXL memory pooling | No app semantics; we have exact access info |
| KAI / Suyeon Lee (2025) | CXL async back-streaming protocol | Complementary (protocol layer); we're app layer |
| CXL-SpecKV (2025) | FPGA speculative prefetch | Hardware approach; we're pure software |
| Garnet (Microsoft) | Thread-per-core KV | No CXL support |
| DragonflyDB | Redis replacement | No CXL, no tiering |

## Required Resources
- Linux (WSL2 Ubuntu or native)
- QEMU (CXL emulation)
- liburing, google/benchmark, abseil
- (optional) Real CXL hardware: Intel DevCloud or university lab
