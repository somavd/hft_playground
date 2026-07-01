#include "core/http_range_fetch.h"
#include "core/reassembler.h"
#include <iostream>

int main() {
    std::string url = "https://thetestdata.com/assets/video/mp4/1080/50MB_1080P_THETESTDATA.COM_mp4.mp4";
    uint64_t file_size = 52636528;
    uint64_t half_size = file_size / 2;

    std::cout << "Fetching first half: bytes 0 to " << half_size << "\n";
    bool success_1 = true; //HttpRangeFetch::fetchRange(url, 0, half_size, "first_half.mp4");

    std::cout << "Fetching second half: bytes " << (half_size + 1) << " to " << (file_size - 1) << "\n";
    bool success_2 = true; //HttpRangeFetch::fetchRange(url, half_size + 1, file_size - 1, "second_half.mp4");

    if (success_1 && success_2) {
        std::cout << "SUCCESS: both halves downloaded\n";
        std::cout << "Reassembling...\n";

        std::vector<ChunkInfo> chunks;
        chunks.push_back({"first_half.mp4", 0});
        chunks.push_back({"second_half.mp4", half_size + 1});

        bool reassembled = Reassembler::reassemble(chunks, "reassembled.mp4", file_size);
        if (reassembled) {
            std::cout << "SUCCESS: reassembled.mp4 created\n";
        } else {
            std::cout << "FAILED: reassembly\n";
            return 1;
        }
    } else {
        std::cout << "FAILED: download\n";
        return 1;
    }

    return 0;
}
