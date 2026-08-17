#include "drayven/EditorApp.hpp"
#include "RmlBackend.hpp"
#include "drayven/Build.hpp"
#include "drayven/Drys.hpp"
#include "drayven/Persian.hpp"
#include "drayven/Project.hpp"
#include "drayven/Scene.hpp"
#include "drayven/UiDocument.hpp"
#include <RmlUi/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Elements/ElementFormControl.h>
#include <RmlUi/Core/Elements/ElementFormControlTextArea.h>
#include <RmlUi/Debugger.h>
#include <SDL3/SDL.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#ifndef DRAYVEN_EDITOR_ASSET_ROOT
#define DRAYVEN_EDITOR_ASSET_ROOT "assets"
#endif

namespace drayven {
namespace fs = std::filesystem;

namespace {
std::string html(std::string_view s) {
    std::string out;
    out.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        default: out.push_back(c); break;
        }
    }
    return out;
}

fs::path locateAssets() {
    std::vector<fs::path> candidates;
    candidates.emplace_back(fs::current_path() / "assets");
    if (const char* base = SDL_GetBasePath()) {
        fs::path p(base);
        candidates.push_back(p / "assets");
        candidates.push_back(p.parent_path() / "assets");
        candidates.push_back(p / "../assets");
    }
    candidates.emplace_back(DRAYVEN_EDITOR_ASSET_ROOT);
    for (auto& c : candidates) {
        std::error_code ec;
        auto normalized = fs::weakly_canonical(c, ec);
        if (!ec && fs::exists(normalized / "editor/editor.rml")) return normalized;
        if (fs::exists(c / "editor/editor.rml")) return fs::absolute(c);
    }
    return {};
}

std::string readText(const fs::path& file) {
    std::ifstream in(file, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void writeText(const fs::path& file, std::string_view text) {
    fs::create_directories(file.parent_path());
    std::ofstream out(file, std::ios::binary);
    out << text;
}

std::string extensionLabel(const fs::path& p) {
    auto e = p.extension().string();
    if (e == ".drys") return "DRYS";
    if (e == ".lua") return "Lua";
    return e;
}
}

struct EditorApp::Impl final : Rml::EventListener {
    Rml::Context* context{};
    Rml::ElementDocument* document{};
    fs::path assetsRoot;
    std::optional<ProjectConfig> project;
    Scene scene;
    EntityId selected{};
    UiDocument ui;
    std::vector<std::string> logs;
    std::vector<fs::path> scriptPaths;
    fs::path currentScript;
    bool running{true};
    bool persian{false};

    void log(std::string text) {
        logs.push_back(std::move(text));
        if (logs.size() > 300) logs.erase(logs.begin(), logs.begin() + 80);
        refreshConsole();
    }

    Rml::Element* el(const char* id) const {
        return document ? document->GetElementById(id) : nullptr;
    }

    std::string value(const char* id) const {
        auto* e = el(id);
        auto* c = dynamic_cast<Rml::ElementFormControl*>(e);
        return c ? c->GetValue() : std::string{};
    }

    void setValue(const char* id, std::string_view v) {
        auto* e = el(id);
        if (auto* c = dynamic_cast<Rml::ElementFormControl*>(e)) c->SetValue(std::string(v));
    }

    void text(const char* id, std::string_view v) {
        if (auto* e = el(id)) e->SetInnerRML(html(v));
    }

    void listen(const char* id, const char* event = "click") {
        if (auto* e = el(id)) e->AddEventListener(event, this);
    }

    void showWorkspace(std::string_view target) {
        for (auto id : {"workspace-scene", "workspace-script", "workspace-ui", "workspace-build"}) {
            if (auto* e = el(id)) e->SetProperty("display", target == id ? "block" : "none");
        }
        for (auto id : {"nav-scene", "nav-script", "nav-ui", "nav-build"}) {
            if (auto* e = el(id)) e->SetClass("selected", target.substr(10) == std::string(id).substr(4));
        }
    }

    void applyLanguage() {
        struct Item { const char* id; const char* en; const char* fa; };
        static const Item items[] = {
            {"nav-scene","Scene","صحنه"},{"nav-script","Scripts","اسکریپت‌ها"},
            {"nav-ui","UI Creator","طراح رابط"},{"nav-build","Build","بیلد"},
            {"project-title","PROJECT","پروژه"},{"hierarchy-title","HIERARCHY","سلسله‌مراتب"},
            {"assets-title","ASSETS","فایل‌ها"},{"console-title","CONSOLE","کنسول"},
            {"script-title","DRYS / LUA WORKSPACE","محیط DRYS / LUA"},
            {"ui-title","UI CREATOR","طراح رابط کاربری"},{"build-title","BUILD & EXPORT","بیلد و خروجی"}
        };
        for (const auto& i : items) text(i.id, persian ? persianVisualOrder(i.fa) : i.en);
        if (auto* root = el("app-root")) root->SetClass("rtl", persian);
        text("lang-state", persian ? "FA" : "EN");
    }

    bool loadFonts() {
        std::vector<fs::path> fonts = {
            assetsRoot / "fonts/Vazirmatn-Regular.ttf",
#ifdef _WIN32
            "C:/Windows/Fonts/segoeui.ttf",
            "C:/Windows/Fonts/tahoma.ttf",
#else
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
#endif
        };
        bool any = false;
        for (auto& f : fonts) {
            if (fs::exists(f) && Rml::LoadFontFace(f.string(), !any)) any = true;
        }
        return any;
    }

    bool init() {
        if (!editor_backend::Initialize("Drayven Engine", 1560, 920)) return false;
        Rml::SetSystemInterface(editor_backend::GetSystemInterface());
        Rml::SetRenderInterface(editor_backend::GetRenderInterface());
        if (!Rml::Initialise()) return false;
        context = Rml::CreateContext("DrayvenEditor", {1560, 920});
        if (!context) return false;
        Rml::Debugger::Initialise(context);

        assetsRoot = locateAssets();
        if (assetsRoot.empty()) {
            log("Editor assets not found.");
            return false;
        }
        fs::current_path(assetsRoot.parent_path());
        loadFonts();

        document = context->LoadDocument("assets/editor/editor.rml");
        if (!document) return false;
        document->Show();

        for (auto id : {
            "win-min","win-max","win-close","nav-scene","nav-script","nav-ui","nav-build",
            "create-2d","create-3d","open-project","save-script","transpile-cpp","transpile-lua",
            "ui-add-panel","ui-add-label","ui-add-button","ui-add-input","ui-add-progress","ui-clear","ui-save",
            "build-desktop","build-android","lang-toggle"
        }) listen(id);

        setValue("project-name", "MyGame");
        setValue("project-path", "DrayvenProjects/MyGame");
        setValue("engine-root", fs::current_path().string());
        showWorkspace("workspace-scene");
        applyLanguage();
        log("Drayven Editor 0.2 ready — RmlUi shell online.");
        log("Native DRYS, LuaJIT bytecode, UI Creator and Android NDK exporter are available.");
        return true;
    }

    void shutdown() {
        document = nullptr;
        context = nullptr;
        Rml::Shutdown();
        editor_backend::Shutdown();
    }

    void createProject(ProjectKind kind) {
        try {
            auto name = value("project-name");
            auto path = value("project-path");
            if (name.empty() || path.empty()) { log("Project name/path cannot be empty."); return; }
            project = Project::create(path, name, kind);
            fs::create_directories(project->root / "Assets/UI");
            writeText(project->root / "Scripts/Main.drys",
                "game Main\n"
                "var speed = 5\n\n"
                "fn start()\n"
                "    log(\"Drayven native script started\")\n"
                "end\n\n"
                "fn update(dt)\n"
                "    if key_down(\"D\")\n"
                "        move_x(speed * dt)\n"
                "    end\n"
                "end\n");
            writeText(project->root / "Scripts/Gameplay.lua",
                "local M = {}\n\n"
                "function M.start()\n"
                "  print(\"LuaJIT script ready\")\n"
                "end\n\nreturn M\n");
            scene = Scene{};
            auto& camera = scene.createEntity("Main Camera");
            camera.hasCamera = true;
            camera.camera.primary = true;
            scene.createEntity("Player");
            currentScript = project->root / "Scripts/Main.drys";
            setValue("script-source", readText(currentScript));
            text("current-project", project->name);
            text("project-badge", kind == ProjectKind::Game3D ? "3D PROJECT" : "2D PROJECT");
            refreshAll();
            showWorkspace("workspace-scene");
            log("Created project: " + project->root.string());
        } catch (const std::exception& e) {
            log(std::string("Create failed: ") + e.what());
        }
    }

    void openProject() {
        try {
            fs::path file = value("project-file");
            if (file.empty()) { log("Enter a .drayven project file path."); return; }
            project = Project::load(file);
            text("current-project", project->name);
            text("project-badge", project->kind == ProjectKind::Game3D ? "3D PROJECT" : "2D PROJECT");
            scene = Scene{};
            auto& camera = scene.createEntity("Main Camera");
            camera.hasCamera = true;
            camera.camera.primary = true;
            refreshAll();
            if (!scriptPaths.empty()) selectScript(0);
            log("Opened project: " + file.string());
        } catch (const std::exception& e) {
            log(std::string("Open failed: ") + e.what());
        }
    }

    void refreshAll() {
        refreshHierarchy();
        refreshAssets();
        refreshScripts();
        refreshUi();
    }

    void refreshHierarchy() {
        auto* box = el("hierarchy-list");
        if (!box) return;
        std::ostringstream out;
        if (!project) {
            out << "<div class=\"muted pad\">Create or open a project</div>";
        } else {
            std::size_t i = 0;
            for (auto& e : scene.entities()) {
                out << "<button id=\"entity-" << i << "\" class=\"tree-row";
                if (e.id == selected) out << " active";
                out << "\"><span class=\"tree-dot\"></span>" << html(e.name) << "</button>";
                ++i;
            }
        }
        box->SetInnerRML(out.str());
        for (std::size_t i = 0; i < scene.entities().size(); ++i) listen(("entity-" + std::to_string(i)).c_str());
    }

    void refreshAssets() {
        auto* box = el("assets-list");
        if (!box) return;
        std::ostringstream out;
        if (!project) out << "<div class=\"muted pad\">No project assets</div>";
        else {
            for (auto folder : {"Assets","Scripts","Scenes"}) {
                fs::path p = project->root / folder;
                out << "<div class=\"asset-group\"><div class=\"asset-head\">" << folder << "</div>";
                if (fs::exists(p)) {
                    int shown = 0;
                    for (auto& e : fs::recursive_directory_iterator(p)) {
                        if (!e.is_regular_file() || shown++ > 40) continue;
                        out << "<div class=\"asset-row\">" << html(fs::relative(e.path(), project->root).generic_string()) << "</div>";
                    }
                }
                out << "</div>";
            }
        }
        box->SetInnerRML(out.str());
    }

    void refreshScripts() {
        scriptPaths.clear();
        auto* box = el("script-list");
        if (!box) return;
        std::ostringstream out;
        if (project && fs::exists(project->root / "Scripts")) {
            for (auto& e : fs::recursive_directory_iterator(project->root / "Scripts")) {
                if (!e.is_regular_file()) continue;
                auto ext = e.path().extension().string();
                if (ext != ".drys" && ext != ".lua") continue;
                auto index = scriptPaths.size();
                scriptPaths.push_back(e.path());
                out << "<button id=\"script-" << index << "\" class=\"script-file\"><span class=\"lang-pill\">"
                    << extensionLabel(e.path()) << "</span><span>" << html(e.path().filename().string()) << "</span></button>";
            }
        }
        if (scriptPaths.empty()) out << "<div class=\"muted pad\">No DRYS/Lua files</div>";
        box->SetInnerRML(out.str());
        for (std::size_t i = 0; i < scriptPaths.size(); ++i) listen(("script-" + std::to_string(i)).c_str());
    }

    void selectScript(std::size_t index) {
        if (index >= scriptPaths.size()) return;
        currentScript = scriptPaths[index];
        setValue("script-source", readText(currentScript));
        text("script-file-name", currentScript.filename().string());
        text("script-language", extensionLabel(currentScript));
        refreshHighlight();
        showWorkspace("workspace-script");
    }

    void saveScript() {
        if (currentScript.empty()) { log("Select a script first."); return; }
        try {
            writeText(currentScript, value("script-source"));
            log("Saved " + currentScript.filename().string());
            refreshHighlight();
        } catch (const std::exception& e) { log(e.what()); }
    }

    void refreshHighlight() {
        auto* box = el("syntax-preview");
        if (!box) return;
        std::string src = value("script-source");
        if (currentScript.extension() != ".drys") {
            box->SetInnerRML("<pre class=\"code-preview\">" + html(src) + "</pre>");
            return;
        }
        std::ostringstream out;
        out << "<div class=\"code-preview\">";
        drys::Lexer lex(src);
        for (auto& t : lex.run()) {
            if (t.kind == drys::TokenKind::End) break;
            if (t.kind == drys::TokenKind::Newline) { out << "<br/>"; continue; }
            const char* cls = "tok";
            if (t.kind == drys::TokenKind::Keyword) cls = "tok kw";
            else if (t.kind == drys::TokenKind::String) cls = "tok str";
            else if (t.kind == drys::TokenKind::Number) cls = "tok num";
            out << "<span class=\"" << cls << "\">" << html(t.text) << "</span> ";
        }
        out << "</div>";
        box->SetInnerRML(out.str());
    }

    void transpilePreview(drys::OutputLanguage language) {
        if (currentScript.empty() || currentScript.extension() != ".drys") {
            log("DRYS transpilation requires a .drys source file.");
            return;
        }
        drys::TranspileOptions options;
        options.language = language;
        options.hardenSymbols = true;
        auto result = drys::Transpiler{}.transpile(value("script-source"), currentScript.stem().string(), options);
        if (!result.ok) {
            for (auto& d : result.diagnostics) log(d);
            return;
        }
        if (auto* box = el("transpile-preview")) box->SetInnerRML("<pre class=\"generated\">" + html(result.output) + "</pre>");
        log(language == drys::OutputLanguage::Cpp ? "Generated hardened C++ preview." : "Generated Lua backend preview.");
    }

    void refreshUi() {
        if (auto* canvas = el("ui-canvas")) canvas->SetInnerRML(ui.toRmlFragment());
        for (const auto& w : ui.widgets()) listen(("uiw_" + std::to_string(w.id)).c_str());
        text("ui-count", std::to_string(ui.widgets().size()) + " widgets");
    }

    void addUi(UiWidgetType type) {
        auto label = value("ui-widget-text");
        auto handler = value("ui-handler");
        ui.add(type, label.empty() ? "Widget" : label, handler);
        refreshUi();
    }

    void saveUi() {
        if (!project) { log("Create/open a project first."); return; }
        try {
            auto file = project->root / "Assets/UI/Main.rml";
            ui.save(file);
            log("UI document saved: " + file.string());
            refreshAssets();
        } catch (const std::exception& e) { log(e.what()); }
    }

    BuildOptions buildOptions(BuildTarget target) {
        BuildOptions o;
        o.target = target;
        o.engineRoot = value("engine-root");
        o.sdkRoot = value("android-sdk");
        o.ndkRoot = value("android-ndk");
        o.luaJit = value("luajit-path");
        o.upx = value("upx-path");
        o.useUpx = !o.upx.empty();
        o.abi = value("android-abi").empty() ? "arm64-v8a" : value("android-abi");
        o.androidApi = value("android-api").empty() ? 24 : std::stoi(value("android-api"));
        o.scriptMode = value("script-backend") == "luajit" ? ScriptMode::LuaJit : ScriptMode::Native;
        o.hardenSymbols = value("symbol-hardening") != "off";
        return o;
    }

    void build(BuildTarget target) {
        if (!project) { log("Create/open a project first."); return; }
        log(target == BuildTarget::Android ? "Starting Android NDK build..." : "Starting desktop native build...");
        auto report = BuildPipeline::build(*project, buildOptions(target));
        for (auto& l : report.log) log(l);
        if (report.ok) {
            text("build-result", report.output.string());
            log("Build succeeded.");
        } else log("Build failed. See console.");
    }

    void refreshConsole() {
        auto* box = el("console-lines");
        if (!box) return;
        std::ostringstream out;
        for (auto& l : logs) out << "<div class=\"console-line\"><span class=\"chev\">›</span>" << html(l) << "</div>";
        box->SetInnerRML(out.str());
        box->SetScrollTop(box->GetScrollHeight());
    }

    void selectEntity(std::size_t index) {
        if (index >= scene.entities().size()) return;
        selected = scene.entities()[index].id;
        auto& e = scene.entities()[index];
        text("inspector-name", e.name);
        text("inspector-components",
             std::string(e.hasCamera ? "Camera  " : "") +
             (e.hasSprite ? "Sprite2D  " : "") +
             (e.hasMesh ? "Mesh3D  " : "") +
             (e.hasAudio ? "Audio  " : "") +
             (e.hasScript ? "Script" : ""));
        refreshHierarchy();
    }

    void ProcessEvent(Rml::Event& event) override {
        auto* target = event.GetCurrentElement();
        if (!target) return;
        const std::string id = target->GetId();

        if (id == "win-close") { editor_backend::RequestExit(); return; }
        if (id == "win-min") { SDL_MinimizeWindow(editor_backend::Window()); return; }
        if (id == "win-max") {
            auto* w = editor_backend::Window();
            if (SDL_GetWindowFlags(w) & SDL_WINDOW_MAXIMIZED) SDL_RestoreWindow(w);
            else SDL_MaximizeWindow(w);
            return;
        }
        if (id == "nav-scene") { showWorkspace("workspace-scene"); return; }
        if (id == "nav-script") { showWorkspace("workspace-script"); return; }
        if (id == "nav-ui") { showWorkspace("workspace-ui"); return; }
        if (id == "nav-build") { showWorkspace("workspace-build"); return; }
        if (id == "lang-toggle") { persian = !persian; applyLanguage(); return; }
        if (id == "create-2d") { createProject(ProjectKind::Game2D); return; }
        if (id == "create-3d") { createProject(ProjectKind::Game3D); return; }
        if (id == "open-project") { openProject(); return; }
        if (id == "save-script") { saveScript(); return; }
        if (id == "transpile-cpp") { transpilePreview(drys::OutputLanguage::Cpp); return; }
        if (id == "transpile-lua") { transpilePreview(drys::OutputLanguage::Lua); return; }
        if (id == "ui-add-panel") { addUi(UiWidgetType::Panel); return; }
        if (id == "ui-add-label") { addUi(UiWidgetType::Label); return; }
        if (id == "ui-add-button") { addUi(UiWidgetType::Button); return; }
        if (id == "ui-add-input") { addUi(UiWidgetType::Input); return; }
        if (id == "ui-add-progress") { addUi(UiWidgetType::Progress); return; }
        if (id == "ui-clear") { ui.clear(); refreshUi(); return; }
        if (id == "ui-save") { saveUi(); return; }
        if (id == "build-desktop") { build(BuildTarget::Desktop); return; }
        if (id == "build-android") { build(BuildTarget::Android); return; }

        if (id.rfind("entity-", 0) == 0) {
            selectEntity(static_cast<std::size_t>(std::stoul(id.substr(7))));
            return;
        }
        if (id.rfind("script-", 0) == 0) {
            selectScript(static_cast<std::size_t>(std::stoul(id.substr(7))));
            return;
        }
        if (id.rfind("uiw_", 0) == 0) {
            auto handler = target->GetAttribute<Rml::String>("data-handler", "");
            if (!handler.empty()) log("UI handler preview: " + handler);
        }
    }
};

EditorApp::EditorApp():m_impl(std::make_unique<Impl>()){}
EditorApp::~EditorApp() = default;

int EditorApp::run() {
    if (!m_impl->init()) {
        m_impl->shutdown();
        return 1;
    }
    while (m_impl->running && editor_backend::ProcessEvents(m_impl->context)) {
        m_impl->context->Update();
        editor_backend::BeginFrame();
        m_impl->context->Render();
        editor_backend::PresentFrame();
    }
    m_impl->shutdown();
    return 0;
}
}
