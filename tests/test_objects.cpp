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

TEST_CASE("Git Object class correctlty works") {
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
    auto obj = GitClient::read_object(tmp, hex);
    CHECK(obj->type() == "blob");
    CHECK(obj->serialize() == payload);
}
TEST_CASE("commit round-trips with no parents") {
    GitClient::CommitData original{
        .tree = "0123456789abcdef0123456789abcdef01234567",
        .parents = {},
        .author = "Alice <alice@example.com> 1234567890 -0400",
        .committer = "Alice <alice@example.com> 1234567890 -0400",
        .message = "initial commit"
    };

    auto serialized = GitClient::serialize_commit(original);
    auto parsed = GitClient::parse_commit(serialized);

    CHECK(parsed.tree == original.tree);
    CHECK(parsed.parents == original.parents);
    CHECK(parsed.author == original.author);
    CHECK(parsed.committer == original.committer);
    CHECK(parsed.message == original.message);
}

TEST_CASE("commit round-trips with one parent") {
    GitClient::CommitData original{
        .tree = "0123456789abcdef0123456789abcdef01234567",
        .parents = {
            "1111111111111111111111111111111111111111"
        },
        .author = "Alice <alice@example.com> 1234567890 -0400",
        .committer = "Alice <alice@example.com> 1234567890 -0400",
        .message = "second commit"
    };

    auto serialized = GitClient::serialize_commit(original);
    auto parsed = GitClient::parse_commit(serialized);

    CHECK(parsed.tree == original.tree);
    CHECK(parsed.parents == original.parents);
    CHECK(parsed.author == original.author);
    CHECK(parsed.committer == original.committer);
    CHECK(parsed.message == original.message);
}

TEST_CASE("commit round-trips with two parents") {
    GitClient::CommitData original{
        .tree = "0123456789abcdef0123456789abcdef01234567",
        .parents = {
            "1111111111111111111111111111111111111111",
            "2222222222222222222222222222222222222222"
        },
        .author = "Alice <alice@example.com> 1234567890 -0400",
        .committer = "Alice <alice@example.com> 1234567890 -0400",
        .message = "merge commit"
    };

    auto serialized = GitClient::serialize_commit(original);
    auto parsed = GitClient::parse_commit(serialized);

    CHECK(parsed.tree == original.tree);
    CHECK(parsed.parents == original.parents);
    CHECK(parsed.author == original.author);
    CHECK(parsed.committer == original.committer);
    CHECK(parsed.message == original.message);
}

TEST_CASE("commit round-trips multi-line message") {
    GitClient::CommitData original{
        .tree = "0123456789abcdef0123456789abcdef01234567",
        .parents = {},
        .author = "Alice <alice@example.com> 1234567890 -0400",
        .committer = "Alice <alice@example.com> 1234567890 -0400",
        .message = "line1\nline2\n\nline4"
    };

    auto serialized = GitClient::serialize_commit(original);
    auto parsed = GitClient::parse_commit(serialized);

    CHECK(parsed.tree == original.tree);
    CHECK(parsed.parents == original.parents);
    CHECK(parsed.author == original.author);
    CHECK(parsed.committer == original.committer);
    CHECK(parsed.message == original.message);
}

TEST_CASE("commit round-trips empty message") {
    GitClient::CommitData original{
        .tree = "0123456789abcdef0123456789abcdef01234567",
        .parents = {},
        .author = "Alice <alice@example.com> 1234567890 -0400",
        .committer = "Alice <alice@example.com> 1234567890 -0400",
        .message = ""
    };

    auto serialized = GitClient::serialize_commit(original);
    auto parsed = GitClient::parse_commit(serialized);

    CHECK(parsed.tree == original.tree);
    CHECK(parsed.parents == original.parents);
    CHECK(parsed.author == original.author);
    CHECK(parsed.committer == original.committer);
    CHECK(parsed.message == original.message);
}

TEST_CASE("parse_commit throws on missing blank line") {
    const std::string payload =
        "tree 0123456789abcdef0123456789abcdef01234567\n"
        "author Alice <alice@example.com> 1234567890 -0400\n"
        "committer Alice <alice@example.com> 1234567890 -0400\n";

    std::vector<std::byte> bytes;
    GitClient::string_to_bytes(bytes, payload);

    CHECK_THROWS(GitClient::parse_commit(bytes));
}

