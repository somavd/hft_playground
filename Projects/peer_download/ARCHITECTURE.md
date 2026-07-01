# Peer Download — Architecture & Code Walkthrough

## Overview

A distributed file downloader where multiple peer nodes split a large file into 1 MB chunks, each peer downloads its assigned subset via HTTP range requests, and reassembles the file locally. A coordinator manages cluster membership and chunk assignment.

---

## Components

```
coordinator          peer_node (×N)
    │                    │
    │◄── HELLO ──────────│   Peer announces itself
    │── CHUNK_ASSIGN ──► │   Coordinator sends chunk list
    │◄── LEAVE ──────────│   Peer departs
    │                    │
    │  (repeat on join/leave: reassign + broadcast)
```

### Binaries

| Binary        | Source                              | Role                                |
|---------------|-------------------------------------|-------------------------------------|
| `coordinator` | `desktop-test-harness/coordinator.cpp` | Manages cluster, assigns chunks     |
| `peer_node`   | `desktop-test-harness/peer_node.cpp`   | Downloads chunks, reassembles file  |

### Core Libraries

| File                  | Purpose                                        |
|-----------------------|------------------------------------------------|
| `chunk_manager.h/cpp` | Chunk math, peer registry, round-robin assignment |
| `peer_discovery.h/cpp`| TCP messaging (HELLO, LEAVE, CHUNK_ASSIGNMENT) |
| `http_range_fetch.h/cpp` | HTTP range download via libcurl              |
| `hasher.h/cpp`        | SHA-256 hashing (OpenSSL)                      |
| `reassembler.h/cpp`   | Reassemble chunk files into output at correct offsets |

---

## Data Structures

### Chunk (`chunk_manager.h`)

```cpp
struct Chunk {
    uint32_t id;           // 0-indexed chunk number
    uint64_t start_byte;   // Inclusive start offset
    uint64_t end_byte;     // Inclusive end offset
    std::string hash;      // SHA-256 after download
    bool downloaded;        // Completion flag
};
```

### Peer (`chunk_manager.h`)

```cpp
struct Peer {
    uint32_t id;                          // Unique node ID
    std::string address;                  // "ip:port"
    int socket_fd;                        // Unused in MVP (-1)
    std::bitset<MAX_CHUNKS> chunk_bitmap; // Which chunks assigned
};
```

### Constants

```cpp
CHUNK_SIZE  = 1,048,576  (1 MB)
MAX_CHUNKS  = 10,000     (supports files up to ~10 GB)
```

---

## Wire Protocol (`peer_discovery.h`)

All messages are TCP. Fields are in **network byte order** (big-endian).

### Message Types

| Type             | ID | Format                                    |
|------------------|----|-------------------------------------------|
| HELLO            | 1  | `[type:1][node_id:4][port:2]` = 7 bytes   |
| LEAVE            | 2  | `[type:1][node_id:4]` = 5 bytes           |
| CHUNK_ASSIGNMENT | 3  | `[type:1][count:4][chunk_id:4]×N`         |

### HELLO (peer → coordinator)

Sent when a peer starts. Contains node ID and listen port.

### LEAVE (peer → coordinator)

Sent on graceful shutdown. Coordinator removes peer and reassigns.

### CHUNK_ASSIGNMENT (coordinator → peer)

Sent to ALL peers whenever the cluster changes. Each peer receives the list of chunk IDs it should download.

---

## Control Flow

### Startup Sequence

```
1. Coordinator starts, listens on TCP port (e.g. 5001)
2. Peer N starts:
   a. Creates work directory: node_N/
   b. Initializes ChunkManager with file size → computes chunk list
   c. Adds self to local peer registry
   d. Starts TCP listener on its port (e.g. 6001)
   e. Sends HELLO to coordinator
   f. Waits for CHUNK_ASSIGNMENT (up to 10s)
3. Coordinator receives HELLO:
   a. Adds peer to registry
   b. Runs round-robin assignment across all peers
   c. Sends CHUNK_ASSIGNMENT to EVERY peer (not just the new one)
4. Peer receives CHUNK_ASSIGNMENT:
   a. Updates local bitmap with assigned chunk IDs
   b. Begins downloading
```

### Download Sequence (per peer)

```
For each assigned chunk ID:
  1. Compute byte range: [id * CHUNK_SIZE, min((id+1)*CHUNK_SIZE - 1, file_size - 1)]
  2. HTTP GET with Range header → save to node_N/chunk_ID.bin
  3. SHA-256 hash the chunk file
  4. Mark chunk as downloaded in ChunkManager
  5. Log progress every 10 chunks
```

### Reassembly Sequence

```
1. Collect all downloaded chunks with their file paths and offsets
2. Pre-allocate output file to total_size bytes (ftruncate)
3. For each chunk: seek to offset, write chunk data
4. SHA-256 hash the final file
5. Print hash for verification
```

### Shutdown

```
1. Peer sends LEAVE to coordinator
2. Coordinator removes peer, reassigns remaining peers
3. Peer stops TCP listener
```

---

## Round-Robin Assignment

```cpp
void ChunkManager::assignChunksRoundRobin() {
    // Clear all bitmaps
    // For chunk i: assign to peers[i % num_peers]
}
```

Example with 51 chunks, 2 peers:
- Peer 1 (index 0): chunks 0, 2, 4, 6, ... → 26 chunks
- Peer 2 (index 1): chunks 1, 3, 5, 7, ... → 25 chunks

---

## File Layout

```
peer_download/
├── CMakeLists.txt
├── ARCHITECTURE.md              ← This file
├── architecture.html            ← Visual documentation
├── core/
│   ├── chunk_manager.h/cpp      ← Chunk math + peer registry
│   ├── peer_discovery.h/cpp     ← TCP protocol (HELLO/LEAVE/ASSIGN)
│   ├── http_range_fetch.h/cpp   ← HTTP range download (libcurl)
│   ├── hasher.h/cpp             ← SHA-256 (OpenSSL)
│   ├── reassembler.h/cpp        ← Chunk → file reassembly
│   ├── chunker.h/cpp            ← V1 chunking (legacy)
│   └── chunk_protocol.h/cpp     ← V1 chunk exchange (legacy)
├── desktop-test-harness/
│   ├── coordinator.cpp          ← Coordinator binary
│   ├── peer_node.cpp            ← Peer node binary
│   ├── peer_dl.cpp              ← V1 single-chunk CLI (legacy)
│   └── reassemble.cpp           ← V1 reassemble CLI (legacy)
├── tests/
│   ├── test_chunker.cpp
│   └── test_hasher.cpp
└── build/                       ← Build output
    ├── coordinator
    ├── peer_node
    └── node_N/                  ← Per-node chunk directories
```

---

## Running

### Build

```bash
cd build && cmake .. && make
```

### Test with 50MB file (2 peers)

Terminal 1 — Coordinator:
```bash
./coordinator --port=5001 \
  --url=https://thetestdata.com/assets/video/mp4/1080/50MB_1080P_THETESTDATA.COM_mp4.mp4 \
  --size=52636528
```

Terminal 2 — Peer 1:
```bash
./peer_node --node_id=1 --port=6001 --coordinator=127.0.0.1:5001 \
  --url=https://thetestdata.com/assets/video/mp4/1080/50MB_1080P_THETESTDATA.COM_mp4.mp4 \
  --size=52636528 --out=video1.mp4
```

Terminal 3 — Peer 2:
```bash
./peer_node --node_id=2 --port=6002 --coordinator=127.0.0.1:5001 \
  --url=https://thetestdata.com/assets/video/mp4/1080/50MB_1080P_THETESTDATA.COM_mp4.mp4 \
  --size=52636528 --out=video2.mp4
```

### Expected output

```
[COORD] node 1 joined (127.0.0.1:6001)
[COORD] -> node 1: 51 chunks assigned
[COORD] node 2 joined (127.0.0.1:6002)
[COORD] -> node 1: 26 chunks assigned
[COORD] -> node 2: 25 chunks assigned

[NODE 1] downloading 51/51 chunks
[NODE 1] progress: 10/51 chunks downloaded
...
[NODE 1] DONE video1.mp4  SHA-256=...

[NODE 2] downloading 25/51 chunks
[NODE 2] progress: 10/25 chunks downloaded
...
[NODE 2] DONE video2.mp4  SHA-256=...
```

---

## Dependencies

- **libcurl** — HTTP range requests
- **OpenSSL** — SHA-256 hashing
- **C++17** — structured bindings, std::filesystem
- **POSIX sockets** — TCP networking

---

## Current Limitations (MVP)

1. **No P2P chunk exchange** — Each peer downloads from the HTTP server only. Peers don't share chunks with each other yet.
2. **No chunk verification across peers** — Each peer hashes its own chunks but doesn't verify against a master manifest.
3. **Assignment is snapshot-based** — If a peer receives a new assignment mid-download, it finishes the old download list. The new assignment is used if the peer restarts.
4. **No resume support** — If a peer crashes, it re-downloads all assigned chunks on restart.
