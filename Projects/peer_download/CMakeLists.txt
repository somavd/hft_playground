cmake_minimum_required(VERSION 3.20)
project(peer_download VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Force arm64 on Apple Silicon to match Homebrew OpenSSL
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES arm64)
endif()

# Find required packages
find_package(CURL REQUIRED)

include_directories(${CURL_INCLUDE_DIRS})

# Use OpenSSL on all platforms
find_package(OpenSSL REQUIRED)
set(CRYPTO_LIB OpenSSL::Crypto)
set(CRYPTO_DEFINE "USE_OPENSSL")
include_directories(${OPENSSL_INCLUDE_DIR})

# Core library
add_library(core STATIC
    core/node.cpp
    core/http_range_fetch.cpp
    core/hasher.cpp
    core/reassembler.cpp
    core/chunk_manager.cpp
    core/peer_discovery.cpp
)

target_compile_definitions(core PRIVATE ${CRYPTO_DEFINE})
target_link_libraries(core
    ${CURL_LIBRARIES}
    ${CRYPTO_LIB}
)

# peer_node executable (v2)
add_executable(peer_node
    desktop-test-harness/peer_node.cpp
)

target_compile_definitions(peer_node PRIVATE ${CRYPTO_DEFINE})
target_link_libraries(peer_node
    core
    ${CURL_LIBRARIES}
    ${CRYPTO_LIB}
)

# coordinator executable (v2)
add_executable(coordinator
    desktop-test-harness/coordinator.cpp
)

target_compile_definitions(coordinator PRIVATE ${CRYPTO_DEFINE})
target_link_libraries(coordinator
    core
    ${CRYPTO_LIB}
)

# Test executables
add_executable(test_hasher
    tests/test_hasher.cpp
)

target_compile_definitions(test_hasher PRIVATE ${CRYPTO_DEFINE})
target_link_libraries(test_hasher
    core
    ${CRYPTO_LIB}
)

add_executable(test_half_fetch
    test_half_fetch.cpp
)

target_compile_definitions(test_half_fetch PRIVATE ${CRYPTO_DEFINE})
target_link_libraries(test_half_fetch
    core
    ${CRYPTO_LIB}
)

# Compiler warnings
if(MSVC)
    target_compile_options(core PRIVATE /W4)
else()
    target_compile_options(core PRIVATE -Wall -Wextra -Wpedantic)
endif()
