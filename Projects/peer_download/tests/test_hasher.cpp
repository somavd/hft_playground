#include "../core/hasher.h"
#include <cassert>
#include <iostream>
#include <fstream>

void testHashBuffer() {
    std::string data = "hello world";
    std::string hash = Hasher::hashBuffer(reinterpret_cast<const uint8_t*>(data.c_str()), data.length());
    // Known SHA-256 for "hello world"
    assert(hash == "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9");
    std::cout << "PASS: Hash buffer" << std::endl;
}

void testHashFile() {
    // Create a test file
    std::ofstream test_file("test_hash_input.txt");
    test_file << "hello world";
    test_file.close();

    std::string hash = Hasher::hashFile("test_hash_input.txt");
    assert(hash == "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9");
    std::cout << "PASS: Hash file" << std::endl;

    // Cleanup
    std::remove("test_hash_input.txt");
}

void testHashFileRange() {
    // Create a test file
    std::ofstream test_file("test_hash_range.txt");
    test_file << "0123456789";
    test_file.close();

    // Hash bytes 0-4 ("01234")
    std::string hash1 = Hasher::hashFileRange("test_hash_range.txt", 0, 5);
    // Hash bytes 5-9 ("56789")
    std::string hash2 = Hasher::hashFileRange("test_hash_range.txt", 5, 5);

    // These should be different
    assert(hash1 != hash2);
    std::cout << "PASS: Hash file range" << std::endl;

    // Cleanup
    std::remove("test_hash_range.txt");
}

int main() {
    std::cout << "=== Hasher Tests ===" << std::endl;
    testHashBuffer();
    testHashFile();
    testHashFileRange();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
