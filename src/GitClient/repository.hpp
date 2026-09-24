#pragma once

#include <filesystem>
#include <optional>
#include <vector>
#include "objects.hpp"
namespace GitClient {
    bool init_repository(const std::filesystem::path& root);
    std::filesystem::path read_head_ref_path(const std::filesystem::path& dir);
    std::optional<std::string> resolve_head(const std::filesystem::path& dir);
    void update_ref(const std::filesystem::path& dir, std::string_view hex);
    std::vector<std::pair<std::string, CommitData>> collect_history(const std::filesystem::path& git_dir);
}