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
    std::vector<TreeEntry> parse_tree(const std::vector<std::byte>& payload);
    bool tree_entry_less(const TreeEntry&, const TreeEntry&);
    void string_to_bytes(std::vector<std::byte>& res, std::string_view input);
    std::vector<std::byte> serialize_tree(const std::vector<TreeEntry>& entries);
    std::array<std::byte, hash_size> write_tree(const std::filesystem::path& root, const std::filesystem::path& dir);
    class GitObject { 
        public:
        virtual ~GitObject() = default;
        virtual std::string_view type() const = 0;
        virtual std::vector<std::byte> serialize() const = 0;
    };

    class Blob : public GitObject {
        private:
        std::vector<std::byte> content_;
        public:
        explicit Blob(std::vector<std::byte> content);
        std::string_view type() const override;
        std::vector<std::byte> serialize() const override;
    };
        class Tree : public GitObject {
        private:
        std::vector<TreeEntry> entries_;
        public:
        explicit Tree(std::vector<TreeEntry> entries);
        std::string_view type() const override;
        std::vector<std::byte> serialize() const override;
    };
        class Commit : public GitObject {
        private:
        std::vector<std::byte> content_;
        public:
        explicit Commit(std::vector<std::byte> content);
        std::string_view type() const override;
        std::vector<std::byte> serialize() const override;
    };
}