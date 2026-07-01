#include "hasher.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

std::string Hasher::hashBuffer(const uint8_t* data, size_t length) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(data, length, hash);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

std::string Hasher::hashFile(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        return "";
    }

    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return hashBuffer(buffer.data(), buffer.size());
}

std::string Hasher::hashFileRange(const std::string& file_path, uint64_t offset, uint64_t length) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        return "";
    }

    file.seekg(offset);
    if (!file) {
        return "";
    }

    std::vector<uint8_t> buffer(length);
    file.read(reinterpret_cast<char*>(buffer.data()), length);
    if (!file) {
        return "";
    }

    return hashBuffer(buffer.data(), length);
}
