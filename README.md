# BlockStoreSim

A lightweight C++ storage-engine simulator for **deduplication, reclamation, replication-style snapshot validation, and regression testing**.

This MVP is designed to show core storage-systems thinking in a compact codebase: content hashing, logical-to-physical block mapping, reference counts, reclaimable space, compaction, deterministic snapshots, and CI-backed tests.

## Why this exists

Storage systems are not only about writing bytes. They need to answer practical correctness questions:

- Is this payload already stored?
- Which logical records point to the same physical block?
- When is a physical block safe to reclaim?
- Did a replicated snapshot preserve data identity and metadata?
- Can regressions be caught before a code change reaches a shared branch?

`BlockStoreSim` implements these ideas without external runtime dependencies, so the behavior is easy to inspect, test, and extend.

## Features

- Deterministic content hashing using FNV-1a
- Logical key to physical block mapping
- Reference-counted physical blocks
- Deduplication for identical writes
- Safe overwrite handling
- Delete and reclaimable-space tracking
- Compaction of unreferenced blocks
- Deterministic snapshot export for replication-style validation
- Scriptable CLI
- Python regression tests
- GitHub Actions CI

## Project layout

```text
.
├── CMakeLists.txt
├── README.md
├── src/
│   ├── block_store.hpp
│   ├── block_store.cpp
│   └── main.cpp
├── tests/
│   └── test_block_store.py
└── .github/
    └── workflows/
        └── ci.yml
```

## Build

```bash
cmake -S . -B build
cmake --build build
```

You can also compile directly:

```bash
g++ -std=c++17 -O2 -Wall -Wextra src/main.cpp src/block_store.cpp -o blockstoresim
```

## Run a demo

```bash
cat << 'EOF' | ./build/blockstoresim
WRITE user_1 hello
WRITE user_2 hello
WRITE user_3 different_payload
STATS
DELETE user_1
STATS
COMPACT
STATS
SNAPSHOT
EOF
```

Example behavior:

- `user_1` and `user_2` share one physical block because their payload is identical.
- Deleting one logical key decrements the reference count but keeps the physical block because another key still uses it.
- A block becomes reclaimable only when its reference count reaches zero.
- `COMPACT` removes reclaimable physical blocks.

## CLI commands

```text
WRITE <key> <payload>   Store or overwrite a logical record
READ <key>              Print the payload for a key
DELETE <key>            Delete a logical key
STATS                   Print storage counters
COMPACT                 Remove unreferenced physical blocks
SNAPSHOT                Print deterministic key/hash/size metadata
HELP                    Show commands
EXIT                    Stop the process
```

## Run tests

```bash
python -m pip install pytest
pytest -q
```

The tests compile the C++ binary and validate deduplication, overwrite behavior, delete/reclamation behavior, compaction, and deterministic snapshot output.

## Resume framing

**BlockStoreSim: C++ Storage Engine Prototype for Deduplication and Reclamation**

- Built a C++ storage simulator with content hashing, block metadata, reference counts, and reclaimable-space tracking.
- Implemented overwrite, delete, compact, stats, and snapshot validation workflows through a scriptable CLI.
- Added automated regression tests and CI checks to validate correctness of deduplication, reclamation, and replication-style metadata export.

## Future extensions

Good next additions:

- Chunk-level deduplication instead of whole-payload deduplication
- Write-ahead log simulation
- Snapshot diffing
- Replication retry simulation
- Crash recovery tests
- Microbenchmarks for throughput and latency
- Concurrent writer stress tests
