# Peer Download v2 - API Guide

This guide explains how to use the v2 distributed download system with 1MB chunks, round-robin assignment, and dynamic node membership.

## Architecture Overview

- **Coordinator**: Manages the cluster, assigns chunks to peers, tracks peer membership
- **Peer Node**: Downloads assigned chunks, broadcasts to other peers, reassembles file
- **Chunk Size**: Fixed 1MB (1,048,576 bytes)
- **Assignment**: Round-robin (peer 0 gets chunks 0,2,4... peer 1 gets 1,3,5...)
- **Discovery**: TCP-based on localhost
- **Broadcast**: Immediate chunk broadcast after download

## Download a File

### Step 1: Start the Coordinator

```bash
cd build
./coordinator --port=5000 --url=<file_url> --size=<file_size_bytes>
```

**Example:**
```bash
./coordinator --port=5000 --url=https://www.w3schools.com/html/mov_bbb.mp4 --size=788493
```

The coordinator will:
- Initialize chunk list based on file size
- Start listening on port 5000 for peer connections
- Wait for peers to join
- Assign chunks to peers using round-robin

### Step 2: Start Peer Nodes

Open separate terminals for each peer:

**Peer 1:**
```bash
./peer_node --node_id=1 --port=6001 --coordinator=127.0.0.1:5000 --url=https://www.w3schools.com/html/mov_bbb.mp4 --size=788493 --out=video_node1.mp4
```

**Peer 2:**
```bash
./peer_node --node_id=2 --port=6002 --coordinator=127.0.0.1:5000 --url=https://www.w3schools.com/html/mov_bbb.mp4 --size=788493 --out=video_node2.mp4
```

Each peer will:
- Connect to coordinator and announce presence
- Receive chunk assignment (round-robin)
- Download assigned chunks from internet
- Broadcast CHUNK_HAVE to coordinator when download completes
- Reassemble file from downloaded chunks

### Step 3: Verify

Each peer will output the final SHA-256 hash of the reassembled file. All peers should have identical hashes.

## Add/Remove Nodes

### Add a Node

Simply start a new peer node while the coordinator is running:

```bash
./peer_node --node_id=3 --port=6003 --coordinator=127.0.0.1:5000 --url=<url> --size=<size> --out=output.mp4
```

The coordinator will:
- Detect the new peer via HELLO message
- Add peer to cluster
- Reassign chunks using round-robin
- The new peer will download its assigned chunks
- Existing peers may redistribute chunks if needed

### Remove a Node

Press Ctrl+C on the peer node terminal. The peer will:
- Send LEAVE message to coordinator
- Close connections
- Exit

The coordinator will:
- Detect peer departure via LEAVE message
- Remove peer from cluster
- Reassign chunks to remaining peers

## API Reference

### Coordinator Binary

```
./coordinator --port=<port> --url=<url> --size=<bytes>
```

**Parameters:**
- `--port`: Port to listen on for peer connections
- `--url`: File URL to download
- `--size`: File size in bytes

**Behavior:**
- Starts TCP discovery server on specified port
- Initializes chunk list based on file size
- Tracks peer membership
- Assigns chunks to peers using round-robin
- Prints cluster status every second

### Peer Node Binary

```
./peer_node --node_id=<id> --port=<port> --coordinator=<addr:port> --url=<url> --size=<bytes> --out=<file>
```

**Parameters:**
- `--node_id`: Unique ID for this peer
- `--port`: Port to listen on for peer connections
- `--coordinator`: Coordinator address (host:port)
- `--url`: File URL to download
- `--size`: File size in bytes
- `--out`: Output file path

**Behavior:**
- Connects to coordinator and announces presence
- Receives chunk assignment
- Downloads assigned chunks via HTTP range requests
- Broadcasts CHUNK_HAVE to coordinator
- Reassembles file from downloaded chunks

## Core APIs

### ChunkManager

```cpp
class ChunkManager {
public:
    ChunkManager(uint32_t node_id);
    void initFromFileSize(uint64_t size, const std::string& url);
    void assignChunksRoundRobin();
    std::vector<uint32_t> getMyChunks() const;
    void markChunkDownloaded(uint32_t chunk_id, const std::string& hash);
    bool hasChunk(uint32_t chunk_id) const;
    const Chunk* getChunk(uint32_t chunk_id) const;
    void addPeer(const Peer& peer);
    void removePeer(uint32_t peer_id);
};
```

### PeerDiscovery

```cpp
class PeerDiscovery {
public:
    PeerDiscovery(uint32_t node_id, uint16_t port);
    bool start();
    void stop();
    bool sendHello(const std::string& address, uint16_t port);
    bool sendHeartbeat(const std::string& address, uint16_t port);
    bool sendLeave(const std::string& address, uint16_t port);
    void setPeerDiscoveredCallback(PeerDiscoveredCallback callback);
    void setPeerLeftCallback(PeerLeftCallback callback);
};
```

### ChunkProtocol

```cpp
class ChunkProtocol {
public:
    ChunkProtocol(uint32_t node_id);
    bool sendChunkHave(const std::string& address, uint16_t port, uint32_t chunk_id, const std::string& hash);
    bool sendChunkRequest(const std::string& address, uint16_t port, uint32_t chunk_id);
    bool sendChunkData(const std::string& address, uint16_t port, uint32_t chunk_id, const std::vector<uint8_t>& data);
};
```

## Protocol Messages

### Discovery Messages

**HELLO:**
- Sent by peer when joining cluster
- Format: `[type(1)][node_id(4)][port(2)]`

**HEARTBEAT:**
- Sent periodically to maintain connection
- Format: `[type(1)][node_id(4)][port(2)]`

**LEAVE:**
- Sent by peer when leaving cluster
- Format: `[type(1)][node_id(4)]`

### Chunk Exchange Messages

**CHUNK_HAVE:**
- Sent by peer after downloading a chunk
- Format: `[type(1)][node_id(4)][chunk_id(4)][hash_len(4)][hash(64)]`

**CHUNK_REQUEST:**
- Sent by peer to request a chunk from another peer
- Format: `[type(1)][node_id(4)][chunk_id(4)]`

**CHUNK_DATA:**
- Sent by peer in response to CHUNK_REQUEST
- Format: `[type(1)][node_id(4)][chunk_id(4)][data_len(4)][data]`

## Example Workflow

### Download a 10MB file with 3 peers

```bash
# Terminal 1: Start coordinator
./coordinator --port=5000 --url=https://example.com/file.bin --size=10485760

# Terminal 2: Start peer 1
./peer_node --node_id=1 --port=6001 --coordinator=127.0.0.1:5000 --url=https://example.com/file.bin --size=10485760 --out=file1.bin

# Terminal 3: Start peer 2
./peer_node --node_id=2 --port=6002 --coordinator=127.0.0.1:5000 --url=https://example.com/file.bin --size=10485760 --out=file2.bin

# Terminal 4: Start peer 3
./peer_node --node_id=3 --port=6003 --coordinator=127.0.0.1:5000 --url=https://example.com/file.bin --size=10485760 --out=file3.bin
```

**Chunk Assignment (10MB = 10 chunks):**
- Peer 1: chunks 0, 3, 6, 9
- Peer 2: chunks 1, 4, 7
- Peer 3: chunks 2, 5, 8

Each peer downloads ~3.3MB from internet, then broadcasts to others.

## Limitations (MVP)

- Coordinator is single point of failure (no coordinator failover)
- No chunk exchange between peers (only broadcast to coordinator)
- No retry mechanism for failed downloads
- No progress persistence across restarts
- Limited to localhost TCP (no Wi-Fi Direct yet)

## Next Steps

- Implement peer-to-peer chunk exchange
- Add coordinator failover
- Implement retry and resume
- Add progress persistence
- Port to Wi-Fi Direct for mobile
