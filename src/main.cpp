#include "GitClient/repository.hpp"
#include "GitClient/objects.hpp"
#include "GitClient/object_store.hpp"
#include "GitClient/sha1.hpp"
#include "GitClient/index.hpp"


#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <chrono>
#include <format>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::exit(1);
    }
    const std::string_view first_arg = argv[1];
    if (first_arg == "init") {
        if (argc != 2) {
            std::exit(1);
        }
        if (GitClient::init_repository(fs::current_path())) {
            std::cout << "Success\n";
            return 0;
        } 
        else {
            std::cout << "Failure\n";
            return 1;
        }
    }
    if (first_arg == "write-tree") {
        if (argc != 2) {
            std::exit(1);
        }
        try {
            auto digest = GitClient::write_tree(fs::current_path() / ".git", fs::current_path());
            std::cout << GitClient::to_hex(digest) << "\n";
            return 0;
        }
        catch (const std::exception& e) {
            std::cerr << e.what();
            return 1;
        }
    }
    if (first_arg == "hash-object") { // ignoring -w flag for now
        if (argc != 3) {
            std::exit(1);
        }
        const std::string_view second_arg = argv[2];
        try {
            auto hash = GitClient::hash_object(fs::current_path() / ".git", second_arg, true);
            std::cout << GitClient::to_hex(hash) << "\n";
            return 0;
        }
        catch (const std::exception& e) {
            std::cerr << e.what();
            return 1;
        }
    }
    if (first_arg == "cat-file") { // ignoring -p flag for now
        if (argc != 3) {
            std::exit(1);
        }
        const std::string_view second_arg = argv[2];
        try {
            auto [_, content] = GitClient::read_object_raw(fs::current_path() / ".git", second_arg);
            std::cout.write(reinterpret_cast<const char*>(content.data()), content.size());
            return 0;
        }
        catch (const std::exception& e) {
            std::cerr << e.what();
            return 1;
        }
    }
    if (first_arg == "ls-tree") {
        if (argc != 3) {
            std::exit(1);
        }
        const std::string_view second_arg = argv[2];
        try {
            auto [header, content] = GitClient::read_object_raw(fs::current_path() / ".git", second_arg);
            if (header != "tree") {
                std::cout << "Error: type is not tree";
                return 1;
            }
            auto digest = GitClient::parse_tree(content);
            for (auto& entry : digest) {
                std::string type;
                if (entry.mode == GitClient::directory) {
                    type = "tree";
                    entry.mode = '0' + entry.mode;
                } else if (entry.mode == GitClient::normal_file) {
                    type = "blob";
                } else {
                    throw std::runtime_error("Unknown object type");
                }
                std::cout << entry.mode << " " << type << " " << GitClient::to_hex(entry.hash) << "\t" << entry.name << "\n";
            }
            return 0;
        }
        catch (const std::exception& e) {
            std::cerr << e.what();
            return 1;
        }
    }
    if (first_arg == "commit") {
        if (argc != 4) {
            std::exit(1);
        }
        const std::string_view second_arg = argv[2];
        const std::string_view third_arg = argv[3];
        if (second_arg != "-m") {
            std::cout << "Incorrect flag";
            return 1;
        }
        try {
            const auto git_dir = fs::current_path() / ".git";
            auto hash = GitClient::write_tree(git_dir, fs::current_path());
            auto head = GitClient::resolve_head(git_dir);
            GitClient::CommitData data;
            data.tree = GitClient::to_hex(hash);
            if (head.has_value()) {
                data.parents = {*head};
            } else {
                data.parents = {};
            }
            const std::string identity = "Jane Doe <janedoe@example.com>";
            auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
            data.author    = std::format("{} {} -0400", identity, seconds);
            data.committer = data.author;
            data.message = std::string(third_arg);
            auto commit = GitClient::serialize_commit(data);
            auto digest = GitClient::write_record(git_dir, "commit", commit, true);
            GitClient::update_ref(git_dir, GitClient::to_hex(digest));
            std::cout << GitClient::to_hex(digest) << "\n";
            return 0;
        }
        catch (const std::exception& e) {
            std::cerr << e.what();
            return 1;
        }
    }
    if (first_arg == "add") {
        if (argc != 3) {
            std::exit(1);
        }
        const std::string_view second_arg = argv[2];
        try {
            GitClient::add(fs::current_path() / ".git", fs::current_path(), second_arg);
            return 0;
        }
        catch (const std::exception& e) {
            std::cerr << e.what();
            return 1;
        }
    }
    std::cerr << "Unknown command";
    return 1;
}