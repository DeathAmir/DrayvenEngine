#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace drayven {
enum class ProjectKind { Game2D, Game3D };

struct ProjectConfig {
    std::string name{"Untitled"};
    ProjectKind kind{ProjectKind::Game2D};
    std::filesystem::path root;
    std::filesystem::path startupScene{"Scenes/Main.dscene"};
    std::string assetKey{"change-me"};
    int width{1280};
    int height{720};
};

class Project {
public:
    static ProjectConfig create(const std::filesystem::path& root, std::string name, ProjectKind kind);
    static ProjectConfig load(const std::filesystem::path& file);
    static void save(const ProjectConfig& config);
    static std::filesystem::path filePath(const ProjectConfig& config);
};
}
