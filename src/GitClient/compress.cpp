#include "compress.hpp"

#include <zlib.h>
#include <stdexcept>
#include <vector>
#include <zconf.h>

namespace GitClient {
    std::vector<std::byte> zlib_compress(const std::vector<std::byte>& payload) {
        auto sourceLen = payload.size();
        auto destLen = compressBound(sourceLen);
        std::vector<std::byte> buffer(destLen);
        auto res = compress(reinterpret_cast<Bytef*>(buffer.data()), &destLen, reinterpret_cast<const Bytef*>(payload.data()), sourceLen);
        if (res != Z_OK) {
            throw std::runtime_error("Error when compressing the payload");
        }
        buffer.resize(destLen);
        return buffer;
    }
    std::vector<std::byte> zlib_decompress(const std::vector<std::byte>& payload) {
        auto size = payload.size() * 4 + 64;
        std::vector<std::byte> buffer(size);
        while (true) {
            auto destLen = buffer.size();
            auto res = uncompress(reinterpret_cast<Bytef*>(buffer.data()), &destLen, reinterpret_cast<const Bytef*>(payload.data()), payload.size());
            if (res == Z_OK) {
                buffer.resize(destLen);
                return buffer;
            } 
            if (res == Z_BUF_ERROR) {
                size *= 2;
                buffer.resize(size);
                continue;
            } 
            throw std::runtime_error("Error when uncompressing the payload");
        }
    }
}