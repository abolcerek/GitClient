#include "object_store.hpp"
#include "sha1.hpp"

#include <cstring>
#include <fstream>
#include <array>
#include <filesystem>
#include <ios>
#include <stdexcept>
#include <vector>
#include <string>
#include <iostream>

namespace fs = std::filesystem;

namespace GitClient {
    std::vector<std::byte> read_file(const fs::path& file_path) {
        if (!fs::exists(file_path)) {
            throw std::runtime_error("File does not exist");
        }
        auto fileSize = fs::file_size(file_path);
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Error when attempting to read file");
        }
        std::vector<std::byte> buffer(fileSize);
        if (file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
            return buffer;
        }
        throw std::runtime_error("Error when reading file contents into buffer");
    }

    std::array<std::byte, GitClient::hash_size> hash_object(const fs::path& root, const fs::path& file_path, std::string type, bool write) {
        auto content = GitClient::read_file(file_path);
        std::string header = type + " " +  std::to_string(content.size());
        header.push_back('\0');
        std::vector<std::byte> buffer;
        buffer.resize(header.size() + content.size());
        std::memcpy(buffer.data(), header.data(), header.size());
        std::memcpy(buffer.data() + header.size(), content.data(), content.size());
        auto hashedObject = GitClient::sha1(buffer);
        if (write) {
            auto hex = GitClient::to_hex(hashedObject);
            auto dir = hex.substr(0, 2);
            auto filename = hex.substr(2);
            auto path = root / fs::path("objects") / fs::path(dir) / fs::path(filename);
            if (!fs::exists(path)) {
                fs::create_directories(path.parent_path());
                std::ofstream outFile(path, std::ios::out | std::ios::binary);
                if (!outFile) {
                    throw std::runtime_error("Error when writing to object file");
                }
                outFile.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
                outFile.close();            
            }
        }
        return hashedObject;
    }
}