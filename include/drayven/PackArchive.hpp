#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
namespace drayven { struct PackedFile { std::string path; std::vector<std::uint8_t> bytes; }; class PackArchive { public: static void packDirectory(const std::filesystem::path&,const std::filesystem::path&,std::string_view); static std::vector<PackedFile> read(const std::filesystem::path&,std::string_view); private: static void legacyCrypt(std::vector<std::uint8_t>&,std::string_view,std::uint64_t); }; }
