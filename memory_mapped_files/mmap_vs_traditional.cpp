#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <random>
#include <vector>

class MemoryMappedFile {
private:
    int fd;
    void* mapped_data;
    size_t file_size;
    
public:
    MemoryMappedFile(const char* filename, size_t size) : fd(-1), mapped_data(nullptr), file_size(size) {
        fd = open(filename, O_RDWR | O_CREAT, 0666);
        if (fd == -1) throw std::runtime_error("Failed to open file");
        
        if (ftruncate(fd, size) == -1) {
            close(fd);
            throw std::runtime_error("Failed to resize file");
        }
        
        mapped_data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (mapped_data == MAP_FAILED) {
            close(fd);
            throw std::runtime_error("Failed to map file");
        }
    }
    
    ~MemoryMappedFile() {
        if (mapped_data) munmap(mapped_data, file_size);
        if (fd != -1) close(fd);
    }
    
    void* data() { return mapped_data; }
    size_t size() const { return file_size; }
    
    void flush() {
        if (msync(mapped_data, file_size, MS_SYNC) == -1) {
            throw std::runtime_error("Failed to sync file");
        }
    }
};

void demonstrate_random_access_advantage() {
    std::cout << "=== Random Access: mmap vs Traditional I/O ===\n\n";
    
    const char* filename = "random_test.dat";
    const size_t file_size = 100 * 1024 * 1024; // 100MB
    const int num_operations = 1000000;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> pos_dist(0, file_size - 8);
    std::uniform_int_distribution<uint64_t> value_dist;
    
    // Test mmap random access
    {
        MemoryMappedFile mmap_file(filename, file_size);
        uint64_t* data = static_cast<uint64_t*>(mmap_file.data());
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_operations; ++i) {
            size_t pos = pos_dist(gen) / 8; // Ensure 8-byte alignment
            data[pos] = value_dist(gen);
        }
        
        mmap_file.flush();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "mmap random access: " << duration.count() << " ms\n";
    }
    
    // Test traditional I/O random access
    {
        int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (fd == -1) {
            std::cerr << "Failed to open file for traditional I/O\n";
            return;
        }
        
        // Initialize file with zeros
        char* zero_buffer = new char[64 * 1024];
        memset(zero_buffer, 0, 64 * 1024);
        for (size_t i = 0; i < file_size; i += 64 * 1024) {
            write(fd, zero_buffer, std::min(64 * 1024UL, file_size - i));
        }
        delete[] zero_buffer;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_operations; ++i) {
            size_t pos = pos_dist(gen);
            uint64_t value = value_dist(gen);
            
            if (lseek(fd, pos, SEEK_SET) == -1) {
                std::cerr << "Seek error\n";
                break;
            }
            
            if (write(fd, &value, sizeof(value)) != sizeof(value)) {
                std::cerr << "Write error\n";
                break;
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Traditional I/O random access: " << duration.count() << " ms\n";
        close(fd);
    }
    
    unlink(filename);
}

void demonstrate_read_modify_write() {
    std::cout << "\n=== Read-Modify-Write: mmap vs Traditional I/O ===\n\n";
    
    const char* filename = "rmw_test.dat";
    const size_t file_size = 50 * 1024 * 1024; // 50MB
    const int num_operations = 500000;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> pos_dist(0, (file_size / sizeof(uint64_t)) - 1);
    
    // Test mmap read-modify-write
    {
        MemoryMappedFile mmap_file(filename, file_size);
        uint64_t* data = static_cast<uint64_t*>(mmap_file.data());
        
        // Initialize with zeros
        memset(data, 0, file_size);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_operations; ++i) {
            size_t pos = pos_dist(gen);
            data[pos]++; // Read, modify, write in one operation
        }
        
        mmap_file.flush();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "mmap read-modify-write: " << duration.count() << " ms\n";
    }
    
    // Test traditional I/O read-modify-write
    {
        int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (fd == -1) {
            std::cerr << "Failed to open file for traditional I/O\n";
            return;
        }
        
        // Initialize with zeros
        char* zero_buffer = new char[64 * 1024];
        memset(zero_buffer, 0, 64 * 1024);
        for (size_t i = 0; i < file_size; i += 64 * 1024) {
            write(fd, zero_buffer, std::min(64 * 1024UL, file_size - i));
        }
        delete[] zero_buffer;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_operations; ++i) {
            size_t pos = pos_dist(gen) * sizeof(uint64_t);
            uint64_t value;
            
            // Read
            if (lseek(fd, pos, SEEK_SET) == -1) break;
            if (read(fd, &value, sizeof(value)) != sizeof(value)) break;
            
            // Modify
            value++;
            
            // Write
            if (lseek(fd, pos, SEEK_SET) == -1) break;
            if (write(fd, &value, sizeof(value)) != sizeof(value)) break;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Traditional I/O read-modify-write: " << duration.count() << " ms\n";
        close(fd);
    }
    
    unlink(filename);
}

void demonstrate_serial_access() {
    std::cout << "\n=== Serial I/O Access: mmap vs Traditional I/O ===\n\n";
    
    const char* filename = "serial_test.dat";
    const size_t file_size = 100 * 1024 * 1024; // 100MB
    
    // Test mmap serial I/O access
    {
        MemoryMappedFile mmap_file(filename, file_size);
        char* data = static_cast<char*>(mmap_file.data());
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Serial write - fill entire file sequentially
        for (size_t i = 0; i < file_size; ++i) {
            data[i] = static_cast<char>(i % 256);
        }
        
        mmap_file.flush();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "mmap serial I/O: " << duration.count() << " ms\n";
    }
    
    // Test traditional I/O serial access
    {
        int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (fd == -1) {
            std::cerr << "Failed to open file for traditional I/O\n";
            return;
        }
        
        // Serial write
        char* buffer = new char[64 * 1024];
        for (int i = 0; i < 64 * 1024; ++i) {
            buffer[i] = static_cast<char>(i % 256);
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < file_size; i += 64 * 1024) {
            size_t chunk_size = std::min(64 * 1024UL, file_size - i);
            if (write(fd, buffer, chunk_size) != static_cast<ssize_t>(chunk_size)) {
                std::cerr << "Write error\n";
                break;
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Traditional I/O serial: " << duration.count() << " ms\n";
        
        delete[] buffer;
        close(fd);
    }
    
    unlink(filename);
}

void demonstrate_patterned_access() {
    std::cout << "\n=== Patterned Access: mmap vs Traditional I/O ===\n\n";
    
    const char* filename = "pattern_test.dat";
    const size_t file_size = 64 * 1024 * 1024; // 64MB
    const size_t page_size = 4096;
    
    // Test mmap patterned access (stride pattern)
    {
        MemoryMappedFile mmap_file(filename, file_size);
        char* data = static_cast<char*>(mmap_file.data());
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Access every 256th byte (simulating sparse access pattern)
        for (size_t i = 0; i < file_size; i += 256) {
            data[i] = static_cast<char>(i & 0xFF);
        }
        
        mmap_file.flush();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "mmap patterned access: " << duration.count() << " ms\n";
    }
    
    // Test traditional I/O patterned access
    {
        int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (fd == -1) {
            std::cerr << "Failed to open file for traditional I/O\n";
            return;
        }
        
        char buffer = 0;
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < file_size; i += 256) {
            if (lseek(fd, i, SEEK_SET) == -1) break;
            buffer = static_cast<char>(i & 0xFF);
            if (write(fd, &buffer, 1) != 1) break;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Traditional I/O patterned access: " << duration.count() << " ms\n";
        close(fd);
    }
    
    unlink(filename);
}

int main() {
    std::cout << "Memory-Mapped Files vs Traditional I/O Performance Comparison\n";
    std::cout << "============================================================\n\n";
    
    demonstrate_random_access_advantage();
    demonstrate_read_modify_write();
    demonstrate_patterned_access();
    demonstrate_serial_access();
    
    return 0;
}
