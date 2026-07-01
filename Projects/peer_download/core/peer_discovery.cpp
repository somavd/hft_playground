#include "peer_discovery.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <cstring>
#include <iostream>

static int tcpConnect(const std::string& host, uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
    if (connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static bool recvAll(int fd, char* buf, size_t len) {
    size_t received = 0;
    while (received < len) {
        ssize_t n = recv(fd, buf + received, len - received, 0);
        if (n <= 0) return false;
        received += n;
    }
    return true;
}

static bool sendAll(int fd, const char* buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, buf + sent, len - sent, 0);
        if (n <= 0) return false;
        sent += n;
    }
    return true;
}

PeerDiscovery::PeerDiscovery(uint32_t node_id, uint16_t p)
    : my_node_id(node_id), listen_port(p), server_fd(-1), running(false) {}

PeerDiscovery::~PeerDiscovery() { stop(); }

bool PeerDiscovery::start() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) return false;

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(listen_port);

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[ERR] bind failed on port " << listen_port << std::endl;
        close(server_fd);
        return false;
    }
    if (listen(server_fd, 64) < 0) {
        close(server_fd);
        return false;
    }

    running = true;

    std::thread([this]() {
        while (running) {
            sockaddr_in cli{};
            socklen_t len = sizeof(cli);
            int cfd = accept(server_fd, (sockaddr*)&cli, &len);
            if (cfd < 0) continue;

            std::thread([this, cfd, cli]() {
                char hdr[1];
                if (!recvAll(cfd, hdr, 1)) { close(cfd); return; }

                auto type = static_cast<MsgType>(hdr[0]);
                switch (type) {
                    case MsgType::HELLO: {
                        char buf[6];
                        if (!recvAll(cfd, buf, 6)) break;
                        uint32_t nid = ntohl(*(uint32_t*)(buf));
                        uint16_t pt  = ntohs(*(uint16_t*)(buf + 4));
                        std::string ip = inet_ntoa(cli.sin_addr);
                        if (on_join) on_join(nid, ip, pt);
                        break;
                    }
                    case MsgType::LEAVE: {
                        char buf[4];
                        if (!recvAll(cfd, buf, 4)) break;
                        uint32_t nid = ntohl(*(uint32_t*)(buf));
                        if (on_leave) on_leave(nid);
                        break;
                    }
                    case MsgType::CHUNK_ASSIGNMENT: {
                        char buf[4];
                        if (!recvAll(cfd, buf, 4)) break;
                        uint32_t count = ntohl(*(uint32_t*)(buf));
                        std::vector<uint32_t> ids(count);
                        if (count > 0) {
                            std::vector<char> data(count * 4);
                            if (!recvAll(cfd, data.data(), data.size())) break;
                            for (uint32_t i = 0; i < count; ++i)
                                ids[i] = ntohl(*(uint32_t*)(data.data() + i*4));
                        }
                        if (on_assign) on_assign(ids);
                        break;
                    }
                    case MsgType::CHUNK_REQUEST: {
                        char buf[11];
                        if (!recvAll(cfd, buf, 11)) break;
                        uint32_t cid = ntohl(*(uint32_t*)(buf));
                        uint8_t source_type = buf[4];
                        std::string source_ip;
                        uint16_t source_port = 0;
                        if (source_type == 1) {
                            struct in_addr a;
                            a.s_addr = *(uint32_t*)(buf + 5);
                            source_ip = inet_ntoa(a);
                            source_port = ntohs(*(uint16_t*)(buf + 9));
                        }
                        if (on_request) on_request(cid, source_type, source_ip, source_port);
                        break;
                    }
                    case MsgType::CHUNK_COMPLETE: {
                        char buf[8];
                        if (!recvAll(cfd, buf, 8)) break;
                        uint32_t nid = ntohl(*(uint32_t*)(buf));
                        uint32_t cid = ntohl(*(uint32_t*)(buf + 4));
                        if (on_complete) on_complete(nid, cid);
                        break;
                    }
                    case MsgType::CHUNK_HAVE: {
                        char buf[8];
                        if (!recvAll(cfd, buf, 8)) break;
                        uint32_t nid = ntohl(*(uint32_t*)(buf));
                        uint32_t count = ntohl(*(uint32_t*)(buf + 4));
                        std::vector<uint32_t> ids(count);
                        if (count > 0) {
                            std::vector<char> data(count * 4);
                            if (!recvAll(cfd, data.data(), data.size())) break;
                            for (uint32_t i = 0; i < count; ++i)
                                ids[i] = ntohl(*(uint32_t*)(data.data() + i*4));
                        }
                        if (on_have) on_have(nid, ids);
                        break;
                    }
                    case MsgType::CHUNK_DATA: {
                        char buf[8];
                        if (!recvAll(cfd, buf, 8)) break;
                        uint32_t cid = ntohl(*(uint32_t*)(buf));
                        uint32_t data_len = ntohl(*(uint32_t*)(buf + 4));
                        std::vector<uint8_t> data(data_len);
                        if (data_len > 0) {
                            if (!recvAll(cfd, (char*)data.data(), data_len)) break;
                        }
                        if (on_data) on_data(cid, data);
                        break;
                    }
                    case MsgType::CHUNK_DATA_REQUEST: {
                        char buf[6];
                        if (!recvAll(cfd, buf, 6)) break;
                        uint32_t cid = ntohl(*(uint32_t*)(buf));
                        uint16_t req_port = ntohs(*(uint16_t*)(buf + 4));
                        std::string requester_ip = inet_ntoa(cli.sin_addr);
                        if (on_request_from_peer) on_request_from_peer(cid, requester_ip, req_port);
                        break;
                    }
                }
                close(cfd);
            }).detach();
        }
    }).detach();

    return true;
}

void PeerDiscovery::stop() {
    running = false;
    if (server_fd >= 0) { close(server_fd); server_fd = -1; }
}

bool PeerDiscovery::sendHello(const std::string& host, uint16_t p) {
    int fd = tcpConnect(host, p);
    if (fd < 0) return false;
    char buf[7];
    buf[0] = (char)MsgType::HELLO;
    *(uint32_t*)(buf+1) = htonl(my_node_id);
    *(uint16_t*)(buf+5) = htons(listen_port);
    sendAll(fd, buf, 7);
    close(fd);
    return true;
}

bool PeerDiscovery::sendLeave(const std::string& host, uint16_t p) {
    int fd = tcpConnect(host, p);
    if (fd < 0) return false;
    char buf[5];
    buf[0] = (char)MsgType::LEAVE;
    *(uint32_t*)(buf+1) = htonl(my_node_id);
    sendAll(fd, buf, 5);
    close(fd);
    return true;
}

bool PeerDiscovery::sendChunkAssignment(const std::string& host, uint16_t p,
                                         const std::vector<uint32_t>& chunk_ids) {
    int fd = tcpConnect(host, p);
    if (fd < 0) return false;
    std::vector<char> buf(5 + chunk_ids.size() * 4);
    buf[0] = (char)MsgType::CHUNK_ASSIGNMENT;
    *(uint32_t*)(buf.data()+1) = htonl((uint32_t)chunk_ids.size());
    for (size_t i = 0; i < chunk_ids.size(); ++i)
        *(uint32_t*)(buf.data()+5+i*4) = htonl(chunk_ids[i]);
    sendAll(fd, buf.data(), buf.size());
    close(fd);
    return true;
}

bool PeerDiscovery::sendChunkRequest(const std::string& host, uint16_t p, uint32_t chunk_id,
                                       uint8_t source_type, const std::string& source_ip, uint16_t source_port) {
    int fd = tcpConnect(host, p);
    if (fd < 0) return false;
    // Format: [type(1)][chunk_id(4)][source_type(1)][source_ip(4)][source_port(2)] = 12 bytes
    char buf[12];
    buf[0] = (char)MsgType::CHUNK_REQUEST;
    *(uint32_t*)(buf+1) = htonl(chunk_id);
    buf[5] = source_type;
    uint32_t ip_addr = (source_type == 1) ? inet_addr(source_ip.c_str()) : 0;
    *(uint32_t*)(buf+6) = ip_addr;
    *(uint16_t*)(buf+10) = htons(source_port);
    sendAll(fd, buf, 12);
    close(fd);
    return true;
}

bool PeerDiscovery::sendChunkComplete(const std::string& host, uint16_t p, uint32_t chunk_id) {
    int fd = tcpConnect(host, p);
    if (fd < 0) return false;
    char buf[9];
    buf[0] = (char)MsgType::CHUNK_COMPLETE;
    *(uint32_t*)(buf+1) = htonl(my_node_id);
    *(uint32_t*)(buf+5) = htonl(chunk_id);
    sendAll(fd, buf, 9);
    close(fd);
    return true;
}

bool PeerDiscovery::sendChunkHave(const std::string& host, uint16_t p, const std::vector<uint32_t>& chunk_ids) {
    int fd = tcpConnect(host, p);
    if (fd < 0) return false;
    std::vector<char> buf(9 + chunk_ids.size() * 4);
    buf[0] = (char)MsgType::CHUNK_HAVE;
    *(uint32_t*)(buf.data()+1) = htonl(my_node_id);
    *(uint32_t*)(buf.data()+5) = htonl((uint32_t)chunk_ids.size());
    for (size_t i = 0; i < chunk_ids.size(); ++i)
        *(uint32_t*)(buf.data()+9+i*4) = htonl(chunk_ids[i]);
    sendAll(fd, buf.data(), buf.size());
    close(fd);
    return true;
}

bool PeerDiscovery::sendChunkData(const std::string& host, uint16_t p, uint32_t chunk_id, const std::vector<uint8_t>& data) {
    int fd = tcpConnect(host, p);
    if (fd < 0) return false;
    // Header: [type(1)][chunk_id(4)][data_len(4)] = 9 bytes
    char hdr[9];
    hdr[0] = (char)MsgType::CHUNK_DATA;
    *(uint32_t*)(hdr+1) = htonl(chunk_id);
    *(uint32_t*)(hdr+5) = htonl((uint32_t)data.size());
    if (!sendAll(fd, hdr, 9) || !sendAll(fd, (const char*)data.data(), data.size())) {
        close(fd);
        return false;
    }
    close(fd);
    return true;
}

bool PeerDiscovery::sendChunkDataRequest(const std::string& host, uint16_t p, uint32_t chunk_id) {
    int fd = tcpConnect(host, p);
    if (fd < 0) return false;
    // Format: [type(1)][chunk_id(4)][listen_port(2)] = 7 bytes
    char buf[7];
    buf[0] = (char)MsgType::CHUNK_DATA_REQUEST;
    *(uint32_t*)(buf+1) = htonl(chunk_id);
    *(uint16_t*)(buf+5) = htons(listen_port);
    sendAll(fd, buf, 7);
    close(fd);
    return true;
}

void PeerDiscovery::onPeerJoin(PeerJoinCallback cb)    { on_join = cb; }
void PeerDiscovery::onPeerLeave(PeerLeaveCallback cb)  { on_leave = cb; }
void PeerDiscovery::onChunkAssign(ChunkAssignCallback cb) { on_assign = cb; }
void PeerDiscovery::onChunkRequest(ChunkRequestCallback cb) { on_request = cb; }
void PeerDiscovery::onChunkComplete(ChunkCompleteCallback cb) { on_complete = cb; }
void PeerDiscovery::onChunkHave(ChunkHaveCallback cb)     { on_have = cb; }
void PeerDiscovery::onChunkData(ChunkDataCallback cb)     { on_data = cb; }
void PeerDiscovery::onChunkRequestFromPeer(ChunkRequestFromPeerCallback cb) { on_request_from_peer = cb; }

uint32_t PeerDiscovery::nodeId() const { return my_node_id; }
uint16_t PeerDiscovery::port() const   { return listen_port; }
