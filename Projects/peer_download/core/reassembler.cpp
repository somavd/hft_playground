#include "reassembler.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

bool Reassembler::reassemble(const std::vector<ChunkInfo>& chunks, const std::string& output_path, uint64_t total_size) {
    // Create output file and pre-allocate space
    std::ofstream outfile(output_path, std::ios::binary);
    if (!outfile) {
        std::cerr << "Failed to create output file: " << output_path << std::endl;
        return false;
    }

    // Use ftruncate to pre-allocate (requires file descriptor)
    outfile.close();
    int fd = open(output_path.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        std::cerr << "Failed to open file descriptor for truncation" << std::endl;
        return false;
    }

    if (ftruncate(fd, total_size) == -1) {
        std::cerr << "Failed to pre-allocate file" << std::endl;
        close(fd);
        return false;
    }
    close(fd);

    // Reopen for writing
    outfile.open(output_path, std::ios::binary | std::ios::in);
    if (!outfile) {
        std::cerr << "Failed to reopen output file" << std::endl;
        return false;
    }

    // Write each chunk at its offset
    for (const auto& chunk : chunks) {
        std::ifstream chunk_file(chunk.path, std::ios::binary);
        if (!chunk_file) {
            std::cerr << "Failed to open chunk file: " << chunk.path << std::endl;
            return false;
        }

        chunk_file.seekg(0, std::ios::end);
        size_t chunk_size = chunk_file.tellg();
        chunk_file.seekg(0, std::ios::beg);

        std::vector<char> buffer(chunk_size);
        chunk_file.read(buffer.data(), chunk_size);
        if (!chunk_file) {
            std::cerr << "Failed to read chunk file" << std::endl;
            return false;
        }

        outfile.seekp(chunk.offset);
        outfile.write(buffer.data(), chunk_size);
        if (!outfile) {
            std::cerr << "Failed to write chunk to output file at offset " << chunk.offset << std::endl;
            return false;
        }
    }

    outfile.close();
    return true;
}
