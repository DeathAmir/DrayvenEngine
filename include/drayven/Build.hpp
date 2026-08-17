#pragma once
#include "drayven/Drys.hpp"
#include "drayven/Project.hpp"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
namespace drayven {
enum class BuildTarget { Desktop, Android };
enum class ScriptMode { Native, LuaJit };
struct BuildOptions {
    BuildTarget target{BuildTarget::Desktop}; ScriptMode scriptMode{ScriptMode::Native};
    std::filesystem::path engineRoot, sdkRoot, ndkRoot, luaJit, upx;
    std::string abi{"arm64-v8a"}; int androidApi{24}; bool hardenSymbols{true}; bool useUpx{false};
    std::uint64_t seed{0x4452415956454eULL};
};
struct BuildReport { bool ok{false}; std::filesystem::path output; std::vector<std::string> log; };
class BuildPipeline {
public:
    static BuildReport build(const ProjectConfig&, const BuildOptions&);
    static std::filesystem::path discoverNdk(const std::filesystem::path& sdkRoot);
    static std::filesystem::path discoverLuaJit(const std::filesystem::path& preferred = {});
};
}
