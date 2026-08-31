#pragma once

#include <array>
#include <cstddef>
#include <vector>
#include <string>

namespace GitClient {
    constexpr size_t word_length = 80;
    constexpr size_t hash_size = 20;
    constexpr std::string_view hexDigits = "0123456789abcdef";
    std::array<std::byte, hash_size> sha1(const std::vector<std::byte>& data);
    std::string to_hex(const std::array<std::byte, hash_size>& data);
}