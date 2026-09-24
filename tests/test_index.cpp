#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <map>
#include <string>

#include "GitClient/index.hpp"
#include "GitClient/objects.hpp"

namespace fs = std::filesystem;
using namespace GitClient;

namespace {
    constexpr std::string_view index_file = "mygit-index";

    // Creates a temporary test directory.
    fs::path make_temp_dir(const std::string& name) {
        auto dir = fs::temp_directory_path() / ("gitclient_index_" + name);
        fs::remove_all(dir);
        fs::create_directories(dir);
        return dir;
    }

    void write_file(const fs::path& path, const std::string& contents) {
        fs::create_directories(path.parent_path());
        std::ofstream file(path);
        REQUIRE(file.is_open());
        file << contents;
    }

    void cleanup(const fs::path& dir) {
        fs::remove_all(dir);
    }
}

TEST_CASE("read_index returns empty map when index does not exist") {
    auto git_dir = make_temp_dir("read_missing");

    auto result = read_index(git_dir);

    CHECK(result.empty());

    cleanup(git_dir);
}

TEST_CASE("write_index creates and writes an index") {
    auto git_dir = make_temp_dir("write");

    std::map<std::string, IndexEntry> entries{
        {"file.txt", {"100644", "abc123"}},
        {"script.sh", {"100755", "def456"}}
    };

    write_index(git_dir, entries);

    CHECK(fs::exists(git_dir / index_file));

    auto result = read_index(git_dir);

    REQUIRE(result.size() == 2);
    CHECK(result.at("file.txt").mode == "100644");
    CHECK(result.at("file.txt").hex == "abc123");
    CHECK(result.at("script.sh").mode == "100755");
    CHECK(result.at("script.sh").hex == "def456");

    cleanup(git_dir);
}

TEST_CASE("read_index correctly parses multiple entries") {
    auto git_dir = make_temp_dir("read_multiple");

    std::ofstream file(git_dir / index_file);
    REQUIRE(file.is_open());

    file << "100644 abc123 file.txt\n";
    file << "100755 def456 script.sh\n";
    file << "100644 789abc src/main.cpp\n";
    file.close();

    auto result = read_index(git_dir);

    REQUIRE(result.size() == 3);

    CHECK(result.at("file.txt").mode == "100644");
    CHECK(result.at("file.txt").hex == "abc123");

    CHECK(result.at("script.sh").mode == "100755");
    CHECK(result.at("script.sh").hex == "def456");

    CHECK(result.at("src/main.cpp").mode == "100644");
    CHECK(result.at("src/main.cpp").hex == "789abc");

    cleanup(git_dir);
}

TEST_CASE("read_index handles paths containing spaces") {
    auto git_dir = make_temp_dir("read_spaces");

    std::ofstream file(git_dir / index_file);
    REQUIRE(file.is_open());

    file << "100644 abc123 path with spaces.txt\n";
    file.close();

    auto result = read_index(git_dir);

    REQUIRE(result.size() == 1);
    CHECK(result.at("path with spaces.txt").mode == "100644");
    CHECK(result.at("path with spaces.txt").hex == "abc123");

    cleanup(git_dir);
}

TEST_CASE("read_index uses the last entry for duplicate paths") {
    auto git_dir = make_temp_dir("read_duplicate");

    std::ofstream file(git_dir / index_file);
    REQUIRE(file.is_open());

    file << "100644 first_hash file.txt\n";
    file << "100755 second_hash file.txt\n";
    file.close();

    auto result = read_index(git_dir);

    REQUIRE(result.size() == 1);
    CHECK(result.at("file.txt").mode == "100755");
    CHECK(result.at("file.txt").hex == "second_hash");

    cleanup(git_dir);
}

TEST_CASE("write_index overwrites an existing index") {
    auto git_dir = make_temp_dir("write_overwrite");

    {
        std::map<std::string, IndexEntry> entries{
            {"old.txt", {"100644", "oldhash"}}
        };

        write_index(git_dir, entries);
    }

    {
        std::map<std::string, IndexEntry> entries{
            {"new.txt", {"100644", "newhash"}}
        };

        write_index(git_dir, entries);
    }

    auto result = read_index(git_dir);

    REQUIRE(result.size() == 1);
    CHECK(result.count("old.txt") == 0);
    CHECK(result.at("new.txt").hex == "newhash");

    cleanup(git_dir);
}

TEST_CASE("write_index throws when git directory does not exist") {
    auto git_dir =
        fs::temp_directory_path() / "gitclient_index_nonexistent";

    fs::remove_all(git_dir);

    std::map<std::string, IndexEntry> entries{
        {"file.txt", {"100644", "abc123"}}
    };

    CHECK_THROWS_AS(
        write_index(git_dir, entries),
        std::runtime_error
    );
}

TEST_CASE("write_index handles an empty map") {
    auto git_dir = make_temp_dir("write_empty");

    std::map<std::string, IndexEntry> entries;

    write_index(git_dir, entries);

    CHECK(fs::exists(git_dir / index_file));

    auto result = read_index(git_dir);

    CHECK(result.empty());

    cleanup(git_dir);
}

TEST_CASE("add adds a regular file to the index") {
    auto root = make_temp_dir("add_file");
    auto git_dir = root / ".git";
    auto worktree = root;

    fs::create_directories(git_dir);

    auto file = worktree / "hello.txt";
    write_file(file, "hello world");

    add(git_dir, worktree, file);

    auto result = read_index(git_dir);

    REQUIRE(result.size() == 1);
    REQUIRE(result.count("hello.txt") == 1);

    CHECK(result.at("hello.txt").mode == "100644");
    CHECK_FALSE(result.at("hello.txt").hex.empty());

    cleanup(root);
}

TEST_CASE("add stores executable files with mode 100755") {
    auto root = make_temp_dir("add_executable");
    auto git_dir = root / ".git";
    auto worktree = root;

    fs::create_directories(git_dir);

    auto file = worktree / "script.sh";
    write_file(file, "#!/bin/sh\necho hello\n");

    fs::permissions(
        file,
        fs::perms::owner_exec,
        fs::perm_options::add
    );

    add(git_dir, worktree, file);

    auto result = read_index(git_dir);

    REQUIRE(result.size() == 1);
    REQUIRE(result.count("script.sh") == 1);

    CHECK(result.at("script.sh").mode == "100755");
    CHECK_FALSE(result.at("script.sh").hex.empty());

    cleanup(root);
}

TEST_CASE("add stores non-executable files with normal file mode") {
    auto root = make_temp_dir("add_non_executable");
    auto git_dir = root / ".git";
    auto worktree = root;

    fs::create_directories(git_dir);

    auto file = worktree / "data.txt";
    write_file(file, "some data");

    fs::permissions(
        file,
        fs::perms::owner_exec,
        fs::perm_options::remove
    );

    add(git_dir, worktree, file);

    auto result = read_index(git_dir);

    REQUIRE(result.count("data.txt") == 1);
    CHECK(result.at("data.txt").mode == "100644");

    cleanup(root);
}

TEST_CASE("add uses path relative to worktree") {
    auto root = make_temp_dir("add_relative");
    auto git_dir = root / ".git";
    auto worktree = root;

    fs::create_directories(git_dir);

    auto file = worktree / "src" / "main.cpp";
    write_file(file, "int main() { return 0; }");

    add(git_dir, worktree, file);

    auto result = read_index(git_dir);

    REQUIRE(result.size() == 1);
    CHECK(result.count("src/main.cpp") == 1);

    cleanup(root);
}

TEST_CASE("add recursively adds files in a directory") {
    auto root = make_temp_dir("add_directory");
    auto git_dir = root / ".git";
    auto worktree = root;

    fs::create_directories(git_dir);

    write_file(worktree / "a.txt", "a");
    write_file(worktree / "b.txt", "b");
    write_file(worktree / "src" / "main.cpp", "main");
    write_file(worktree / "src" / "util.cpp", "util");

    add(git_dir, worktree, worktree);

    auto result = read_index(git_dir);

    CHECK(result.size() == 4);

    CHECK(result.count("a.txt") == 1);
    CHECK(result.count("b.txt") == 1);
    CHECK(result.count("src/main.cpp") == 1);
    CHECK(result.count("src/util.cpp") == 1);

    cleanup(root);
}

TEST_CASE("add skips .git directory") {
    auto root = make_temp_dir("add_skip_git");
    auto git_dir = root / ".git";
    auto worktree = root;

    fs::create_directories(git_dir);

    write_file(worktree / "tracked.txt", "tracked");

    // The index now lives at .git/mygit-index.
    write_file(git_dir / index_file, "existing content");
    write_file(git_dir / "should_not_be_added.txt", "internal");

    add(git_dir, worktree, worktree);

    auto result = read_index(git_dir);

    CHECK(result.count("tracked.txt") == 1);
    CHECK(result.count(".git/mygit-index") == 0);
    CHECK(result.count(".git/should_not_be_added.txt") == 0);

    cleanup(root);
}

TEST_CASE("add preserves existing index entries") {
    auto root = make_temp_dir("add_preserve");
    auto git_dir = root / ".git";
    auto worktree = root;

    fs::create_directories(git_dir);

    std::map<std::string, IndexEntry> existing{
        {"existing.txt", {"100644", "existinghash"}}
    };

    write_index(git_dir, existing);

    auto file = worktree / "new.txt";
    write_file(file, "new");

    add(git_dir, worktree, file);

    auto result = read_index(git_dir);

    REQUIRE(result.size() == 2);

    CHECK(result.at("existing.txt").mode == "100644");
    CHECK(result.at("existing.txt").hex == "existinghash");

    CHECK(result.count("new.txt") == 1);

    cleanup(root);
}

TEST_CASE("add updates an existing index entry") {
    auto root = make_temp_dir("add_update");
    auto git_dir = root / ".git";
    auto worktree = root;

    fs::create_directories(git_dir);

    auto file = worktree / "file.txt";
    write_file(file, "original");

    add(git_dir, worktree, file);

    auto first = read_index(git_dir);

    REQUIRE(first.count("file.txt") == 1);
    auto first_hash = first.at("file.txt").hex;

    write_file(file, "modified");

    add(git_dir, worktree, file);

    auto second = read_index(git_dir);

    REQUIRE(second.size() == 1);
    REQUIRE(second.count("file.txt") == 1);

    CHECK(second.at("file.txt").mode == "100644");
    CHECK_FALSE(second.at("file.txt").hex.empty());
    CHECK(second.at("file.txt").hex != first_hash);

    cleanup(root);
}

TEST_CASE("write_tree and write_tree_from_index agree on a nested worktree") {
    auto root = make_temp_dir("tree_agreement");
    auto git_dir = root / ".git";
    auto worktree = root;

    fs::create_directories(git_dir);

    write_file(worktree / "a.txt", "a");
    write_file(worktree / "sub" / "b.txt", "b");
    write_file(worktree / "sub" / "deep" / "c.txt", "c");

    // Build a tree directly from the worktree.
    auto digest_a = write_tree(git_dir, worktree);

    // Build the index from exactly the same worktree.
    add(git_dir, worktree, worktree);

    // Build a tree from the resulting index.
    auto index = read_index(git_dir);
    auto digest_b = write_tree_from_index(git_dir, index);

    CHECK(digest_a == digest_b);

    cleanup(root);
}

TEST_CASE("write_tree_from_index creates a tree for a single file") {
    auto root = make_temp_dir("write_tree_single");
    auto git_dir = root / ".git";

    fs::create_directories(git_dir);

    write_file(root / "file.txt", "hello");

    add(git_dir, root, root / "file.txt");

    auto index = read_index(git_dir);
    auto tree = write_tree_from_index(git_dir, index);

    CHECK(tree != std::array<std::byte, GitClient::hash_size>{});

    cleanup(root);
}

TEST_CASE("write_tree_from_index creates nested directory trees") {
    auto root = make_temp_dir("write_tree_nested");
    auto git_dir = root / ".git";

    fs::create_directories(git_dir);

    write_file(root / "a.txt", "a");
    write_file(root / "sub" / "b.txt", "b");
    write_file(root / "sub" / "deep" / "c.txt", "c");

    add(git_dir, root, root);

    auto index = read_index(git_dir);

    REQUIRE(index.size() == 3);
    CHECK(index.count("a.txt") == 1);
    CHECK(index.count("sub/b.txt") == 1);
    CHECK(index.count("sub/deep/c.txt") == 1);

    auto tree = write_tree_from_index(git_dir, index);

    CHECK(tree != std::array<std::byte, GitClient::hash_size>{});

    cleanup(root);
}

TEST_CASE("write_tree_from_index agrees with write_tree for multiple files") {
    auto root = make_temp_dir("write_tree_multiple");
    auto git_dir = root / ".git";

    fs::create_directories(git_dir);

    write_file(root / "a.txt", "aaa");
    write_file(root / "b.txt", "bbb");
    write_file(root / "src" / "main.cpp", "main");
    write_file(root / "src" / "util.cpp", "util");

    auto expected = write_tree(git_dir, root);

    add(git_dir, root, root);

    auto index = read_index(git_dir);
    auto actual = write_tree_from_index(git_dir, index);

    CHECK(actual == expected);

    cleanup(root);
}

TEST_CASE("write_tree_from_index preserves executable file modes") {
    auto root = make_temp_dir("write_tree_modes");
    auto git_dir = root / ".git";

    fs::create_directories(git_dir);

    write_file(root / "normal.txt", "normal");
    write_file(root / "script.sh", "#!/bin/sh\necho hello\n");

    fs::permissions(
        root / "script.sh",
        fs::perms::owner_exec,
        fs::perm_options::add
    );

    auto expected = write_tree(git_dir, root);

    add(git_dir, root, root);

    auto index = read_index(git_dir);

    REQUIRE(index.size() == 2);
    CHECK(index.at("normal.txt").mode == "100644");
    CHECK(index.at("script.sh").mode == "100755");

    auto actual = write_tree_from_index(git_dir, index);

    CHECK(actual == expected);

    cleanup(root);
}
