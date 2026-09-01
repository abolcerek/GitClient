#pragma once

#include "object_store.hpp"

#include <utility>
#include <string>
#include <filesystem>
#include <vector>
#include <cstddef>
#include <array>

namespace GitClient {
    constexpr std::string_view normal_file = "100644";
    constexpr std::string_view executable = "100755";
    constexpr std::string_view directory = "40000";
    constexpr std::string_view simlink = "120000";
    std::pair<std::string, std::vector<std::byte>> read_object_raw(const std::filesystem::path& root, std::string_view hex);
    struct TreeEntry {
        std::string mode;
        std::string name;
        std::array<std::byte, hash_size> hash;
    };
    std::array<std::byte, hash_size> write_tree(const std::filesystem::path& root, const std::filesystem::path& dir);
    bool tree_entry_less(const TreeEntry&, const TreeEntry&);
    std::vector<std::byte> serialize_tree(const std::vector<TreeEntry>& entries);
    std::vector<std::byte> string_to_bytes(std::string input);
}