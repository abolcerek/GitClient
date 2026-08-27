#include "sha1.hpp"

#include <array>
#include <vector>
#include <cstdint>
#include <string>
#include <bit>

namespace GitClient {
    std::array<std::byte, 20> sha1(const std::vector<std::byte>& data) {
        uint64_t file_length = data.size() * 8;
        auto padding = data;
        padding.push_back(std::byte{0x80});
        auto padding_required = (56 - padding.size()) % 64;
        padding.insert(padding.end(), padding_required, std::byte{0});
        if constexpr (std::endian::native == std::endian::little) { // big endian conversion
            file_length = std::byteswap(data.size() * 8);
        }
        const std::byte* bytes = reinterpret_cast<const std::byte*>(&file_length);
        padding.insert(padding.end(), bytes, bytes + sizeof(file_length));
        for (auto it = padding.begin(); it != padding.end(); std::advance(it, 64)) { // for each 512 bit chunk in file contents
            auto slice = std::ranges::subrange(it, it + 64); // 512-bit chunk
            std::array<uint32_t, 80> words;
            size_t i = 0;
            for (auto inner_it = slice.begin(); inner_it != slice.end(); std::advance(inner_it, 4)) { // for each 32 bit word in the chunk
                auto val = std::ranges::subrange(inner_it, inner_it + 4); // 32-bit word


            }
        }
    }
}
