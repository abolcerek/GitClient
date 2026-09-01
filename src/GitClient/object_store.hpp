#pragma once

#include "sha1.hpp"
#include <array>
#include <cstddef>
#include <filesystem>
#include <vector>
#include <cstdint>
#include <string>

namespace GitClient {
    std::array<std::byte, GitClient::hash_size> hash_object(const std::filesystem::path& root, const std::filesystem::path& file_path, std::string type, bool write);
    std::vector<std::byte> read_file(const std::filesystem::path& file_path);
}