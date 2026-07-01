#pragma once

#include <vector>
#include <map>
#include <string>
#include <cstdint>
#include <bitset>

constexpr uint64_t CHUNK_SIZE = 1024 * 1024; // 1MB
constexpr uint32_t MAX_CHUNKS = 10000; // Maximum chunks supported

struct Chunk {
    uint32_t id;
    uint64_t start_byte;
    uint64_t end_byte;
    std::string hash;
    bool downloaded;
};

struct Peer {
    uint32_t id;
    std::string address;
    int socket_fd;
    std::bitset<MAX_CHUNKS> chunk_bitmap;
};

class ChunkManager {
private:
    std::vector<Chunk> chunks;
    std::map<uint32_t, Peer> peers;
    uint32_t my_node_id;
    std::string file_url;
    uint64_t file_size;

public:
    ChunkManager(uint32_t node_id);
    
    // Initialize chunk list from file size
    void initFromFileSize(uint64_t size, const std::string& url);
    
    // Assign chunks to peers using round-robin
    void assignChunksRoundRobin();
    
    // Get chunks assigned to this node
    std::vector<uint32_t> getMyChunks() const;
    
    // Mark a chunk as downloaded
    void markChunkDownloaded(uint32_t chunk_id, const std::string& hash);
    
    // Check if this node has a specific chunk
    bool hasChunk(uint32_t chunk_id) const;
    
    // Get chunk info by ID
    const Chunk* getChunk(uint32_t chunk_id) const;
    
    // Get total number of chunks
    size_t getChunkCount() const;
    
    // Add a peer to the cluster
    void addPeer(const Peer& peer);
    
    // Remove a peer from the cluster
    void removePeer(uint32_t peer_id);
    
    // Get all peers
    const std::map<uint32_t, Peer>& getPeers() const;
    
    // Update peer's chunk bitmap
    void updatePeerChunkBitmap(uint32_t peer_id, uint32_t chunk_id, bool has_chunk);
    
    // Get my node ID
    uint32_t getMyNodeId() const;
    
    // Get file URL
    std::string getFileUrl() const;
    
    // Get file size
    uint64_t getFileSize() const;
};
