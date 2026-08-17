#pragma once
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace drayven {
struct PackedFile { std::string path; std::vector<std::uint8_t> bytes; };

class PackArchive {
public:
    static void packDirectory(const std::filesystem::path& input, const std::filesystem::path& output, std::string_view key);
    static std::vector<PackedFile> read(const std::filesystem::path& archive, std::string_view key);
private:
    static void crypt(std::vector<std::uint8_t>& data, std::string_view key, std::uint64_t nonce);
};
}
