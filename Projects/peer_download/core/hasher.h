#pragma once

#include <string>
#include <vector>
#include <cstdint>

class Hasher {
public:
    // Compute SHA-256 hash of a file
    // Returns hex string of hash, or empty string on error
    static std::string hashFile(const std::string& file_path);

    // Compute SHA-256 hash of a byte range within a file
    // Returns hex string of hash, or empty string on error
    static std::string hashFileRange(const std::string& file_path, uint64_t offset, uint64_t length);

    // Compute SHA-256 hash of a buffer
    // Returns hex string of hash
    static std::string hashBuffer(const uint8_t* data, size_t length);
};
