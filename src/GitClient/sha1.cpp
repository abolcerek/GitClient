#include "sha1.hpp"

#include <array>
#include <vector>
#include <cstdint>
#include <cstring>
#include <string>
#include <bit>

namespace GitClient {
    std::string to_hex(const std::array<std::byte, GitClient::hash_size>& data) {
        std::string hexString = "";
            for (auto b : data) {
            auto highNibble = static_cast<int>(b >> 4);
            auto lowNibble = static_cast<int>(b & std::byte{15});
            hexString = hexString + GitClient::hexDigits[highNibble] + GitClient::hexDigits[lowNibble];
        }
        return hexString;
    }

    std::array<std::byte, GitClient::hash_size> sha1(const std::vector<std::byte>& data) {
        uint32_t H0 = 0x67452301;
        uint32_t H1 = 0xEFCDAB89;
        uint32_t H2 = 0x98BADCFE;
        uint32_t H3 = 0x10325476;
        uint32_t H4 = 0xC3D2E1F0;
        uint64_t file_length = data.size() * 8;
        auto padding = data;
        padding.push_back(std::byte{0x80});
        auto padding_required = (56 - padding.size()) % 64;
        padding.insert(padding.end(), padding_required, std::byte{0});
        if constexpr (std::endian::native == std::endian::little) { // big endian conversion
            file_length = std::byteswap(data.size() * 8);
        }
        const std::byte* bytes = reinterpret_cast<const std::byte*>(&file_length);
        padding.insert(padding.end(), bytes, bytes + sizeof(file_length));
        for (auto it = padding.begin(); it != padding.end(); std::advance(it, 64)) { // for each 512 bit chunk in file contents
            auto slice = std::ranges::subrange(it, it + 64); // 512-bit chunk
            std::array<uint32_t, GitClient::word_length> words;
            size_t i = 0;
            for (auto inner_it = slice.begin(); inner_it != slice.end(); std::advance(inner_it, 4)) { // for each 32 bit word in the chunk
                uint32_t word = 0;
                std::memcpy(&word, &*inner_it, sizeof(uint32_t));
                if constexpr (std::endian::native == std::endian::little) {
                    word = std::byteswap(word);
                }
                words[i] = word;
                i++;
            }
            for (auto j = 16; j < 80; j++) {
                auto val = (words[i - 3] ^ words[i - 8] ^ words[i - 14] ^ words[i - 16]);
                words[i] = std::rotl(val, 1);
                i++;
            }
            auto a = H0;
            auto b = H1;
            auto c = H2;
            auto d = H3;
            auto e = H4;
            uint32_t f;
            uint32_t k;
            for (size_t index = 0; index < GitClient::word_length; index++) {
                if (index <= 19) {
                    f = (b & c) | (~b & d);
                    k = 0x5A827999;
                }
                else if (index <= 39) {
                    f = b ^ c ^ d;
                    k = 0x6ED9EBA1;
                }
                else if (index <= 59) {
                    f = (b & c) | (b & d) | (c & d);
                    k = 0x8F1BBCDC;
                }
                else if (index <= 79) {
                    f = b ^ c ^ d;
                    k = 0xCA62C1D6;
                }
                auto temp = std::rotl(a, 5) + f + e + k + words[index];
                e = d;
                d = c;
                c = std::rotl(b, 30);
                b = a;
                a = temp;
            }
            H0 += a;
            H1 += b;
            H2 += c;
            H3 += d;
            H4 += e;
        }
        if constexpr (std::endian::native == std::endian::little) {
            H0 = std::byteswap(H0);
            H1 = std::byteswap(H1);
            H2 = std::byteswap(H2);
            H3 = std::byteswap(H3);
            H4 = std::byteswap(H4);
        }
        std::array<std::byte, GitClient::hash_size> res;
        std::memcpy(&res[0], &H0, sizeof(H0));
        std::memcpy(&res[4], &H1, sizeof(H1));
        std::memcpy(&res[8], &H2, sizeof(H2));
        std::memcpy(&res[12], &H3, sizeof(H3));
        std::memcpy(&res[16], &H4, sizeof(H4));
        return res;
    }
}