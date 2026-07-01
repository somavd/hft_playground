# P2P Chunk Download System — Design Specification

## Overview

A distributed file download system where the **coordinator node starts downloading immediately** and dynamically includes new peers as they join. Every node (including coordinator) downloads chunks, shares with peers, and ends up with the complete file.

---

## Core Principles

1. **Coordinator downloads too** — it's a worker + orchestrator
2. **Dynamic peer inclusion** — peers join mid-download, immediately get assigned work
3. **Fully async** — no thread blocks waiting for another node
4. **Every node gets all chunks** — download + peer sharing = complete file everywhere

---

## Architecture

```
┌─────────────────────────────────────────────────────┐
│                    NODE (every node)                  │
├─────────────────────────────────────────────────────┤
│                                                      │
│  ┌──────────────┐  ┌──────────────┐  ┌───────────┐ │
│  │  Network     │  │  HTTP Worker │  │   Peer    │ │
│  │  Thread      │  │  Thread      │  │  Exchange │ │
│  │              │  │              │  │  Thread   │ │
│  │ TCP listen + │  │ Pulls from   │  │           │ │
│  │ msg dispatch │  │ download     │  │ Request + │ │
│  │              │  │ queue        │  │ serve     │ │
│  └──────┬───────┘  └──────┬───────┘  └─────┬─────┘ │
│         │                  │                │       │
│         └──────────┬───────┴────────────────┘       │
│                    │                                 │
│         ┌──────────▼──────────┐                     │
│         │   Shared State      │                     │
│         │   (mutex-protected) │                     │
│         └─────────────────────┘                     │
│                                                      │
│  ┌──────────────┐  (coordinator only)               │
│  │ Coordinator  │                                    │
│  │ Thread       │  Assigns chunks, tracks peers     │
│  └──────────────┘                                    │
└─────────────────────────────────────────────────────┘
```

---

## Threads (per node)

| Thread | Purpose | Runs On |
|--------|---------|---------|
| **Network** | TCP server, accepts connections, dispatches messages to callbacks | All nodes |
| **HTTP Worker** | Consumes HTTP download tasks from queue, fetches byte ranges | All nodes |
| **Peer Exchange** | Periodically requests missing chunks from known peers; serves incoming requests | All nodes |
| **Coordinator** | Assigns next chunk to least-busy peer, handles peer join/leave | Coordinator only |

---

## Data Structures

### ChunkState (per chunk)
```cpp
enum class ChunkStatus { PENDING, ASSIGNED, DOWNLOADING, COMPLETED };

struct ChunkState {
    uint32_t    id;
    uint64_t    start_byte;
    uint64_t    end_byte;
    ChunkStatus status;
    uint32_t    assigned_to;  // node_id, 0 = unassigned
};
```

### PeerInfo
```cpp
struct PeerInfo {
    uint32_t    node_id;
    std::string ip;
    uint16_t    port;
    std::set<uint32_t> has_chunks;  // chunks this peer has completed
};
```

### SharedState (mutex-protected)
```cpp
struct SharedState {
    std::mutex                          mu;
    std::vector<ChunkState>            chunks;         // all chunk metadata
    std::set<uint32_t>                 my_chunks;      // chunks I have locally
    std::map<uint32_t, PeerInfo>       peers;          // known peers
    std::queue<uint32_t>               http_queue;     // chunk_ids to download via HTTP
    std::condition_variable            http_cv;        // wake HTTP worker
    std::condition_variable            done_cv;        // signal completion
    std::atomic<bool>                  shutdown{false};
};
```

---

## Thread Responsibilities

### 1. Network Thread
- Accepts TCP connections
- Parses message type, dispatches to registered callback
- Non-blocking: each connection handled in detached thread

### 2. HTTP Worker Thread
```
loop:
    wait on http_queue (condition variable)
    pop chunk_id from queue
    fetch byte range from HTTP server
    save to disk
    mark chunk COMPLETED in shared state
    notify coordinator (CHUNK_COMPLETE message)
```

### 3. Peer Exchange Thread
```
loop (every 500ms or on signal):
    for each missing chunk:
        find a peer that has it
        send CHUNK_DATA_REQUEST (fire-and-forget)
    
    // Incoming requests handled via callback:
    on CHUNK_DATA_REQUEST:
        read chunk from disk
        send CHUNK_DATA back to requester
```

- Requests are non-blocking: fire all at once, don't wait for individual responses
- Responses arrive async via `onChunkData` callback

### 4. Coordinator Thread (coordinator only)
```
loop:
    if unassigned chunks exist AND idle peers available:
        assign next chunk to next peer (round-robin)
        send CHUNK_REQUEST to that peer
    
    on CHUNK_COMPLETE from peer:
        mark chunk completed
        assign next unassigned chunk to that peer
    
    on PEER_JOIN:
        add to peer list
        immediately assign it a chunk
    
    on PEER_LEAVE:
        reassign its in-progress chunks
```

---

## Message Protocol (TCP)

| Type | ID | Format | Direction |
|------|----|--------|-----------|
| HELLO | 1 | `[type:1][node_id:4][port:2]` | Peer → Coordinator |
| LEAVE | 2 | `[type:1][node_id:4]` | Peer → Coordinator |
| CHUNK_REQUEST | 3 | `[type:1][chunk_id:4][source_type:1][source_ip:4][source_port:2]` | Coordinator → Peer |
| CHUNK_COMPLETE | 4 | `[type:1][node_id:4][chunk_id:4]` | Peer → Coordinator |
| CHUNK_HAVE | 5 | `[type:1][node_id:4][count:4][chunk_ids:4*N]` | Any → Any |
| CHUNK_DATA_REQUEST | 6 | `[type:1][chunk_id:4][listen_port:2]` | Peer → Peer |
| CHUNK_DATA | 7 | `[type:1][chunk_id:4][data_len:4][data:N]` | Peer → Peer |

- `source_type`: 0 = HTTP, 1 = peer
- All multi-byte integers: network byte order (big-endian)
- Large payloads (CHUNK_DATA): sent with `sendAll`/`recvAll` loops

---

## Flow: Dynamic Peer Inclusion

```
Timeline:
─────────────────────────────────────────────────────►

Coordinator starts, begins downloading chunks 0,1,2,3...
                │
                ▼
        [chunk 0 done] [chunk 1 done] [chunk 2 done]
                                        │
                                        │  Peer A joins
                                        ▼
        Coordinator assigns chunk 5 to Peer A
        Coordinator downloads chunk 4 itself
                │
                ▼
        [chunk 3 done] [chunk 4 done]   [Peer A: chunk 5 done]
                                                    │
                                                    │  Peer B joins
                                                    ▼
        Coordinator assigns chunk 8 to Peer B
        Coordinator downloads chunk 6
        Peer A downloads chunk 7
                │
                ▼
        ... continues until all chunks downloaded ...
                │
                ▼
        Peer Exchange phase: everyone shares missing chunks
                │
                ▼
        All nodes have all chunks → reassemble → done
```

---

## APIs

### Node API (public interface)

```cpp
class Node {
public:
    void start(const Config& config);
    void stop();
    
    // Status
    bool isComplete() const;
    uint32_t chunksDownloaded() const;
    uint32_t totalChunks() const;
};
```

### Config

```cpp
struct Config {
    uint32_t    node_id;
    uint16_t    listen_port;
    std::string file_url;
    uint64_t    file_size;
    std::string output_path;
    std::string work_dir;
    bool        is_coordinator;
    std::string coordinator_host;  // empty if is_coordinator
    uint16_t    coordinator_port;
};
```

### Internal APIs (between threads via shared state)

```cpp
// HTTP Worker consumes from:
void pushHttpTask(uint32_t chunk_id);

// Peer Exchange fires:
void requestChunkFromPeer(uint32_t chunk_id, const PeerInfo& peer);

// Any thread marks completion:
void markChunkComplete(uint32_t chunk_id);

// Coordinator assigns:
void assignChunkToPeer(uint32_t chunk_id, uint32_t peer_id);
```

---

## Chunk Assignment Strategy

1. Coordinator maintains a pointer to next unassigned chunk
2. On peer join or chunk completion → assign next chunk to that peer
3. Coordinator also assigns chunks to itself (via its own HTTP worker)
4. Round-robin when multiple peers are idle simultaneously

---

## Peer Sharing Strategy

- After HTTP download phase, each node has a subset of chunks
- Peer exchange thread fires CHUNK_DATA_REQUEST for ALL missing chunks simultaneously
- Requests are distributed across peers that have the data
- No waiting: responses arrive via async callback, saved immediately
- Coordinator tracks CHUNK_HAVE to know when everyone is done

---

## Error Handling

| Failure | Recovery |
|---------|----------|
| HTTP download fails | Retry 3 times, then mark chunk as failed and reassign |
| Peer disconnects mid-transfer | Coordinator reassigns its pending chunks |
| Peer request timeout | Retry from different peer |
| Coordinator crash | Peers cannot recover (single point of coordination) |

---

## File Layout

```
Projects/peer_download/
├── core/
│   ├── peer_discovery.h/cpp    — TCP message layer
│   ├── http_range_fetch.h/cpp  — HTTP byte-range downloads
│   ├── hasher.h/cpp            — SHA-256 file hashing
│   ├── reassembler.h/cpp       — Chunk → file assembly
│   └── chunk_manager.h/cpp     — Chunk metadata + CHUNK_SIZE constant
├── desktop-test-harness/
│   ├── coordinator.cpp         — Coordinator node main
│   └── peer_node.cpp           — Peer node main
├── CMakeLists.txt
└── DESIGN.md                   ← this file
```

---

## Constants

| Name | Value | Notes |
|------|-------|-------|
| CHUNK_SIZE | 1 MB (1048576) | Balance between parallelism and overhead |
| MAX_RETRIES | 3 | Per-chunk HTTP download retries |
| PEER_EXCHANGE_INTERVAL | 500ms | How often peer exchange thread polls |
| LISTEN_BACKLOG | 64 | TCP listen queue depth |

---

## Future Improvements

- [ ] Coordinator also downloads (currently only assigns)
- [ ] NAT traversal for non-local peers
- [ ] Chunk integrity verification (per-chunk SHA-256)
- [ ] Bandwidth-aware assignment (assign more to faster peers)
- [ ] Multi-file support
- [ ] Peer discovery via multicast/mDNS instead of explicit coordinator address
