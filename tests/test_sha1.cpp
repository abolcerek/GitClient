#include <doctest/doctest.h>

#include "GitClient/sha1.hpp"


TEST_CASE("sha1 correctly hashes") {
    std::string str1 = "abc";
    std::string str2 = "";
    std::string str3 = "The quick brown fox jumps over the lazy dog";
    std::string str4 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    auto* start1 = reinterpret_cast<const std::byte*>(str1.data());
    auto* start2 = reinterpret_cast<const std::byte*>(str2.data());
    auto* start3 = reinterpret_cast<const std::byte*>(str3.data());
    auto* start4 = reinterpret_cast<const std::byte*>(str4.data());
    CHECK(GitClient::to_hex(GitClient::sha1(std::vector<std::byte>(start1, start1 + str1.size()))) == "a9993e364706816aba3e25717850c26c9cd0d89d");
    CHECK(GitClient::to_hex(GitClient::sha1(std::vector<std::byte>(start2, start2 + str2.size()))) == "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    CHECK(GitClient::to_hex(GitClient::sha1(std::vector<std::byte>(start3, start3 + str3.size()))) == "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12");
    CHECK(GitClient::to_hex(GitClient::sha1(std::vector<std::byte>(start4, start4 + str4.size()))) == "e61cfffe0d9195a525fc6cf06ca2d77119c24a40");
}