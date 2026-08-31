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
}