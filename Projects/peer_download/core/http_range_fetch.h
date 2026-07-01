#pragma once

#include <string>
#include <cstdint>

class HttpRangeFetch {
public:
    // Download a byte range from URL to output file
    // Returns true on success, false on failure
    // Checks if server supports range requests (Accept-Ranges header)
    static bool fetchRange(const std::string& url, uint64_t start, uint64_t end, const std::string& output_path);

    // Check if URL supports range requests
    static bool supportsRange(const std::string& url);
};
