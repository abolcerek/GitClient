#include <doctest/doctest.h>
#include <fstream>
#include <algorithm>

#include "GitClient/object_store.hpp"
#include "GitClient/sha1.hpp"
#include "GitClient/objects.hpp"

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
    auto [header, payload] = GitClient::read_object_raw(tmp, hex);
    const std::vector<std::byte> test = {std::byte{'h'}, std::byte{'e'}, std::byte{'l'}, std::byte{'l'}, std::byte{'o'}, std::byte{'\n'}};
    CHECK(header == "blob");
    CHECK(payload == test);
    
}

