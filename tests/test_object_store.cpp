#include <doctest/doctest.h>
#include <fstream>

#include "GitClient/object_store.hpp"
#include "GitClient/sha1.hpp"

namespace fs = std::filesystem;

TEST_CASE("object store correctly hashes") {
    const fs::path tmp = fs::temp_directory_path() / "mygit_object_test";
    fs::remove_all(tmp);
    fs::create_directories(tmp);
    auto input = tmp / "hello.txt";
    std::ofstream out_file(input);
    REQUIRE(out_file.is_open());
    out_file << "hello" << "\n";
    out_file.close();
    auto digest = GitClient::hash_object(tmp, input.string(), true);
    auto hex = "ce013625030ba8dba906f756967f9e9ca394464a";
    CHECK(GitClient::to_hex(digest) == hex);
    const fs::path object_path = tmp / "objects" / "ce" / "013625030ba8dba906f756967f9e9ca394464a";
    CHECK(fs::exists(object_path));
    const std::vector<std::byte> test = {std::byte{'b'}, std::byte{'l'}, std::byte{'o'}, std::byte{'b'}, std::byte{' '}, std::byte{'6'}, std::byte{'\0'}, std::byte{'h'}, std::byte{'e'}, std::byte{'l'}, std::byte{'l'}, std::byte{'o'}, std::byte{'\n'}};
    std::ifstream in_file(object_path, std::ios::binary);
    REQUIRE(in_file.is_open());
    std::vector<std::byte> buffer(test.size());
    in_file.read(reinterpret_cast<char*>(buffer.data()), test.size());
    CHECK(buffer == test);

    
}