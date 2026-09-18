#pragma once

#include <vector>

namespace GitClient {
    std::vector<std::byte> zlib_compress(const std::vector<std::byte>& payload);
    std::vector<std::byte> zlib_decompress(const std::vector<std::byte> &payload);
}