#include "../core/node.h"
#include "../core/hasher.h"
#include "../core/reassembler.h"
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    uint16_t listen_port = 0;
    std::string file_url;
    uint64_t file_size = 0;
    std::string output_path = "coordinator_output.mp4";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if      (arg.rfind("--port=", 0) == 0)  listen_port = std::atoi(arg.substr(7).c_str());
        else if (arg.rfind("--url=", 0) == 0)   file_url    = arg.substr(6);
        else if (arg.rfind("--size=", 0) == 0)  file_size   = std::atoll(arg.substr(7).c_str());
        else if (arg.rfind("--out=", 0) == 0)   output_path = arg.substr(6);
    }

    if (listen_port == 0 || file_url.empty() || file_size == 0) {
        std::cerr << "Usage: coordinator --port=PORT --url=URL --size=BYTES [--out=FILE]\n";
        return 1;
    }

    Config config;
    config.node_id = 0;
    config.listen_port = listen_port;
    config.file_url = file_url;
    config.file_size = file_size;
    config.output_path = output_path;
    config.work_dir = "node_0";
    config.is_coordinator = true;
    config.coordinator_host = "";
    config.coordinator_port = 0;

    std::cout << "[COORD] port=" << listen_port
              << "  chunks=" << ((file_size + CHUNK_SIZE - 1) / CHUNK_SIZE)
              << "  size=" << file_size << "\n";

    Node node(config);
    node.start();

    // Wait for all chunks to be downloaded
    while (!node.isComplete()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "[COORD] Progress: " << node.chunksDownloaded() << "/" << node.totalChunks() << "\n";
    }

    std::cout << "[COORD] All chunks downloaded. Reassembling...\n";

    // Reassemble
    std::vector<ChunkInfo> infos;
    for (uint32_t i = 0; i < node.totalChunks(); ++i) {
        uint64_t start = (uint64_t)i * CHUNK_SIZE;
        infos.push_back({config.work_dir + "/chunk_" + std::to_string(i) + ".bin", start});
    }

    if (Reassembler::reassemble(infos, output_path, file_size)) {
        std::string hash = Hasher::hashFile(output_path);
        std::cout << "[COORD] DONE " << output_path << "  SHA-256=" << hash << "\n";
    } else {
        std::cerr << "[COORD] FAIL reassembly\n";
    }

    node.stop();
    return 0;
}
