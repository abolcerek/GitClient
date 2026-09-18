#include <doctest/doctest.h>

#include "GitClient/compress.hpp"

#include <cstddef>
#include <stdexcept>
#include <vector>

TEST_CASE("zlib compression round-trips an empty vector") {
    const std::vector<std::byte> input;

    const auto compressed = GitClient::zlib_compress(input);
    const auto decompressed = GitClient::zlib_decompress(compressed);

    CHECK(decompressed == input);
}

TEST_CASE("zlib compression round-trips binary data") {
    const std::vector<std::byte> input{
        std::byte{0x00},
        std::byte{0x01},
        std::byte{0x7f},
        std::byte{0x80},
        std::byte{0xfe},
        std::byte{0xff},
        std::byte{0x00},
        std::byte{0xff},
    };

    const auto compressed = GitClient::zlib_compress(input);
    const auto decompressed = GitClient::zlib_decompress(compressed);

    CHECK(decompressed == input);
}

TEST_CASE("zlib decompression rejects garbage") {
    const std::vector<std::byte> garbage{
        std::byte{0xde},
        std::byte{0xad},
        std::byte{0xbe},
        std::byte{0xef},
    };

    CHECK_THROWS_AS(
        GitClient::zlib_decompress(garbage),
        std::runtime_error
    );
}
