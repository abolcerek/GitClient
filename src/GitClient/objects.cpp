#include "objects.hpp"
#include "object_store.hpp"
#include "sha1.hpp"

#include <iterator>
#include <ranges>
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
    Blob::Blob(std::vector<std::byte> content)
    : content_{std::move(content)} { }
    
    std::string_view Blob::type() const {
        return "blob";
    }
    std::vector<std::byte> Blob::serialize() const {
        return content_;
    }
    Tree::Tree(std::vector<TreeEntry> entries)
    : entries_{std::move(entries)} { }

    std::string_view Tree::type() const {
        return "tree";
    }
    std::vector<std::byte> Tree::serialize() const {
        return serialize_tree(entries_);
    }
    Commit::Commit(std::vector<std::byte> content) 
    : content_{std::move(content)} { }
    std::string_view Commit::type() const {
        return "commit";
    }
    std::vector<std::byte> Commit::serialize() const {
        return content_;
    }
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

    std::vector<TreeEntry> parse_tree(const std::vector<std::byte>& payload) {
        size_t pos = 0;
        std::vector<TreeEntry> res;
        while (pos < payload.size()) {
            TreeEntry entry;
            auto space = std::find(payload.begin() + pos, payload.end(), std::byte{' '});
            if (space == payload.end()) {
                throw std::runtime_error("Error: payload does not contain a space");
            }
            auto mode = std::ranges::subrange(payload.begin() + pos, space);
            entry.mode = std::string(reinterpret_cast<const char*>(&*mode.begin()), mode.size());
            pos = (space - payload.begin()) + 1;
            auto null_terminator = std::find(payload.begin() + pos, payload.end(), std::byte{0});
            if (null_terminator == payload.end()) {
                throw std::runtime_error("Error: payload does not contain a null terminator");
            }
            auto name = std::ranges::subrange(payload.begin() + pos, null_terminator);
            entry.name = std::string(reinterpret_cast<const char*>(&*name.begin()), name.size());
            pos = (null_terminator - payload.begin()) + 1;
            if ((pos + 20) <= payload.size()) {
                std::array<std::byte, hash_size> hash;
                std::copy_n(payload.begin() + pos, hash_size, hash.begin());
                entry.hash = hash;
                pos += 20;
            } else {
                throw std::runtime_error("Error: incorrect hash size");
            }
            res.push_back(std::move(entry));
        }
        return res;
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
                if (fs::is_directory(entry) && !fs::is_empty(entry)) {
                    auto tree_entry = TreeEntry{};
                    tree_entry.mode = std::string(directory);
                    tree_entry.name = entry.path().filename().string();
                    tree_entry.hash = write_tree(root, entry.path()); // does guard againt a subdir containing only empty subdirs yet
                    entries.push_back(std::move(tree_entry));
                }
            }
        } else {
            throw std::runtime_error("Error: path is not a directory and does not exist");
        }
        return write_record(root, "tree", serialize_tree(entries), true);
    }

}