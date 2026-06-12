#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdexcept>

class MemoryMappedFile {
private:
    int fd;
    void* mapped_data;
    size_t file_size;
    
public:
    MemoryMappedFile(const char* filename, size_t size) : fd(-1), mapped_data(nullptr), file_size(size) {
        // Create and open file
        fd = open(filename, O_RDWR | O_CREAT, 0666);
        if (fd == -1) {
            throw std::runtime_error("Failed to open file");
        }
        
        // Resize file
        if (ftruncate(fd, size) == -1) {
            close(fd);
            throw std::runtime_error("Failed to resize file");
        }
        
        // Map file to memory
        mapped_data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (mapped_data == MAP_FAILED) {
            close(fd);
            throw std::runtime_error("Failed to map file");
        }
    }
    
    ~MemoryMappedFile() {
        if (mapped_data) {
            munmap(mapped_data, file_size);
        }
        if (fd != -1) {
            close(fd);
        }
    }
    
    void* data() {
        return mapped_data;
    }
    
    void flush() {
        if (msync(mapped_data, file_size, MS_SYNC) == -1) {
            throw std::runtime_error("Failed to sync file");
        }
    }
};

int main() {

    const char *filename = "test.dat";
    const size_t file_size = 1024 * 1024; // 1MB
    
    try {
        MemoryMappedFile mmap_file(filename, file_size);

        char *data = static_cast<char*>(mmap_file.data());
        
        // Fill data with pattern
        for (size_t i = 0; i < file_size; ++i) {
            data[i] = 'A' + (i % 26);
        }
        
        mmap_file.flush();

        // Verify data
        void *verify_data = mmap_file.data();
        bool correct = true;
        for (size_t i = 0; i < 10; ++i) {
            if (static_cast<char*>(verify_data)[i] != 'A' + (i % 26)) {
                correct = false;
                break;
            }
        }
        std::cout << "Data verification: " << (correct ? "PASSED" : "FAILED") << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    unlink(filename);
    
    return 0;
}
