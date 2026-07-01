#pragma once

#include <string>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <memory>

class PeerDiscovery;

constexpr uint64_t CHUNK_SIZE = 1048576;  // 1 MB
constexpr uint32_t MAX_RETRIES = 3;
constexpr uint32_t PEER_EXCHANGE_INTERVAL_MS = 500;

enum class ChunkStatus { PENDING, ASSIGNED, COMPLETED };

struct ChunkState {
    uint32_t    id;
    uint64_t    start_byte;
    uint64_t    end_byte;
    ChunkStatus status      = ChunkStatus::PENDING;
    uint32_t    assigned_to = 0;
};

struct PeerInfo {
    uint32_t    node_id;
    std::string ip;
    uint16_t    port;
    std::set<uint32_t> has_chunks;
};

struct Config {
    uint32_t    node_id;
    uint16_t    listen_port;
    std::string file_url;
    uint64_t    file_size;
    std::string output_path;
    std::string work_dir;
    bool        is_coordinator;
    std::string coordinator_host;
    uint16_t    coordinator_port;
};

class Node {
public:
    Node(const Config& config);
    ~Node();

    void start();
    void stop();

    bool isComplete() const;
    uint32_t chunksDownloaded() const;
    uint32_t totalChunks() const;

private:
    Config config_;
    std::unique_ptr<PeerDiscovery> net_;
    std::atomic<bool> running_{false};

    // Chunk state
    mutable std::mutex mu_;
    std::vector<ChunkState> chunks_;
    std::set<uint32_t> my_chunks_;
    std::map<uint32_t, PeerInfo> peers_;
    std::set<uint32_t> coordinator_has_chunks_;  // chunks coordinator announced via CHUNK_HAVE
    std::condition_variable done_cv_;

    // HTTP download queue
    std::queue<uint32_t> http_queue_;
    std::condition_variable http_cv_;

    // Coordinator assignment counter
    std::atomic<uint32_t> next_chunk_{0};

    // Threads (PeerDiscovery runs its own network thread internally)
    std::thread http_worker_;
    std::thread peer_exchange_;

    void initChunks();
    void pushHttpTask(uint32_t chunk_id);

    // Coordinator: assign next available chunk to a worker
    void assignNextTo(uint32_t worker_id);

    // Thread functions
    void httpWorkerFunc();
    void peerExchangeFunc();
};
