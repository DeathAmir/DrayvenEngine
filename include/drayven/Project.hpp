#pragma once
#include <filesystem>
#include <string>
namespace drayven { enum class ProjectKind { Game2D, Game3D }; struct ProjectConfig { std::string name{"Untitled"}; ProjectKind kind{ProjectKind::Game2D}; std::filesystem::path root; std::filesystem::path startupScene{"Scenes/Main.dscene"}; std::string assetKey{"change-me"}; int width{1280},height{720}; std::filesystem::path androidSdk,androidNdk,luaJit,upx; std::string scriptBackend{"native"}; }; class Project { public: static ProjectConfig create(const std::filesystem::path&,std::string,ProjectKind); static ProjectConfig load(const std::filesystem::path&); static void save(const ProjectConfig&); static std::filesystem::path filePath(const ProjectConfig&); }; }
