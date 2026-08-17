#pragma once
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>
namespace drayven {
struct CipherBlob { std::vector<std::uint8_t> nonce, data, tag; };
class Crypto {
public:
    static bool available();
    static CipherBlob encryptAes256Gcm(std::span<const std::uint8_t>, std::string_view);
    static std::vector<std::uint8_t> decryptAes256Gcm(const CipherBlob&, std::string_view);
};
}
