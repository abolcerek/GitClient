#pragma once

#include "sha1.hpp"
#include <array>
#include <cstddef>
#include <filesystem>
#include <vector>
#include <cstdint>
#include <string>

namespace GitClient {
    std::vector<std::byte> read_file(const std::filesystem::path& file_path);
    std::array<std::byte, GitClient::hash_size> write_record(const std::filesystem::path& root, std::string type, const std::vector<std::byte>& payload, bool write);
    std::array<std::byte, GitClient::hash_size> hash_object(const std::filesystem::path& root, const std::filesystem::path& file_path, bool write);
}