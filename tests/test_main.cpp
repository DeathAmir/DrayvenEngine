#include "drayven/Build.hpp"
#include "drayven/Drys.hpp"
#include "drayven/PackArchive.hpp"
#include "drayven/Project.hpp"
#include "drayven/UiDocument.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

int main() {
    const auto base = fs::temp_directory_path() / "drayven_engine_tests";
    std::error_code ec;
    fs::remove_all(base, ec);

    auto project = drayven::Project::create(base / "TestGame", "TestGame", drayven::ProjectKind::Game2D);
    assert(fs::exists(drayven::Project::filePath(project)));
    assert(fs::exists(project.root / "Assets/UI"));
    assert(fs::exists(project.root / "Plugins"));

    const std::string source =
        "game Test\n"
        "var speed = 4\n"
        "fn update(dt)\n"
        "  if key_down(\"D\")\n"
        "    move_x(speed * dt)\n"
        "  end\n"
        "end\n";
    drayven::drys::TranspileOptions native;
    native.hardenSymbols = true;
    auto cpp = drayven::drys::Transpiler{}.transpile(source, "Test", native);
    assert(cpp.ok);
    assert(cpp.output.find("drayven::script::moveX") != std::string::npos);
    assert(cpp.output.find("_dv_") != std::string::npos);

    drayven::drys::TranspileOptions lua;
    lua.language = drayven::drys::OutputLanguage::Lua;
    auto l = drayven::drys::Transpiler{}.transpile(source, "Test", lua);
    assert(l.ok);
    assert(l.output.find("drayven.move_x") != std::string::npos);

    drayven::UiDocument ui;
    ui.add(drayven::UiWidgetType::Button, "Play", "on_play");
    ui.add(drayven::UiWidgetType::Progress, "Loading");
    auto uiFile = project.root / "Assets/UI/Main.rml";
    ui.save(uiFile);
    assert(fs::exists(uiFile));
    assert(fs::exists(project.root / "Assets/UI/Main.dui.json"));

    std::ofstream(project.root / "Assets/hello.txt") << "drayven";
    const auto pack = project.root / "Build/test.dpack";
    drayven::PackArchive::packDirectory(project.root / "Assets", pack, "test-key");
    auto files = drayven::PackArchive::read(pack, "test-key");
    bool found = false;
    for (auto& f : files)
        if (f.path == "hello.txt" && std::string(f.bytes.begin(), f.bytes.end()) == "drayven") found = true;
    assert(found);

    fs::create_directories(base / "sdk/ndk/27.0");
    fs::create_directories(base / "sdk/ndk/28.1");
    assert(drayven::BuildPipeline::discoverNdk(base / "sdk").filename() == "28.1");

    fs::remove_all(base, ec);
    std::cout << "Drayven tests passed\n";
    return 0;
}
