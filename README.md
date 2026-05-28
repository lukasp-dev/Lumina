# Lumina

An Low latency, Lock Free In-Memory, Key-Value Store.

[![C++20](https://img.shields.io/badge/C++-20-blue.svg)](https://isocpp.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()

Lumina is a custom-built, high-performance in-memory database engine written from scratch in Modern C++ (C++20). Designed with strict adherence to **Mechanical Sympathy**, Lumina bypasses the overhead of traditional multi-threaded web servers to achieve sub-millisecond P99 latencies, making it ideal for the extreme demands of quantitative trading and matching engines.

## Motivation: Why Lumina?

While general-purpose in-memory data stores (like Redis r Memcached) are exceptionally fast for standard web applications, they introduce unacceptable jitter and overhead for High-Frequency Trading (HFT) and ultra-low latency environments.

Lumina was engineered to address the exact bottlenecks of traditional engines:

**1. Bypassing Protocol Overhead:** Traditional stores use text-based protocols (like RESP) that require CPU-heavy string parsing and branching. Lumina strictly uses a heavily optimized, zero-copy **9-byte Binary RPC**.
 
**2. Eradicating Lock Contention:** Multi-threaded databases suffer from mutex bottlenecks and false sharing. Lumina adopts a **Shared-Nothing, Thread-per-Core architecture** (inspired by ScyllaDB and LMAX Disruptor) to completely eliminate locking overhead.

**3. Mechanical Sympathy:** General dynamic allocation (`malloc/new`) causes heap fragmentation and CPU cache misses. Lumina relies on **Cache-line aligned Custom Memory Pools** to maximize L1/L2 cache hit rates.

**4. Optimized for Write_Intensive Workloads:** HFT environments (e.g., continuous order book updates and cancellations) are overwhelmingly write-heavy. Traditional engines stall under massive write throughput. Lumina utilizes **lock-free, in-place updates** (drawing inspiration from Microsoft's FASTER architecture) to ensure that extreme write volumes never block the event loop.

*Lumina is not a general-purpose database; it is a highly specialized engine for scenarios where a single microsecond dictates success or failure.*

## Tech Stack & Core Mechanisms

- **Language**: Modern C++20
- **Concurrency**: Shared-Nothing, Single-threaded per core
- **I/O Multiplexing**: Edge-triggered `kqueue` (macOS/FreeBSD)
- **Memory Management**: Custom Memory Pools, Strict RAII
- **Network**: Fully Asynchronous, Non-blocking (`O_NONBLOCK`) I/O
- **Data Transfer**: Zero-copy packet parsing via `std::span` and `std::string_view`

## Architecture

### 1. The Network Layer
Lumina's network layer is engineered to feed the database engine without ever stalling the CPU.
- **RAII-Driven Resources**: All file descriptors and sockets are wrapped in strict RAII classes. Copy constructors are explicitly deleted (`= delete`) to prevent double-free errors, and ownership is transferred purely via zero-overhead Move Semantics (`std::move` / `std::exchange`).
- **100% Non-Blocking I/O**: Every socket operates in `O_NONBLOCK` mode. The system fails fast (`EAGAIN` / `EWOULDBLOCK`) rather than suspending the thread, ensuring the event loop never pauses.
- **kqueue Event Loop**: Replaces traditional thread-per-connection models, allowing a single core to monitor thousands of concurrent connections while eliminating context-switching latency.

### 2. The Storage Engine
- **In-Memory Hash Map**: A highly optimized, cache-aligned hash index designed for rapid point lookups and in-place mutations.
- **Zero-Copy Pipeline**: Data read from the kernel buffer is directly mapped to C++ `structs`. The database engine reads opcodes and payloads straight from the network buffer without deep copying.