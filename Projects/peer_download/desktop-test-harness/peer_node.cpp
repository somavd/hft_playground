#include "../core/node.h"
#include "../core/hasher.h"
#include "../core/reassembler.h"
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    uint32_t node_id = 0;
    uint16_t listen_port = 0;
    std::string coord_str, file_url, output_path;
    uint64_t file_size = 0;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if      (a.rfind("--node_id=", 0) == 0)     node_id     = std::atoi(a.substr(10).c_str());
        else if (a.rfind("--port=", 0) == 0)         listen_port = std::atoi(a.substr(7).c_str());
        else if (a.rfind("--coordinator=", 0) == 0)  coord_str   = a.substr(14);
        else if (a.rfind("--url=", 0) == 0)          file_url    = a.substr(6);
        else if (a.rfind("--size=", 0) == 0)         file_size   = std::atoll(a.substr(7).c_str());
        else if (a.rfind("--out=", 0) == 0)          output_path = a.substr(6);
    }

    if (node_id == 0 || listen_port == 0 || coord_str.empty() ||
        file_url.empty() || file_size == 0 || output_path.empty()) {
        std::cerr << "Usage: peer_node --node_id=ID --port=PORT --coordinator=HOST:PORT"
                  << " --url=URL --size=BYTES --out=FILE\n";
        return 1;
    }

    size_t cp = coord_str.find(':');
    std::string coord_host = coord_str.substr(0, cp);
    uint16_t    coord_port = std::atoi(coord_str.substr(cp + 1).c_str());

    Config config;
    config.node_id = node_id;
    config.listen_port = listen_port;
    config.file_url = file_url;
    config.file_size = file_size;
    config.output_path = output_path;
    config.work_dir = "node_" + std::to_string(node_id);
    config.is_coordinator = false;
    config.coordinator_host = coord_host;
    config.coordinator_port = coord_port;

    std::cout << "[NODE " << node_id << "] port=" << listen_port
              << "  chunks=" << ((file_size + CHUNK_SIZE - 1) / CHUNK_SIZE)
              << "  coordinator=" << coord_host << ":" << coord_port << "\n";

    Node node(config);
    node.start();

    // Wait for all chunks to be downloaded
    while (!node.isComplete()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "[NODE " << node_id << "] Progress: " << node.chunksDownloaded() << "/" << node.totalChunks() << "\n";
    }

    std::cout << "[NODE " << node_id << "] All chunks downloaded. Reassembling...\n";

    // Reassemble
    std::vector<ChunkInfo> infos;
    for (uint32_t i = 0; i < node.totalChunks(); ++i) {
        uint64_t start = (uint64_t)i * CHUNK_SIZE;
        infos.push_back({config.work_dir + "/chunk_" + std::to_string(i) + ".bin", start});
    }

    if (Reassembler::reassemble(infos, output_path, file_size)) {
        std::string hash = Hasher::hashFile(output_path);
        std::cout << "[NODE " << node_id << "] DONE " << output_path << "  SHA-256=" << hash << "\n";
    } else {
        std::cerr << "[NODE " << node_id << "] FAIL reassembly\n";
    }

    node.stop();
    return 0;
}
