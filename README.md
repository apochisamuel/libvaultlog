# libvaultlog

A segmented time-series storage engine with compaction vaultlogs,
snapshot isolation, cursor management, WAL replay, and lease-based
retention.

## Overview

libvaultlog organizes time-series data into segments that are managed
through a vaultlog-based compaction system. Segments are created,
appended to, sealed when full, and eventually merged during compaction.
The library supports snapshot isolation for consistent reads, cursor-based
iteration, write-ahead logging for crash recovery, and a lease-based
retention system for garbage collection.

## Architecture

The library is organized into 16 subsystems:

- **core** — arena allocator, byte utilities, ring buffer, error codes
- **wire** — nested frame encode/decode, CRC validation, protocol conformance
- **crc** — CRC-32C (Castagnoli) integrity checking
- **segment** — time-series data segment storage, block format, validation
- **index** — timestamp-to-offset B-tree index with range queries
- **cursor** — read cursors with position tracking, range scans, filtering
- **snapshot** — point-in-time immutable views with diff and merge
- **compact** — segment merge, compaction planning, vaultlog-aware scheduling
- **wal** — write-ahead log with checkpoint, replay, and crash recovery
- **lease** — segment leases with expiry, renewal, and chain linking
- **retention** — multi-policy GC engine with lease-aware sweeping
- **vaultlog** — compaction boundary tracking, zone management, debt computation
- **catalog** — segment metadata registry with LRU eviction and access tracking
- **clock** — logical clock, epoch management, history tracking
- **slab** — LIFO pool-based memory management with type tagging
- **session** — session lifecycle, frame dispatch, migration protocol

## Wire Protocol

All data is exchanged as nested length-prefixed frames with CRC integrity.

## Building

```bash
make
```

## Testing

```bash
# Unit tests
clang -std=c11 -I include tests/test_wire.c src/crc/crc32.c src/wire/wire_decode.c -o test_wire && ./test_wire
```

## License

MIT — see LICENSE file.
