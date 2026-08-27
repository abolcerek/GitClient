#pragma once

#include <array>
#include <vector>
#include <cstdint>
#include <string>

namespace GitClient {
    constexpr size_t word_length = 80;
    constexpr std::string_view hexDigits = "0123456789abcdef";
    std::string sha1(const std::vector<std::byte>& data);
    std::string to_hex(const std::array<std::byte, 4>& data);
}