#include "chunk_manager.h"
#include <iostream>

ChunkManager::ChunkManager(uint32_t node_id) : my_node_id(node_id), file_size(0) {}

void ChunkManager::initFromFileSize(uint64_t size, const std::string& url) {
    file_size = size;
    file_url = url;
    chunks.clear();
    
    uint32_t chunk_id = 0;
    for (uint64_t offset = 0; offset < size; offset += CHUNK_SIZE) {
        Chunk chunk;
        chunk.id = chunk_id++;
        chunk.start_byte = offset;
        chunk.end_byte = std::min(offset + CHUNK_SIZE - 1, size - 1);
        chunk.downloaded = false;
        chunks.push_back(chunk);
    }
    
    std::cout << "[CM] " << chunks.size() << " chunks (" << size << " bytes)\n";
}

void ChunkManager::assignChunksRoundRobin() {
    uint32_t num_peers = peers.size();
    if (num_peers == 0) {
        std::cerr << "[CM] no peers to assign\n";
        return;
    }
    
    // Clear all assignments
    for (auto& peer : peers) {
        peer.second.chunk_bitmap.reset();
    }
    
    // Assign chunks round-robin
    for (size_t i = 0; i < chunks.size(); ++i) {
        uint32_t peer_id = i % num_peers;
        auto it = peers.begin();
        std::advance(it, peer_id);
        it->second.chunk_bitmap.set(i);
    }
    
}

std::vector<uint32_t> ChunkManager::getMyChunks() const {
    std::vector<uint32_t> my_chunks;
    auto it = peers.find(my_node_id);
    if (it != peers.end()) {
        for (size_t i = 0; i < chunks.size(); ++i) {
            if (it->second.chunk_bitmap.test(i)) {
                my_chunks.push_back(i);
            }
        }
    }
    return my_chunks;
}

void ChunkManager::markChunkDownloaded(uint32_t chunk_id, const std::string& hash) {
    if (chunk_id >= chunks.size()) return;
    
    chunks[chunk_id].downloaded = true;
    chunks[chunk_id].hash = hash;
    
    // Update my bitmap
    auto it = peers.find(my_node_id);
    if (it != peers.end()) {
        it->second.chunk_bitmap.set(chunk_id);
    }
}

bool ChunkManager::hasChunk(uint32_t chunk_id) const {
    if (chunk_id >= chunks.size()) {
        return false;
    }
    return chunks[chunk_id].downloaded;
}

const Chunk* ChunkManager::getChunk(uint32_t chunk_id) const {
    if (chunk_id >= chunks.size()) {
        return nullptr;
    }
    return &chunks[chunk_id];
}

size_t ChunkManager::getChunkCount() const {
    return chunks.size();
}

void ChunkManager::addPeer(const Peer& peer) {
    peers[peer.id] = peer;
}

void ChunkManager::removePeer(uint32_t peer_id) {
    peers.erase(peer_id);
}

const std::map<uint32_t, Peer>& ChunkManager::getPeers() const {
    return peers;
}

void ChunkManager::updatePeerChunkBitmap(uint32_t peer_id, uint32_t chunk_id, bool has_chunk) {
    auto it = peers.find(peer_id);
    if (it != peers.end()) {
        it->second.chunk_bitmap.set(chunk_id, has_chunk);
    }
}

uint32_t ChunkManager::getMyNodeId() const {
    return my_node_id;
}

std::string ChunkManager::getFileUrl() const {
    return file_url;
}

uint64_t ChunkManager::getFileSize() const {
    return file_size;
}
