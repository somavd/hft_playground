#pragma once

#include <string>
#include <functional>
#include <vector>
#include <cstdint>

enum class MsgType : uint8_t {
    HELLO            = 1,
    LEAVE            = 2,
    CHUNK_ASSIGNMENT = 3,
    CHUNK_REQUEST    = 4,
    CHUNK_COMPLETE   = 5,
    CHUNK_HAVE       = 6,
    CHUNK_DATA       = 7,
    CHUNK_DATA_REQUEST = 8
};

using PeerJoinCallback       = std::function<void(uint32_t node_id, const std::string& ip, uint16_t port)>;
using PeerLeaveCallback      = std::function<void(uint32_t node_id)>;
using ChunkAssignCallback    = std::function<void(const std::vector<uint32_t>& chunk_ids)>;
using ChunkRequestCallback   = std::function<void(uint32_t chunk_id, uint8_t source_type, const std::string& source_ip, uint16_t source_port)>;
using ChunkCompleteCallback  = std::function<void(uint32_t node_id, uint32_t chunk_id)>;
using ChunkHaveCallback      = std::function<void(uint32_t node_id, const std::vector<uint32_t>& chunk_ids)>;
using ChunkDataCallback      = std::function<void(uint32_t chunk_id, const std::vector<uint8_t>& data)>;
using ChunkRequestFromPeerCallback = std::function<void(uint32_t chunk_id, const std::string& requester_ip, uint16_t requester_port)>;

class PeerDiscovery {
private:
    uint32_t my_node_id;
    uint16_t listen_port;
    int server_fd;
    bool running;

    PeerJoinCallback    on_join;
    PeerLeaveCallback   on_leave;
    ChunkAssignCallback on_assign;
    ChunkRequestCallback on_request;
    ChunkCompleteCallback on_complete;
    ChunkHaveCallback   on_have;
    ChunkDataCallback   on_data;
    ChunkRequestFromPeerCallback on_request_from_peer;

public:
    PeerDiscovery(uint32_t node_id, uint16_t port);
    ~PeerDiscovery();

    bool start();
    void stop();

    bool sendHello(const std::string& host, uint16_t port);
    bool sendLeave(const std::string& host, uint16_t port);
    bool sendChunkAssignment(const std::string& host, uint16_t port,
                             const std::vector<uint32_t>& chunk_ids);
    bool sendChunkRequest(const std::string& host, uint16_t port, uint32_t chunk_id,
                         uint8_t source_type = 0, const std::string& source_ip = "", uint16_t source_port = 0);
    bool sendChunkComplete(const std::string& host, uint16_t port, uint32_t chunk_id);
    bool sendChunkHave(const std::string& host, uint16_t port, const std::vector<uint32_t>& chunk_ids);
    bool sendChunkData(const std::string& host, uint16_t port, uint32_t chunk_id, const std::vector<uint8_t>& data);
    bool sendChunkDataRequest(const std::string& host, uint16_t port, uint32_t chunk_id);

    void onPeerJoin(PeerJoinCallback cb);
    void onPeerLeave(PeerLeaveCallback cb);
    void onChunkAssign(ChunkAssignCallback cb);
    void onChunkRequest(ChunkRequestCallback cb);
    void onChunkComplete(ChunkCompleteCallback cb);
    void onChunkHave(ChunkHaveCallback cb);
    void onChunkData(ChunkDataCallback cb);
    void onChunkRequestFromPeer(ChunkRequestFromPeerCallback cb);

    uint32_t nodeId() const;
    uint16_t port() const;
};
