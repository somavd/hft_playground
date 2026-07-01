# Peer Download - Distributed File Download MVP

Phase 1 implementation of a peer-assisted distributed download system. This MVP demonstrates chunking, HTTP range-based downloads, SHA-256 verification, and reassembly without any networking layer.

## Project Structure

```
peer_download/
├── core/                      # C++ core library
│   ├── chunker.h/cpp         # Byte-range calculation
│   ├── http_range_fetch.h/cpp # libcurl HTTP range requests
│   ├── hasher.h/cpp          # SHA-256 verification
│   └── reassembler.h/cpp     # Chunk reassembly
├── desktop-test-harness/      # CLI executables for testing
│   ├── peer_dl.cpp           # Download a single peer's chunk
│   └── reassemble.cpp        # Reassemble chunks into final file
├── tests/                     # Unit tests
│   ├── test_chunker.cpp
│   └── test_hasher.cpp
├── CMakeLists.txt
└── README.md
```

## Dependencies

- CMake 3.20+
- C++17 compiler
- libcurl
- OpenSSL

### macOS

```bash
brew install cmake curl openssl
```

### Linux (Ubuntu/Debian)

```bash
sudo apt-get install cmake libcurl4-openssl-dev libssl-dev
```

## Building

```bash
cd peer_download
mkdir build && cd build
cmake ..
make
```

This will create:
- `peer_dl` - CLI to download a peer's chunk
- `reassemble` - CLI to reassemble chunks
- `test_chunker` - Unit tests for chunker
- `test_hasher` - Unit tests for hasher

## Running Tests

```bash
./test_chunker
./test_hasher
```

## Usage Example

### Step 1: Download chunks (simulate 2 peers)

For a 1MB file split between 2 peers:

```bash
# Peer 0 downloads first half
./peer_dl --url=https://example.com/file.bin --size=1048576 --peers=2 --index=0 --out=chunk_0.bin

# Peer 1 downloads second half
./peer_dl --url=https://example.com/file.bin --size=1048576 --peers=2 --index=1 --out=chunk_1.bin
```

### Step 2: Reassemble chunks

```bash
./reassemble --chunks=chunk_0.bin,chunk_1.bin --out=file.bin --size=1048576
```

### Step 3: Verify

```bash
# Compare SHA-256 with original
shasum -a 256 file.bin
```

## Command Reference

### peer_dl

```
--url=<url>      : URL of the file to download
--size=<bytes>   : Total file size in bytes
--peers=<n>      : Total number of peers in the cluster
--index=<i>      : This peer's index (0 to peers-1)
--out=<file>     : Output file path for this peer's chunk
```

### reassemble

```
--chunks=<files> : Comma-separated list of chunk files
--out=<file>     : Output file path
--size=<bytes>   : Total file size in bytes
```

## How It Works

1. **Chunker**: Divides the file into N byte ranges based on peer count. Handles uneven division by distributing remainder bytes across first few peers.

2. **HTTP Range Fetch**: Uses libcurl to download only the assigned byte range via HTTP `Range` header. Checks if server supports range requests first.

3. **Hasher**: Computes SHA-256 of downloaded chunks for verification. Can hash entire files or specific byte ranges.

4. **Reassembler**: Pre-allocates output file using `ftruncate`, then writes each chunk at its correct offset using `pwrite` (out-of-order safe).

## Limitations (MVP)

- File size must be specified manually (no HEAD request parsing yet)
- Chunks must be provided in order to reassemble (no offset metadata)
- No networking layer (peers simulated as separate processes on same machine)
- No error recovery or resume capability

## Next Steps (Phase 2)

- Add HEAD request to auto-detect file size
- Implement chunk metadata file (offset, hash, size)
- Add local discovery (BLE/Wi-Fi Direct)
- Implement peer-to-peer chunk exchange protocol
- Add Android JNI bridge

## References

- [Microsoft COMBINE](https://www.microsoft.com/en-us/research/publication/combine-boosting-mobile-network-performance-with-collaborative-downloads/)
- [2Fast](https://dl.acm.org/doi/10.1145/2674005.2674996)
- [7DS](https://ieeexplore.ieee.org/document/1257549)
