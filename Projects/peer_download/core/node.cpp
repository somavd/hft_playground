#include "node.h"
#include "peer_discovery.h"
#include "http_range_fetch.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>

#define LOG(msg) std::cout << "[NODE " << config_.node_id << "] " << msg << "\n"

Node::Node(const Config& config) : config_(config) {
    initChunks();
}

Node::~Node() { stop(); }

void Node::initChunks() {
    uint32_t n = (config_.file_size + CHUNK_SIZE - 1) / CHUNK_SIZE;
    chunks_.resize(n);
    for (uint32_t i = 0; i < n; ++i) {
        chunks_[i].id = i;
        chunks_[i].start_byte = (uint64_t)i * CHUNK_SIZE;
        chunks_[i].end_byte = std::min(chunks_[i].start_byte + CHUNK_SIZE - 1, config_.file_size - 1);
    }
}

void Node::start() {
    running_.store(true);
    mkdir(config_.work_dir.c_str(), 0755);

    net_ = std::make_unique<PeerDiscovery>(config_.node_id, config_.listen_port);

    // --- Peer join: coordinator tracks peer but does not assign HTTP work ---
    net_->onPeerJoin([this](uint32_t id, const std::string& ip, uint16_t port) {
        {
            std::lock_guard<std::mutex> lock(mu_);
            peers_[id] = {id, ip, port, {}};
        }
        LOG("peer " << id << " joined (" << ip << ":" << port << ")");
    });

    net_->onPeerLeave([this](uint32_t id) {
        std::lock_guard<std::mutex> lock(mu_);
        peers_.erase(id);
        LOG("peer " << id << " left");
    });

    // --- Chunk request from coordinator (peers receive this) ---
    net_->onChunkRequest([this](uint32_t chunk_id, uint8_t source_type,
                                const std::string& source_ip, uint16_t source_port) {
        if (source_type == 1) {
            LOG("requesting chunk " << chunk_id << " from peer " << source_ip << ":" << source_port);
            net_->sendChunkDataRequest(source_ip, source_port, chunk_id);
        } else {
            pushHttpTask(chunk_id);
        }
    });

    // --- Chunk complete from remote peer (coordinator receives this) ---
    net_->onChunkComplete([this](uint32_t node_id, uint32_t chunk_id) {
        if (!config_.is_coordinator) return;
        {
            std::lock_guard<std::mutex> lock(mu_);
            if (peers_.count(node_id)) {
                peers_[node_id].has_chunks.insert(chunk_id);
            }
        }
        LOG("node " << node_id << " completed chunk " << chunk_id);
    });

    // --- Chunk have from peer ---
    net_->onChunkHave([this](uint32_t node_id, const std::vector<uint32_t>& chunk_ids) {
        std::lock_guard<std::mutex> lock(mu_);
        if (node_id == 0 && !config_.is_coordinator) {
            // Coordinator announced chunks
            for (uint32_t cid : chunk_ids) {
                coordinator_has_chunks_.insert(cid);
            }
        } else if (peers_.count(node_id)) {
            // Peer announced chunks
            for (uint32_t cid : chunk_ids) {
                peers_[node_id].has_chunks.insert(cid);
            }
        }
    });

    // --- Serve chunk data to requesting peer ---
    net_->onChunkRequestFromPeer([this](uint32_t chunk_id, const std::string& req_ip, uint16_t req_port) {
        std::string path = config_.work_dir + "/chunk_" + std::to_string(chunk_id) + ".bin";
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) return;
        file.seekg(0, std::ios::end);
        size_t sz = file.tellg();
        file.seekg(0);
        std::vector<uint8_t> data(sz);
        file.read((char*)data.data(), sz);
        file.close();
        net_->sendChunkData(req_ip, req_port, chunk_id, data);
        LOG("served chunk " << chunk_id << " to " << req_ip << ":" << req_port);
    });

    // --- Receive chunk data from peer ---
    net_->onChunkData([this](uint32_t chunk_id, const std::vector<uint8_t>& data) {
        // Coordinator should not accept P2P chunks - only download via HTTP
        if (config_.is_coordinator) {
            LOG("rejecting P2P chunk " << chunk_id << " (coordinator only downloads via HTTP)");
            return;
        }

        std::string path = config_.work_dir + "/chunk_" + std::to_string(chunk_id) + ".bin";
        {
            std::ofstream file(path, std::ios::binary);
            file.write((const char*)data.data(), data.size());
        }
        {
            std::lock_guard<std::mutex> lock(mu_);
            my_chunks_.insert(chunk_id);
            chunks_[chunk_id].status = ChunkStatus::COMPLETED;
        }
        done_cv_.notify_all();
        std::vector<uint32_t> have;
        {
            std::lock_guard<std::mutex> lock(mu_);
            have.assign(my_chunks_.begin(), my_chunks_.end());
        }
        net_->sendChunkHave(config_.coordinator_host, config_.coordinator_port, have);
        LOG("chunk " << chunk_id << " from peer (" << my_chunks_.size() << "/" << chunks_.size() << ")");
    });

    if (!net_->start()) {
        std::cerr << "[ERR] bind failed on port " << config_.listen_port << "\n";
        return;
    }

    // Start HTTP worker thread
    http_worker_ = std::thread(&Node::httpWorkerFunc, this);

    // Start peer exchange thread
    peer_exchange_ = std::thread(&Node::peerExchangeFunc, this);

    if (!config_.is_coordinator) {
        net_->sendHello(config_.coordinator_host, config_.coordinator_port);
        LOG("sent HELLO to coordinator");
    } else {
        // Coordinator starts downloading immediately
        assignNextTo(config_.node_id);
    }

    LOG("started on port " << config_.listen_port << "  chunks=" << chunks_.size());
}

void Node::stop() {
    if (!running_.load()) return;
    running_.store(false);
    http_cv_.notify_all();
    done_cv_.notify_all();

    if (http_worker_.joinable()) http_worker_.join();
    if (peer_exchange_.joinable()) peer_exchange_.join();

    if (!config_.is_coordinator && net_) {
        net_->sendLeave(config_.coordinator_host, config_.coordinator_port);
    }
    if (net_) net_->stop();
    LOG("stopped");
}

bool Node::isComplete() const {
    std::lock_guard<std::mutex> lock(mu_);
    return my_chunks_.size() >= chunks_.size();
}

uint32_t Node::chunksDownloaded() const {
    std::lock_guard<std::mutex> lock(mu_);
    return my_chunks_.size();
}

uint32_t Node::totalChunks() const {
    return chunks_.size();
}

void Node::pushHttpTask(uint32_t chunk_id) {
    {
        std::lock_guard<std::mutex> lock(mu_);
        http_queue_.push(chunk_id);
    }
    http_cv_.notify_one();
}

void Node::assignNextTo(uint32_t worker_id) {
    uint32_t cid = next_chunk_.fetch_add(1);
    if (cid >= chunks_.size()) return;

    {
        std::lock_guard<std::mutex> lock(mu_);
        chunks_[cid].status = ChunkStatus::ASSIGNED;
        chunks_[cid].assigned_to = worker_id;
    }

    if (worker_id == config_.node_id) {
        pushHttpTask(cid);
        LOG("downloading chunk " << cid << " (self)");
    } else {
        std::string ip;
        uint16_t port;
        {
            std::lock_guard<std::mutex> lock(mu_);
            auto it = peers_.find(worker_id);
            if (it == peers_.end()) return;
            ip = it->second.ip;
            port = it->second.port;
        }
        net_->sendChunkRequest(ip, port, cid, 0, "", 0);
        LOG("assigned chunk " << cid << " to node " << worker_id);
    }
}

// HTTP worker thread
void Node::httpWorkerFunc() {
    while (running_.load()) {
        uint32_t chunk_id;
        {
            std::unique_lock<std::mutex> lock(mu_);
            http_cv_.wait(lock, [&]{ return !http_queue_.empty() || !running_.load(); });
            if (!running_.load() && http_queue_.empty()) return;
            chunk_id = http_queue_.front();
            http_queue_.pop();
        }

        uint64_t start_byte = chunks_[chunk_id].start_byte;
        uint64_t end_byte = chunks_[chunk_id].end_byte;
        std::string path = config_.work_dir + "/chunk_" + std::to_string(chunk_id) + ".bin";

        bool success = false;
        for (uint32_t retry = 0; retry < MAX_RETRIES; ++retry) {
            if (HttpRangeFetch::fetchRange(config_.file_url, start_byte, end_byte, path)) {
                success = true;
                break;
            }
            LOG("HTTP chunk " << chunk_id << " retry " << (retry + 1));
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        if (!success) {
            LOG("FAIL HTTP chunk " << chunk_id);
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(mu_);
            my_chunks_.insert(chunk_id);
            chunks_[chunk_id].status = ChunkStatus::COMPLETED;
        }
        done_cv_.notify_all();
        LOG("chunk " << chunk_id << " from HTTP (" << my_chunks_.size() << "/" << chunks_.size() << ")");

        if (config_.is_coordinator) {
            // Coordinator broadcasts CHUNK_HAVE to all peers
            std::vector<uint32_t> have;
            {
                std::lock_guard<std::mutex> lock(mu_);
                have.assign(my_chunks_.begin(), my_chunks_.end());
            }
            for (auto& [pid, peer] : peers_) {
                net_->sendChunkHave(peer.ip, peer.port, have);
            }
            // Coordinator always assigns to self (HTTP download)
            assignNextTo(config_.node_id);
        } else {
            // Peer: notify coordinator
            net_->sendChunkComplete(config_.coordinator_host, config_.coordinator_port, chunk_id);
            std::vector<uint32_t> have;
            {
                std::lock_guard<std::mutex> lock(mu_);
                have.assign(my_chunks_.begin(), my_chunks_.end());
            }
            net_->sendChunkHave(config_.coordinator_host, config_.coordinator_port, have);
        }
    }
}

// Peer exchange thread: requests missing chunks from known peers
void Node::peerExchangeFunc() {
    // Coordinator only downloads via HTTP, no peer exchange needed
    if (config_.is_coordinator) {
        while (running_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(PEER_EXCHANGE_INTERVAL_MS));
        }
        return;
    }

    while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(PEER_EXCHANGE_INTERVAL_MS));

        std::vector<uint32_t> missing;
        std::set<uint32_t> my_chunks_copy;
        std::set<uint32_t> coordinator_has_copy;
        {
            std::lock_guard<std::mutex> lock(mu_);
            for (uint32_t i = 0; i < chunks_.size(); ++i) {
                if (my_chunks_.count(i) == 0) missing.push_back(i);
            }
            my_chunks_copy = my_chunks_;
            coordinator_has_copy = coordinator_has_chunks_;
        }

        if (missing.empty()) continue;

        // For each missing chunk, request from coordinator if coordinator has it
        for (uint32_t cid : missing) {
            if (my_chunks_copy.count(cid)) continue;
            if (coordinator_has_copy.count(cid)) {
                net_->sendChunkDataRequest(config_.coordinator_host, config_.coordinator_port, cid);
            }
        }
    }
}
