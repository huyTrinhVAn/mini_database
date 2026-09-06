# Prerequisite Knowledge

A consolidated reading list for the whole project, organized by stage. Treat this as "read/understand
before you start coding that stage," not something to finish 100% before touching the keyboard —
some of it (e.g. Raft, gossip) only really clicks once you've built the simpler pieces first.

## Foundational (needed from day one)

- **C++ language mechanics**: templates, references vs pointers, RAII, move semantics
  (`std::move`, rvalue references), smart pointers (`unique_ptr`/`shared_ptr`), `const` correctness.
  You'll lean on all of this immediately in Stage 1.
- **Big-O and amortized analysis**: not just "what's the complexity" but *why* — e.g. why a
  dynamic array's `push_back` is amortized O(1) even though individual calls sometimes cost O(n).
  This reasoning pattern reappears for hash table rehashing and LSM compaction.
- **Build tooling basics**: how a compiler turns source into an object file and a linker turns
  object files into an executable (this project already made you hit a real linker issue — worth
  understanding roughly what `ld` is doing, not just memorizing the workaround flag).

## Stage 1 — Core Data Structures

- Hash tables: see `01-hash-table.md` for the focused breakdown.
- **Balanced ordered structures**: Skip List (probabilistic balancing) or B-Tree (the structure
  real databases use for on-disk indexes). Understand why a plain BST degrades to O(n) on sorted
  input and what each alternative does about it.
- **Durability basics**: what "durability" actually guarantees, why appending to a log file is
  fast (sequential disk/SSD writes vs random writes), and what `fsync` does (and why skipping it
  breaks the durability guarantee even though everything "looks" written).
- **LSM Tree concept**: memtable (in-memory, ordered), flushing to immutable SSTables on disk,
  and compaction (merging SSTables, removing overwritten/deleted keys). Read how RocksDB or
  Cassandra describes this at a conceptual level — you don't need their source code, just the
  read/write path and why LSM trades read amplification for write throughput compared to a B-Tree.

## Stage 2 — Networking

- **TCP/IP fundamentals**: what a socket is, the client/server socket call sequence
  (`socket`/`bind`/`listen`/`accept` vs `socket`/`connect`), blocking vs non-blocking I/O.
- **I/O multiplexing**: `select`/`poll`/`epoll` — what problem they solve (handling many
  connections without one thread per connection) and why `epoll` scales better than `select`
  (hint: O(1) vs O(n) per wait call, and why that matters at thousands of connections).
- **Concurrency models for servers**: thread-per-connection vs thread pool vs single-threaded
  event loop (this is the same design question Redis, Nginx, and Node.js each answered differently
  — worth knowing what each one chose and why).
- **Binary protocol design**: how to frame messages over a byte stream (length-prefixing vs
  delimiter-based, like Redis RESP's `\r\n`-terminated frames), and why "just send a struct" isn't
  safe across machines (endianness, padding).
- **Backpressure**: what happens when a server accepts data faster than it can process it, and
  the basic strategies (bounded queues, blocking the producer, dropping).

## Stage 3 — Distributed Systems

- **CAP theorem**: what consistency, availability, and partition tolerance actually mean here,
  and why it's a real tradeoff and not just trivia — you'll be making this tradeoff concretely
  when you choose your replication model.
- **Consistent hashing**: why naive `hash(key) % num_nodes` falls apart when nodes are added or
  removed, and how consistent hashing (plus virtual nodes) minimizes reshuffling.
- **Replication**: leader-follower (single-leader) replication, synchronous vs asynchronous
  replication, and what each choice does to consistency and availability during a failure.
- **Consensus (Raft)**: leader election, log replication, and the safety property being provided
  (all nodes agree on the same sequence of operations). You're allowed to use an existing Raft
  library for this project — but understand what problem it's solving before wiring it in.
- **Gossip protocols**: how nodes learn about cluster membership/failure without a central
  coordinator, and why this trades immediate consistency of membership info for scalability and
  resilience.
- **Failure detection**: heartbeats, timeouts, and why distinguishing "node is dead" from
  "node/network is just slow" is fundamentally hard (this motivates a lot of the design choices
  above).

## Stage 4 — Optimization

- **Linear programming basics**: objective function, constraints, feasible region — enough to
  frame "distribute load across shards to minimize max load" as an optimization problem, even if
  you end up implementing a simpler heuristic in practice.
- **Common load-balancing strategies**: round robin, least-connections, and consistent-hashing-as
  load-balancing — and being able to articulate the tradeoff between them in plain language for an
  interview setting.

## How to use this list
Don't try to master everything up front. Read the Stage 1 section now, keep the rest as a map of
what's coming, and revisit each section right before you start that stage — concepts like Raft or
gossip will make far more sense once you've felt the problems they solve (e.g. after you've
manually dealt with a naive replication bug).
