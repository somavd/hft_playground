# Memory-Mapped Files

This folder demonstrates memory-mapped files for high-performance I/O operations, commonly used in HFT applications.

## Files

- `mmap.cpp` - Clean memory-mapped file implementation with RAII pattern
- `README.md` - This file

## Compile and Run

```bash
# Build the memory-mapped file implementation
g++ -O3 -std=c++20 -pthread mmap.cpp -o mmap
./mmap
```

## What It Demonstrates

### Memory-Mapped File Implementation (`mmap.cpp`)

**Concepts:**
- RAII pattern for resource management
- Memory-mapped file creation and access
- Direct memory operations on file data
- Exception-safe error handling

**Key Features:**
- **Clean Class Design**: `MemoryMappedFile` class with proper encapsulation
- **RAII Pattern**: Automatic resource cleanup in destructor
- **Error Handling**: Comprehensive exception handling with proper cleanup
- **Simple API**: `data()` method for memory access, `flush()` for synchronization
- **Verification**: Data integrity checking after operations

**Code Quality Highlights:**
- Exception-safe construction and destruction
- Proper file descriptor and memory cleanup
- Clear error messages via `std::runtime_error`
- Safe pointer casting with `static_cast`
- Resource leak prevention

**Usage Example:**
```cpp
MemoryMappedFile mmap_file("data.dat", 1024 * 1024); // 1MB file
char* data = static_cast<char*>(mmap_file.data());

// Write to file through memory
for (size_t i = 0; i < 1024 * 1024; ++i) {
    data[i] = 'A' + (i % 26);
}

// Sync to disk
mmap_file.flush();
```










