#pragma once
#include <filesystem>

namespace GitClient {
    bool init_repository(const std::filesystem::path& root);
}