# Roadmap: Distributed KV Store (C++, built from scratch)

Goal: build deep DSA + system design understanding by hand-writing every layer, without relying on ready-made frameworks (unless a step explicitly says a library is allowed).

Read `00b-prerequisites.md` for the consolidated list of concepts to study before/while working
through each stage.

Working principles:
- Each stage has its own guide file in `docs/` — read it before writing any code.
- You write the code. Claude only reviews / explains / answers questions when asked.
- Each stage ends with a self-check + tests you write yourself before moving to the next stage.
- Strict ordering isn't mandatory, but it's recommended since later stages build on earlier ones.

## Stage 1 — Core Data Structures (in progress)
- [ ] 1.1 Hash Table (separate chaining) — see `01-hash-table.md`
- [ ] 1.2 Skip List or B-Tree (ordered data)
- [ ] 1.3 Write-Ahead Log (WAL) for durability
- [ ] 1.4 LSM Tree (combining memtable + WAL + on-disk SSTables)

## Stage 2 — Networking (hand-written, no HTTP framework)
- [ ] 2.1 Raw TCP server (blocking, single client) — understand the socket API first
- [ ] 2.2 Design a binary wire protocol (Redis RESP-style)
- [ ] 2.3 Concurrency: thread pool first, epoll later (needs a real Linux/WSL environment)
- [ ] 2.4 Connection pooling, timeout handling, backpressure

## Stage 3 — Distributed Systems
- [ ] 3.1 Consistent hashing to shard data
- [ ] 3.2 Simple leader-follower replication
- [ ] 3.3 Raft consensus (an existing Raft library is allowed here, to learn the mechanism)
- [ ] 3.4 Gossip protocol

## Stage 4 — Optimization
- [ ] 4.1 Load balancing / request routing across shards (tie back to linear programming)

## Environment notes
- Current machine: Windows, with MSYS2 g++ 14.2 (ucrt64) and cmake 3.27. No real Linux WSL distro yet (only docker-desktop, currently stopped) — install a distro (e.g. Ubuntu) before reaching the epoll part of Stage 2.3.
- Known gotcha: MSYS2's default dynamic link against `libstdc++-6.dll` fails with `ld returned 116 exit status` when linking any program that uses `<iostream>`/`<string>`/STL in this environment. Workaround: build with `-static` (or `-static-libgcc -static-libstdc++` if you only want the C++ runtime statically linked). This is a toolchain/machine issue, not a code bug — try that flag before debugging your code if you hit it again.
