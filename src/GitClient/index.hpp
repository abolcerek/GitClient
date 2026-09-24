#pragma once

#include <map>
#include <filesystem>
#include <string>

namespace GitClient {
    struct IndexEntry{
        std::string mode;
        std::string hex;
    };
    std::map<std::string, IndexEntry> read_index(const std::filesystem::path& git_dir);
    void write_index(const std::filesystem::path& git_dir, const std::map<std::string, IndexEntry>& map);
    void add(const std::filesystem::path& git_dir, const std::filesystem::path& worktree, const std::filesystem::path& path);
}