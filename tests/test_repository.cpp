#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "GitClient/repository.hpp"
#include "GitClient/objects.hpp"

namespace fs = std::filesystem;

namespace {

fs::path make_test_repo(const std::string& name) {
    const fs::path tmp = fs::temp_directory_path() / name;
    fs::remove_all(tmp);
    fs::create_directories(tmp);

    REQUIRE(GitClient::init_repository(tmp));

    return tmp / ".git";
}

} // namespace

TEST_CASE("init creates repository skeleton") {
    const fs::path tmp = fs::temp_directory_path() / "mygit_test_init";
    fs::remove_all(tmp);
    fs::create_directories(tmp);

    CHECK(GitClient::init_repository(tmp));
    CHECK(fs::exists(tmp / ".git" / "objects"));
    CHECK(fs::exists(tmp / ".git" / "refs" / "heads"));
    CHECK(fs::exists(tmp / ".git" / "HEAD"));

    CHECK_FALSE(GitClient::init_repository(tmp));

    fs::remove_all(tmp);
}

TEST_CASE("resolve_head returns hash from HEAD reference") {
    const fs::path git = make_test_repo("mygit_test_resolve_head");

    const std::string hash = "0123456789abcdef0123456789abcdef01234567";
    std::ofstream(git / "refs" / "heads" / "main") << hash << "\n";

    CHECK(GitClient::resolve_head(git) == hash);

    fs::remove_all(git.parent_path());
}

TEST_CASE("resolve_head removes trailing newline") {
    const fs::path git = make_test_repo("mygit_test_resolve_head_newline");

    const std::string hash = "0123456789abcdef0123456789abcdef01234567";
    std::ofstream(git / "refs" / "heads" / "main") << hash << "\n";

    CHECK(GitClient::resolve_head(git) == hash);

    fs::remove_all(git.parent_path());
}

TEST_CASE("resolve_head returns nullopt when reference file does not exist") {
    const fs::path git = make_test_repo("mygit_test_resolve_head_missing");

    CHECK_FALSE(GitClient::resolve_head(git).has_value());

    fs::remove_all(git.parent_path());
}

TEST_CASE("resolve_head throws when hash is not 40 characters") {
    const fs::path git = make_test_repo("mygit_test_resolve_head_bad_hash");

    std::ofstream(git / "refs" / "heads" / "main")
        << "0123456789abcdef\n";

    CHECK_THROWS_AS(
        GitClient::resolve_head(git),
        std::runtime_error
    );

    fs::remove_all(git.parent_path());
}

TEST_CASE("resolve_head accepts a 40 character hash without newline") {
    const fs::path git = make_test_repo("mygit_test_resolve_head_no_newline");

    const std::string hash = "0123456789abcdef0123456789abcdef01234567";
    std::ofstream(git / "refs" / "heads" / "main") << hash;

    CHECK(GitClient::resolve_head(git) == hash);

    fs::remove_all(git.parent_path());
}

TEST_CASE("update_ref creates reference and writes hash") {
    const fs::path git = make_test_repo("mygit_test_update_ref");

    const std::string hash = "0123456789abcdef0123456789abcdef01234567";

    GitClient::update_ref(git, hash);

    CHECK(fs::exists(git / "refs" / "heads" / "main"));

    std::ifstream file(git / "refs" / "heads" / "main");
    std::string contents(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    CHECK(contents == hash + "\n");

    fs::remove_all(git.parent_path());
}

TEST_CASE("update_ref creates missing parent directories") {
    const fs::path git = make_test_repo("mygit_test_update_ref_dirs");

    // Change HEAD to point at a nested branch.
    std::ofstream(git / "HEAD")
        << "ref: refs/heads/feature/test\n";

    const std::string hash = "0123456789abcdef0123456789abcdef01234567";

    GitClient::update_ref(git, hash);

    CHECK(fs::exists(git / "refs" / "heads" / "feature" / "test"));

    std::ifstream file(git / "refs" / "heads" / "feature" / "test");
    std::string contents(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    CHECK(contents == hash + "\n");

    fs::remove_all(git.parent_path());
}

TEST_CASE("update_ref overwrites existing reference") {
    const fs::path git = make_test_repo("mygit_test_update_ref_overwrite");

    const std::string old_hash = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    const std::string new_hash = "0123456789abcdef0123456789abcdef01234567";

    std::ofstream(git / "refs" / "heads" / "main") << old_hash << "\n";

    GitClient::update_ref(git, new_hash);

    std::ifstream file(git / "refs" / "heads" / "main");
    std::string contents(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    CHECK(contents == new_hash + "\n");

    fs::remove_all(git.parent_path());
}

TEST_CASE("collect_history returns empty history for fresh repository") {
    const fs::path git = make_test_repo("mygit_test_collect_history_empty");

    auto history = GitClient::collect_history(git);

    CHECK(history.empty());

    fs::remove_all(git.parent_path());
}


TEST_CASE("collect_history returns commits newest-first with intact parent chain") {
    const fs::path git = make_test_repo("mygit_test_collect_history");

    // Create the first commit.
    GitClient::CommitData first;
    first.message = "first";

    auto first_payload = serialize_commit(first);
    auto first_hash = GitClient::write_record(
        git,
        "commit",
        first_payload,
        true
    );

    // Create the second commit with first as its parent.
    GitClient::CommitData second;
    second.message = "second";
    second.parents.push_back(GitClient::to_hex(first_hash));

    auto second_payload = serialize_commit(second);
    auto second_hash = GitClient::write_record(
        git,
        "commit",
        second_payload,
        true
    );

    // Create the third commit with second as its parent.
    GitClient::CommitData third;
    third.message = "third";
    third.parents.push_back(GitClient::to_hex(second_hash));

    auto third_payload = serialize_commit(third);
    auto third_hash = GitClient::write_record(
        git,
        "commit",
        third_payload,
        true
    );

    const auto first_hex = GitClient::to_hex(first_hash);
    const auto second_hex = GitClient::to_hex(second_hash);
    const auto third_hex = GitClient::to_hex(third_hash);

    // HEAD must point to the newest commit.
    GitClient::update_ref(git, third_hex);

    auto history = GitClient::collect_history(git);

    REQUIRE(history.size() == 3);

    // Newest-first ordering.
    CHECK(history[0].first == third_hex);
    CHECK(history[0].second.message == "third");

    CHECK(history[1].first == second_hex);
    CHECK(history[1].second.message == "second");

    CHECK(history[2].first == first_hex);
    CHECK(history[2].second.message == "first");

    // Parent chain integrity.
    REQUIRE(history[0].second.parents.size() == 1);
    CHECK(history[0].second.parents[0] == history[1].first);

    REQUIRE(history[1].second.parents.size() == 1);
    CHECK(history[1].second.parents[0] == history[2].first);

    CHECK(history[2].second.parents.empty());

    fs::remove_all(git.parent_path());
}


