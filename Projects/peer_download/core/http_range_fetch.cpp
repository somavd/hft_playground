#include "http_range_fetch.h"
#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <cstring>

namespace {
    size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t totalSize = size * nmemb;
        std::ofstream* file = static_cast<std::ofstream*>(userp);
        file->write(static_cast<char*>(contents), totalSize);
        return totalSize;
    }

    size_t headerCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
        size_t totalSize = size * nitems;
        std::string* header = static_cast<std::string*>(userdata);
        *header += std::string(buffer, totalSize);
        return totalSize;
    }
}

bool HttpRangeFetch::supportsRange(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }

    std::string headers;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);  // HEAD request
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headers);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    bool supports = false;

    if (res == CURLE_OK) {
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code == 200) {
            // Check for Accept-Ranges header
            supports = headers.find("Accept-Ranges: bytes") != std::string::npos ||
                       headers.find("accept-ranges: bytes") != std::string::npos;
        }
    }

    curl_easy_cleanup(curl);
    return supports;
}

bool HttpRangeFetch::fetchRange(const std::string& url, uint64_t start, uint64_t end, const std::string& output_path) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize curl" << std::endl;
        return false;
    }

    std::ofstream outfile(output_path, std::ios::binary);
    if (!outfile) {
        std::cerr << "Failed to open output file: " << output_path << std::endl;
        curl_easy_cleanup(curl);
        return false;
    }

    char range_header[64];
    snprintf(range_header, sizeof(range_header), "%llu-%llu", (unsigned long long)start, (unsigned long long)end);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_RANGE, range_header);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outfile);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    CURLcode res = curl_easy_perform(curl);
    bool success = (res == CURLE_OK);

    if (!success) {
        std::cerr << "Failed to fetch range: " << curl_easy_strerror(res) << std::endl;
    }

    curl_easy_cleanup(curl);
    outfile.close();
    return success;
}
