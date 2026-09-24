#pragma once

#include <map>
#include <filesystem>
#include <string>

#include "object_store.hpp"

namespace GitClient {
    struct IndexEntry{
        std::string mode;
        std::string hex;
    };
    std::map<std::string, IndexEntry> read_index(const std::filesystem::path& git_dir);
    void write_index(const std::filesystem::path& git_dir, const std::map<std::string, IndexEntry>& map);
    void add(const std::filesystem::path& git_dir, const std::filesystem::path& worktree, const std::filesystem::path& path);
    std::array<std::byte, GitClient::hash_size> write_tree_from_index(const std::filesystem::path& git_dir, const std::map<std::string, IndexEntry>& map);
}