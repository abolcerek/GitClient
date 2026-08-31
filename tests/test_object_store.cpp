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
    auto digest = GitClient::hash_object(tmp, input.string(), false);
    auto hex = "ce013625030ba8dba906f756967f9e9ca394464a"
    CHECK(GitClient::to_hex(digest) == hex);
    CHECK(fs::exists(tmp / "objects" / hex.substr(0, 2) / hex.substr(2)));
    std::ifstream in_file(tmp / "objects" / hex.substr(0, 2) / hex.substr(2), std::ios::binary);
    
}