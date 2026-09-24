#include <map>
#include <filesystem>
#include <string>
#include <fstream>
#include <sstream>

#include "index.hpp"
#include "objects.hpp"

namespace fs = std::filesystem;

namespace GitClient {
    constexpr std::string_view index_file = "mygit-index";
    std::map<std::string, IndexEntry> read_index(const fs::path& git_dir) {
        std::map<std::string, IndexEntry> res;
        if (!fs::exists(git_dir / index_file)) {
            return res;
        }
        std::ifstream file(git_dir/ index_file);
        if (!file.is_open()) {
            throw std::runtime_error("Error when opening git index");
        }
        std::string indexes;
        while (std::getline(file, indexes)) {
            std::stringstream ss(indexes);
            IndexEntry entry;
            std::string path;
            std::getline(ss, entry.mode, ' ');
            std::getline(ss, entry.hex, ' ');
            std::getline(ss, path);
            res[path] = entry;
        }
        return res;
    }
    void write_index(const fs::path& git_dir, const std::map<std::string, IndexEntry>& map) {
        if (!fs::exists(git_dir)) {
            throw std::runtime_error("Error: git directory does not exist");
        }
        std::ofstream file(git_dir / index_file);
        if (!file.is_open()) {
            throw std::runtime_error("Error when opening git index");
        }
        for (const auto& [path, entry] : map) {
            file << entry.mode << " " << entry.hex << " " << path << "\n";
        }
    }
    void add(const std::filesystem::path& git_dir, const std::filesystem::path& worktree, const std::filesystem::path& path) {
        if (fs::is_directory(path)) {
            for (const auto& entry : fs::directory_iterator(path)) {
                if (entry.path().filename().string() == ".git") {
                    continue;
                }
                if (fs::is_regular_file(entry)) {
                    add(git_dir, worktree, entry);
                }
                if (fs::is_directory(entry)) {
                    add(git_dir, worktree, entry);
                }
            }
            return;
        }
        auto hash = hash_object(git_dir, path, true);
        std::string mode;
        if ((fs::status(path).permissions() & fs::perms::owner_exec) != fs::perms::none)
            mode = "100755";
        else 
            mode = std::string(normal_file); 
        auto idx = read_index(git_dir);
        idx[fs::relative(path, worktree).string()] = {mode, to_hex(hash)};
        write_index(git_dir, idx);
    }
    std::array<std::byte, GitClient::hash_size> write_tree_from_index(const std::filesystem::path& git_dir, const std::map<std::string, IndexEntry>& map) {
        std::vector<TreeEntry> entries;
        std::map<std::string, std::map<std::string, IndexEntry>> groups;
        for (const auto& [path, entry] : map) {
            if (!path.contains("/")) {        
                entries.push_back(TreeEntry{.mode = entry.mode, .name = path, .hash = hex_to_bytes(entry.hex)});
            }
            else {
                auto pos = path.find("/");
                auto prefix = path.substr(0, pos);
                auto rest = path.substr(pos + 1);
                groups[prefix][rest] = entry;
            }
        }
        for (const auto& [prefix, sub_map] : groups) {
            entries.push_back(TreeEntry{.mode = std::string(directory), .name = prefix, .hash = write_tree_from_index(git_dir, sub_map)});
        }
        return write_record(git_dir, "tree", serialize_tree(entries), true); 
    }
}