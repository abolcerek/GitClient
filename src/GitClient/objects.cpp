#include "objects.hpp"
#include "object_store.hpp"

#include <utility>
#include <string>
#include <filesystem>
#include <vector>
#include <cstddef>
#include <fstream>
#include <algorithm>
#include <stdexcept>
#include <charconv>
#include <numeric>

namespace fs = std::filesystem;

namespace GitClient {
    std::pair<std::string, std::vector<std::byte>> read_object_raw(const fs::path& root, std::string_view hex) {
        const fs::path file_path = root / "objects" / hex.substr(0, 2) / hex.substr(2);
        auto buffer = GitClient::read_file(file_path);
        auto it = std::find(buffer.begin(), buffer.end(), std::byte{0});
        if (it == buffer.end()) {
            throw std::runtime_error("Header and payload could not be parsed");
        }
        std::vector<std::byte> header_vector(buffer.begin(), it);
        std::string header(reinterpret_cast<const char*>(header_vector.data()), header_vector.size());
        auto space_pos = header.find(' ');
        if (space_pos == std::string::npos) {
            throw std::runtime_error("Error: byte is incorrectly formatted");
        }
        std::string type = header.substr(0, space_pos);
        std::string payload_size = header.substr(space_pos + 1);
        size_t value;
        auto res = std::from_chars(payload_size.data(), payload_size.data() + payload_size.size(), value);
        if (res.ec != std::errc()) {
            throw std::runtime_error("Error parsing payload size");
        }
        std::vector<std::byte> payload(it + 1, buffer.end());
        if (value != payload.size()) {
            throw std::runtime_error("Error: payload size does not match size in header");
        } 
        return {type, payload};
    }

    bool tree_entry_less(const TreeEntry& a, const TreeEntry& b) {
        auto key_a = a.name + (a.mode == directory ? "/" : "");
        auto key_b = b.name + (b.mode == directory ? "/" : "");
        return key_a < key_b;
    }

    void string_to_bytes(std::vector<std::byte>& res, std::string_view input) {
        for (auto& i : input) {
            res.push_back(static_cast<std::byte>(i));
        }
    }
    std::vector<std::byte> serialize_tree(const std::vector<TreeEntry>& entries) {
        std::vector<std::byte> res;
        size_t total = std::accumulate(entries.begin(), entries.end(), size_t(0),[](size_t current, const TreeEntry& entry) {
            return current + entry.name.size() + entry.mode.size() + entry.hash.size() + 2;
        });
        res.reserve(total);
        auto sorted = entries;
        std::sort(sorted.begin(), sorted.end(), tree_entry_less);
        for (const auto& entry : sorted) {
            string_to_bytes(res, entry.mode);
            res.push_back(std::byte{' '});
            string_to_bytes(res, entry.name);
            res.push_back(std::byte{0});
            for (auto& hash_byte : entry.hash) {
                res.push_back(hash_byte);
            }
        }
        return res;
    }

    std::array<std::byte, hash_size> write_tree(const std::filesystem::path& root, const std::filesystem::path& dir) {
        std::vector<TreeEntry> entries;
        if (fs::exists(dir) && fs::is_directory(dir)) {
            for (const auto& entry : fs::directory_iterator(dir)) {
                //auto tree_entry = TreeEntry{};
                //skipping sub directories for now
                if (entry.path().filename().string() == ".git") {
                        continue;
                }
                if (fs::is_regular_file(entry)) {
                    auto tree_entry = TreeEntry{};
                    if ((fs::status(entry).permissions() & fs::perms::owner_exec) != fs::perms::none)
                        tree_entry.mode = "100755";
                    else 
                        tree_entry.mode = std::string(normal_file); 
                    tree_entry.name = entry.path().filename().string();
                    tree_entry.hash = hash_object(root, entry.path(), true);
                    entries.push_back(std::move(tree_entry));
                }
            }
        } else {
            throw std::runtime_error("Error: path is not a directory and does not exist");
        }
        return write_record(root, "tree", serialize_tree(entries), true);
    }

}