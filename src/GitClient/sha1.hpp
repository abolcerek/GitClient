#pragma once

#include <array>
#include <vector>
#include <cstdint>
#include <string>

namespace GitClient {
    inline constexpr uint32_t H0 = 0x67452301;
    inline constexpr uint32_t H1 = 0xEFCDAB89;
    inline constexpr uint32_t H2 = 0x98BADCFE;
    inline constexpr uint32_t H3 = 0x10325476;
    inline constexpr uint32_t H4 = 0xC3D2E1F0;
    std::array<std::byte, 20> sha1(const std::vector<std::byte>& data);
    std::string to_hex(const std::array<std::byte, 20>& data);
}