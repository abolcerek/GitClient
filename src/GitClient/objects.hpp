#pragma once

#include <utility>
#include <string>
#include <filesystem>
#include <vector>
#include <cstddef>

namespace GitClient {
    std::pair<std::string, std::vector<std::byte>> read_object_raw(const std::filesystem::path& root, std::string_view hex);
}