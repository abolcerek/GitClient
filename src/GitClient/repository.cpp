#include "repository.hpp"

#include <fstream>
#include <filesystem>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace fs = std::filesystem;

namespace GitClient {
    bool init_repository(const fs::path& root) {
        const fs::path gitPath = root / ".git";
        if (fs::exists(gitPath)) {
            return false;
        } 
        const fs::path objectsPath = gitPath / "objects";
        const fs::path refsPath = gitPath / "refs" / "heads";
        const fs::path headPath = gitPath / "HEAD";
        fs::create_directories(objectsPath);
        fs::create_directories(refsPath);
        std::ofstream HEAD(headPath);
        if (!HEAD) {
            return false;
        }
        HEAD << "ref: refs/heads/main\n";
        return true;
    }
    fs::path read_head_ref_path(const fs::path& dir) {
        std::ifstream HEAD(dir);
        if (!HEAD.is_open()) {
            throw std::runtime_error("Error: HEAD file not found");
        }
        std::string contents((std::istreambuf_iterator<char>(HEAD)), std::istreambuf_iterator<char>());
        if (!contents.starts_with("ref: ")) {
            throw std::runtime_error("Error: head file does not contain reference");
        }
        std::string path = contents.substr(5);
        std::erase(path, '\n');
        return path;
    }
    std::optional<std::string> resolve_head(const fs::path& dir) { 
        const fs::path headPath = dir / "HEAD";
        auto path = read_head_ref_path(headPath);
        std::ifstream file(dir / path);
        if (!file.is_open()) {
            return std::nullopt;
        }
        std::string hash((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        std::erase(hash, '\n');
        if (hash.size() != 40) {
            throw std::runtime_error("Incorrect hash format");
        }
        return hash;
    }
    void update_ref(const fs::path& dir, std::string_view hex) {
        const fs::path headPath = dir / "HEAD";
        auto path = read_head_ref_path(headPath);
        fs::create_directories((dir / path).parent_path());
        std::ofstream file(dir / path);
        if (!file) {
            throw std::runtime_error("Unable to open file");
        }
        file << hex << "\n";
    }
}