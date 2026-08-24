#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <filesystem>

#include "GitClient/repository.hpp"

namespace fs = std::filesystem;

TEST_CASE("init creates repository skeleton") {
    const fs::path tmp = fs::temp_directory_path() / "mygit_test_init";
    fs::remove_all(tmp);
    fs::create_directories(tmp);

    CHECK(GitClient::init_repository(tmp));
    CHECK(fs::exists(tmp / ".git" / "objects"));
    CHECK(fs::exists(tmp / ".git" / "refs" / "heads"));
    CHECK(fs::exists(tmp / ".git" / "HEAD"));

    // second init on the same path must fail
    CHECK_FALSE(GitClient::init_repository(tmp));

    fs::remove_all(tmp);
}