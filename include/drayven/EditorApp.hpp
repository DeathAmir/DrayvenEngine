#pragma once
#include "drayven/Application.hpp"
#include "drayven/Localization.hpp"
#include "drayven/Project.hpp"
#include "drayven/Scene.hpp"
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace drayven {
class EditorApp {
public:
    int run();
private:
    bool init();
    void shutdown();
    void frame();
    void drawWelcome();
    void drawDockspace();
    void drawHierarchy();
    void drawInspector();
    void drawAssets();
    void drawConsole();
    void drawScriptEditor();
    void drawBuildWindow();
    void newProject(ProjectKind kind);
    bool openProject(const std::filesystem::path& file);
    void saveScene();
    void addLog(std::string text);
    void buildDesktop();
    void buildAndroid();

    Application m_app;
    Localization m_i18n;
    std::optional<ProjectConfig> m_project;
    Scene m_scene;
    EntityId m_selected{};
    std::vector<std::string> m_logs;
    std::string m_projectPath{"DrayvenProjects/MyGame"};
    std::string m_projectName{"MyGame"};
    std::string m_scriptBuffer;
    std::filesystem::path m_currentScript;
    bool m_showWelcome{true};
    bool m_showBuild{false};
};

std::string persianVisualOrder(std::string_view utf8);
}
