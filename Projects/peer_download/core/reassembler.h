#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct ChunkInfo {
    std::string path;
    uint64_t offset;
};

class Reassembler {
public:
    // Reassemble chunks into a single file
    // Chunks are written at their specified offsets (out-of-order safe)
    // Returns true on success, false on failure
    static bool reassemble(const std::vector<ChunkInfo>& chunks, const std::string& output_path, uint64_t total_size);
};
