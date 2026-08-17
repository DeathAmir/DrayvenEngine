#include "drayven/EditorApp.hpp"
#include "drayven/Build.hpp"
#include "drayven/Drys.hpp"
#include "drayven/Project.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#include <nuklear.h>

namespace drayven {
namespace fs = std::filesystem;

namespace {
constexpr int kTitleHeight = 48;
constexpr int kSideWidth = 218;
constexpr int kInspectorWidth = 292;
constexpr int kStatusHeight = 30;

std::string readText(const fs::path& file) {
    std::ifstream in(file, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void writeText(const fs::path& file, std::string_view text) {
    fs::create_directories(file.parent_path());
    std::ofstream out(file, std::ios::binary);
    out.write(text.data(), static_cast<std::streamsize>(text.size()));
}

template <std::size_t N>
void setBuffer(std::array<char, N>& dst, std::string_view text) {
    const std::size_t n = std::min<std::size_t>(N - 1, text.size());
    std::memset(dst.data(), 0, N);
    std::memcpy(dst.data(), text.data(), n);
}

std::string trimForLog(std::string text, std::size_t maxLen = 220) {
    std::replace(text.begin(), text.end(), '\n', ' ');
    if (text.size() > maxLen) text.resize(maxLen);
    return text;
}

void clipboardCopy(nk_handle, const char* text, int len) {
    if (!text || len <= 0) return;
    std::string copy(text, text + len);
    SDL_SetClipboardText(copy.c_str());
}

void clipboardPaste(nk_handle, nk_text_edit* edit) {
    char* text = SDL_GetClipboardText();
    if (text) {
        nk_textedit_paste(edit, text, static_cast<int>(std::strlen(text)));
        SDL_free(text);
    }
}

SDL_HitTestResult SDLCALL editorHitTest(SDL_Window* window, const SDL_Point* p, void*) {
    int w = 0, h = 0;
    SDL_GetWindowSize(window, &w, &h);
    constexpr int edge = 6;
    constexpr int controls = 154;
    const bool left = p->x < edge;
    const bool right = p->x >= w - edge;
    const bool top = p->y < edge;
    const bool bottom = p->y >= h - edge;
    if (top && left) return SDL_HITTEST_RESIZE_TOPLEFT;
    if (top && right) return SDL_HITTEST_RESIZE_TOPRIGHT;
    if (bottom && left) return SDL_HITTEST_RESIZE_BOTTOMLEFT;
    if (bottom && right) return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
    if (top) return SDL_HITTEST_RESIZE_TOP;
    if (bottom) return SDL_HITTEST_RESIZE_BOTTOM;
    if (left) return SDL_HITTEST_RESIZE_LEFT;
    if (right) return SDL_HITTEST_RESIZE_RIGHT;
    if (p->y < kTitleHeight && p->x < w - controls) return SDL_HITTEST_DRAGGABLE;
    return SDL_HITTEST_NORMAL;
}

struct NkVertex {
    float position[2];
    float uv[2];
    nk_byte color[4];
};

class NuklearSdl3 {
public:
    bool init(SDL_Window* win, SDL_Renderer* rnd) {
        window = win;
        renderer = rnd;
        if (!nk_init_default(&ctx, nullptr)) return false;
        contextReady = true;
        ctx.clip.copy = clipboardCopy;
        ctx.clip.paste = clipboardPaste;
        nk_buffer_init_default(&commands);
        commandsReady = true;

        nk_font_atlas_init_default(&atlas);
        atlasReady = true;
        nk_font_atlas_begin(&atlas);
        nk_font* font = nk_font_atlas_add_default(&atlas, 15.5f, nullptr);
        int tw = 0, th = 0;
        const void* pixels = nk_font_atlas_bake(&atlas, &tw, &th, NK_FONT_ATLAS_RGBA32);
        if (!pixels || tw <= 0 || th <= 0) return false;

        fontTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, tw, th);
        if (!fontTexture) return false;
        if (!SDL_UpdateTexture(fontTexture, nullptr, pixels, tw * 4)) return false;
        SDL_SetTextureBlendMode(fontTexture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(fontTexture, SDL_SCALEMODE_LINEAR);
        nk_font_atlas_end(&atlas, nk_handle_ptr(fontTexture), &nullTexture);
        if (font) nk_style_set_font(&ctx, &font->handle);
        applyTheme();
        return true;
    }

    void shutdown() {
        if (atlasReady) {
            nk_font_atlas_clear(&atlas);
            atlasReady = false;
        }
        if (contextReady) {
            nk_free(&ctx);
            contextReady = false;
        }
        if (commandsReady) {
            nk_buffer_free(&commands);
            commandsReady = false;
        }
        if (fontTexture) {
            SDL_DestroyTexture(fontTexture);
            fontTexture = nullptr;
        }
    }

    nk_context* get() { return &ctx; }

    void beginInput() { nk_input_begin(&ctx); }
    void endInput() { nk_input_end(&ctx); }

    void handleEvent(const SDL_Event& ev) {
        const bool ctrl = (SDL_GetModState() & SDL_KMOD_CTRL) != 0;
        switch (ev.type) {
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP: {
            const bool down = ev.type == SDL_EVENT_KEY_DOWN;
            switch (ev.key.key) {
            case SDLK_LSHIFT:
            case SDLK_RSHIFT: nk_input_key(&ctx, NK_KEY_SHIFT, down); break;
            case SDLK_DELETE: nk_input_key(&ctx, NK_KEY_DEL, down); break;
            case SDLK_RETURN:
            case SDLK_KP_ENTER: nk_input_key(&ctx, NK_KEY_ENTER, down); break;
            case SDLK_TAB: nk_input_key(&ctx, NK_KEY_TAB, down); break;
            case SDLK_BACKSPACE: nk_input_key(&ctx, NK_KEY_BACKSPACE, down); break;
            case SDLK_HOME:
                nk_input_key(&ctx, NK_KEY_TEXT_START, down);
                nk_input_key(&ctx, NK_KEY_SCROLL_START, down);
                break;
            case SDLK_END:
                nk_input_key(&ctx, NK_KEY_TEXT_END, down);
                nk_input_key(&ctx, NK_KEY_SCROLL_END, down);
                break;
            case SDLK_PAGEUP: nk_input_key(&ctx, NK_KEY_SCROLL_UP, down); break;
            case SDLK_PAGEDOWN: nk_input_key(&ctx, NK_KEY_SCROLL_DOWN, down); break;
            case SDLK_UP: nk_input_key(&ctx, NK_KEY_UP, down); break;
            case SDLK_DOWN: nk_input_key(&ctx, NK_KEY_DOWN, down); break;
            case SDLK_LEFT:
                nk_input_key(&ctx, ctrl ? NK_KEY_TEXT_WORD_LEFT : NK_KEY_LEFT, down);
                break;
            case SDLK_RIGHT:
                nk_input_key(&ctx, ctrl ? NK_KEY_TEXT_WORD_RIGHT : NK_KEY_RIGHT, down);
                break;
            case SDLK_A: nk_input_key(&ctx, NK_KEY_TEXT_SELECT_ALL, down && ctrl); break;
            case SDLK_C: nk_input_key(&ctx, NK_KEY_COPY, down && ctrl); break;
            case SDLK_V: nk_input_key(&ctx, NK_KEY_PASTE, down && ctrl); break;
            case SDLK_X: nk_input_key(&ctx, NK_KEY_CUT, down && ctrl); break;
            case SDLK_Z: nk_input_key(&ctx, NK_KEY_TEXT_UNDO, down && ctrl); break;
            case SDLK_R: nk_input_key(&ctx, NK_KEY_TEXT_REDO, down && ctrl); break;
            default: break;
            }
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            const bool down = ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
            const int x = static_cast<int>(ev.button.x);
            const int y = static_cast<int>(ev.button.y);
            if (ev.button.button == SDL_BUTTON_LEFT) {
                if (ev.button.clicks > 1) nk_input_button(&ctx, NK_BUTTON_DOUBLE, x, y, down);
                nk_input_button(&ctx, NK_BUTTON_LEFT, x, y, down);
            } else if (ev.button.button == SDL_BUTTON_MIDDLE) {
                nk_input_button(&ctx, NK_BUTTON_MIDDLE, x, y, down);
            } else if (ev.button.button == SDL_BUTTON_RIGHT) {
                nk_input_button(&ctx, NK_BUTTON_RIGHT, x, y, down);
            }
            break;
        }
        case SDL_EVENT_MOUSE_MOTION:
            nk_input_motion(&ctx, static_cast<int>(ev.motion.x), static_cast<int>(ev.motion.y));
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            nk_input_scroll(&ctx, nk_vec2(ev.wheel.x, ev.wheel.y));
            break;
        case SDL_EVENT_TEXT_INPUT: {
            const char* text = ev.text.text;
            int remaining = static_cast<int>(std::strlen(text));
            while (remaining > 0) {
                nk_rune rune = 0;
                const int consumed = nk_utf_decode(text, &rune, remaining);
                if (consumed <= 0) break;
                nk_input_unicode(&ctx, rune);
                text += consumed;
                remaining -= consumed;
            }
            break;
        }
        default: break;
        }
    }

    void render() {
        static const nk_draw_vertex_layout_element layout[] = {
            {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(NkVertex, position)},
            {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(NkVertex, uv)},
            {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(NkVertex, color)},
            {NK_VERTEX_LAYOUT_END}
        };
        nk_convert_config config{};
        config.vertex_layout = layout;
        config.vertex_size = sizeof(NkVertex);
        config.vertex_alignment = NK_ALIGNOF(NkVertex);
        config.tex_null = nullTexture;
        config.circle_segment_count = 22;
        config.curve_segment_count = 22;
        config.arc_segment_count = 22;
        config.global_alpha = 1.0f;
        config.shape_AA = NK_ANTI_ALIASING_ON;
        config.line_AA = NK_ANTI_ALIASING_ON;

        nk_buffer vertices{};
        nk_buffer elements{};
        nk_buffer_init_default(&vertices);
        nk_buffer_init_default(&elements);
        nk_convert(&ctx, &commands, &vertices, &elements, &config);

        const auto* srcVertices = static_cast<const NkVertex*>(nk_buffer_memory_const(&vertices));
        const auto* indexOffset = static_cast<const nk_draw_index*>(nk_buffer_memory_const(&elements));
        const std::size_t vertexCount = vertices.needed / sizeof(NkVertex);
        const nk_draw_command* cmd = nullptr;
        std::vector<SDL_Vertex> expanded;

        nk_draw_foreach(cmd, &ctx, &commands) {
            if (!cmd->elem_count) continue;
            SDL_Rect clip{
                static_cast<int>(std::floor(cmd->clip_rect.x)),
                static_cast<int>(std::floor(cmd->clip_rect.y)),
                std::max(0, static_cast<int>(std::ceil(cmd->clip_rect.w))),
                std::max(0, static_cast<int>(std::ceil(cmd->clip_rect.h)))};
            SDL_SetRenderClipRect(renderer, &clip);

            expanded.clear();
            expanded.reserve(cmd->elem_count);
            for (unsigned int i = 0; i < cmd->elem_count; ++i) {
                const std::size_t index = indexOffset[i];
                if (index >= vertexCount) continue;
                const NkVertex& v = srcVertices[index];
                SDL_Vertex out{};
                out.position = SDL_FPoint{v.position[0], v.position[1]};
                out.tex_coord = SDL_FPoint{v.uv[0], v.uv[1]};
                out.color = SDL_FColor{
                    v.color[0] / 255.0f,
                    v.color[1] / 255.0f,
                    v.color[2] / 255.0f,
                    v.color[3] / 255.0f};
                expanded.push_back(out);
            }
            if (!expanded.empty()) {
                SDL_RenderGeometry(renderer, static_cast<SDL_Texture*>(cmd->texture.ptr),
                                   expanded.data(), static_cast<int>(expanded.size()), nullptr, 0);
            }
            indexOffset += cmd->elem_count;
        }
        SDL_SetRenderClipRect(renderer, nullptr);
        nk_clear(&ctx);
        nk_buffer_clear(&commands);
        nk_buffer_free(&vertices);
        nk_buffer_free(&elements);
    }

private:
    void applyTheme() {
        nk_color colors[NK_COLOR_COUNT];
        colors[NK_COLOR_TEXT] = nk_rgb(218, 226, 235);
        colors[NK_COLOR_WINDOW] = nk_rgb(14, 18, 24);
        colors[NK_COLOR_HEADER] = nk_rgb(18, 24, 31);
        colors[NK_COLOR_BORDER] = nk_rgb(40, 51, 64);
        colors[NK_COLOR_BUTTON] = nk_rgb(28, 36, 46);
        colors[NK_COLOR_BUTTON_HOVER] = nk_rgb(35, 47, 59);
        colors[NK_COLOR_BUTTON_ACTIVE] = nk_rgb(50, 219, 164);
        colors[NK_COLOR_TOGGLE] = nk_rgb(34, 43, 53);
        colors[NK_COLOR_TOGGLE_HOVER] = nk_rgb(44, 55, 67);
        colors[NK_COLOR_TOGGLE_CURSOR] = nk_rgb(50, 219, 164);
        colors[NK_COLOR_SELECT] = nk_rgb(29, 42, 50);
        colors[NK_COLOR_SELECT_ACTIVE] = nk_rgb(43, 190, 143);
        colors[NK_COLOR_SLIDER] = nk_rgb(31, 40, 50);
        colors[NK_COLOR_SLIDER_CURSOR] = nk_rgb(50, 219, 164);
        colors[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgb(74, 230, 180);
        colors[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgb(32, 197, 145);
        colors[NK_COLOR_PROPERTY] = nk_rgb(23, 30, 38);
        colors[NK_COLOR_EDIT] = nk_rgb(19, 25, 32);
        colors[NK_COLOR_EDIT_CURSOR] = nk_rgb(50, 219, 164);
        colors[NK_COLOR_COMBO] = nk_rgb(24, 31, 39);
        colors[NK_COLOR_CHART] = nk_rgb(20, 26, 33);
        colors[NK_COLOR_CHART_COLOR] = nk_rgb(50, 219, 164);
        colors[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgb(255, 190, 72);
        colors[NK_COLOR_SCROLLBAR] = nk_rgb(16, 21, 27);
        colors[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgb(48, 59, 70);
        colors[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgb(62, 74, 86);
        colors[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgb(50, 219, 164);
        colors[NK_COLOR_TAB_HEADER] = nk_rgb(22, 29, 37);
        nk_style_from_table(&ctx, colors);
        ctx.style.window.rounding = 8.0f;
        ctx.style.button.rounding = 7.0f;
        ctx.style.edit.rounding = 6.0f;
        ctx.style.property.rounding = 6.0f;
        ctx.style.combo.rounding = 6.0f;
        ctx.style.window.padding = nk_vec2(10.0f, 9.0f);
        ctx.style.button.padding = nk_vec2(10.0f, 7.0f);
    }

    SDL_Window* window{};
    SDL_Renderer* renderer{};
    nk_context ctx{};
    nk_buffer commands{};
    nk_font_atlas atlas{};
    nk_draw_null_texture nullTexture{};
    SDL_Texture* fontTexture{};
    bool contextReady{};
    bool commandsReady{};
    bool atlasReady{};
};

enum class Workspace { Dashboard, Scene, Scripts, UiCreator, Build, Settings };
enum class UiKind { Panel, Button, Label, Input, Progress };

const char* workspaceName(Workspace w) {
    switch (w) {
    case Workspace::Dashboard: return "Dashboard";
    case Workspace::Scene: return "Scene";
    case Workspace::Scripts: return "Scripts";
    case Workspace::UiCreator: return "UI Creator";
    case Workspace::Build: return "Build & Export";
    case Workspace::Settings: return "Settings";
    }
    return "Workspace";
}

const char* uiKindName(UiKind k) {
    switch (k) {
    case UiKind::Panel: return "Panel";
    case UiKind::Button: return "Button";
    case UiKind::Label: return "Label";
    case UiKind::Input: return "Input";
    case UiKind::Progress: return "Progress";
    }
    return "Element";
}

struct UiItem {
    UiKind kind{UiKind::Button};
    std::string label{"Button"};
    int x{30}, y{30}, w{130}, h{36};
    int value{65};
};
}

struct EditorApp::Impl {
    SDL_Window* window{};
    SDL_Renderer* renderer{};
    NuklearSdl3 uiBackend;
    nk_context* ctx{};
    bool running{true};
    Workspace workspace{Workspace::Dashboard};
    std::optional<ProjectConfig> project;
    fs::path currentScript;
    std::vector<std::string> logs;
    std::vector<UiItem> uiItems;
    int selectedUi{-1};
    int scriptMode{0};
    nk_bool hardenSymbols{1};
    nk_bool useUpx{0};

    std::array<char, 128> projectName{};
    std::array<char, 512> projectPath{};
    std::array<char, 512> projectFile{};
    std::array<char, 32768> scriptSource{};
    std::array<char, 32768> transpileOutput{};
    std::array<char, 512> engineRoot{};
    std::array<char, 512> sdkRoot{};
    std::array<char, 512> ndkRoot{};
    std::array<char, 512> luaJitRoot{};
    std::array<char, 512> upxPath{};

    void log(std::string text) {
        logs.push_back(trimForLog(std::move(text)));
        if (logs.size() > 180) logs.erase(logs.begin(), logs.begin() + 40);
    }

    bool init() {
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
            SDL_Log("Drayven: SDL init failed: %s", SDL_GetError());
            return false;
        }
        const SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS;
        window = SDL_CreateWindow("Drayven Engine", 1500, 900, flags);
        if (!window) {
            SDL_Log("Drayven: window creation failed: %s", SDL_GetError());
            return false;
        }
        SDL_SetWindowMinimumSize(window, 1040, 680);
        SDL_SetWindowHitTest(window, editorHitTest, nullptr);

        const char* requested = std::getenv("DRAYVEN_UI_RENDERER");
#ifdef _WIN32
        const char* safeRenderers = "direct3d12,direct3d11,direct3d,software";
#elif defined(__APPLE__)
        const char* safeRenderers = "metal,software";
#else
        const char* safeRenderers = "vulkan,software";
#endif
        renderer = SDL_CreateRenderer(window, (requested && *requested) ? requested : safeRenderers);
        if (!renderer && (!requested || std::strcmp(requested, "software") != 0)) {
            SDL_Log("Drayven: hardware UI renderer unavailable (%s), forcing software renderer", SDL_GetError());
            renderer = SDL_CreateRenderer(window, "software");
        }
        if (!renderer) {
            SDL_Log("Drayven: SDL renderer creation failed: %s", SDL_GetError());
            return false;
        }
        SDL_SetRenderVSync(renderer, 1);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        if (!uiBackend.init(window, renderer)) {
            SDL_Log("Drayven: Nuklear renderer init failed: %s", SDL_GetError());
            return false;
        }
        ctx = uiBackend.get();
        SDL_StartTextInput(window);

        setBuffer(projectName, "MyGame");
        setBuffer(projectPath, (fs::current_path() / "DrayvenProjects/MyGame").string());
        setBuffer(projectFile, (fs::current_path() / "DrayvenProjects/MyGame/MyGame.drayven").string());
        setBuffer(engineRoot, fs::current_path().string());
        setBuffer(scriptSource,
            "game Main\n\n"
            "var speed = 5\n\n"
            "fn start()\n"
            "    log(\"Drayven ready\")\n"
            "end\n\n"
            "fn update(dt)\n"
            "    if key_down(\"D\")\n"
            "        move_x(speed * dt)\n"
            "    end\n"
            "end\n");
        setBuffer(transpileOutput, "// Transpiled output appears here");
        uiItems.push_back({UiKind::Panel, "HUD Panel", 26, 28, 390, 230, 0});
        uiItems.push_back({UiKind::Label, "PLAYER", 48, 52, 120, 30, 0});
        uiItems.push_back({UiKind::Progress, "Health", 48, 92, 260, 28, 78});
        uiItems.push_back({UiKind::Button, "PLAY", 48, 146, 120, 38, 0});

        const char* rname = SDL_GetRendererName(renderer);
        log(std::string("Editor online: Nuklear + SDL3 Renderer / ") + (rname ? rname : "unknown"));
        log("OpenGL context disabled for the editor; DRAYVEN_UI_RENDERER can override the renderer.");
        return true;
    }

    void shutdown() {
        if (window) SDL_StopTextInput(window);
        uiBackend.shutdown();
        if (renderer) SDL_DestroyRenderer(renderer);
        renderer = nullptr;
        if (window) SDL_DestroyWindow(window);
        window = nullptr;
        SDL_Quit();
    }

    void createProject(ProjectKind kind) {
        try {
            if (!projectName[0] || !projectPath[0]) {
                log("Project name and path are required.");
                return;
            }
            project = Project::create(fs::path(projectPath.data()), projectName.data(), kind);
            fs::create_directories(project->root / "Scripts");
            fs::create_directories(project->root / "Assets/UI");
            currentScript = project->root / "Scripts/Main.drys";
            writeText(currentScript, scriptSource.data());
            writeText(project->root / "Scripts/Gameplay.lua",
                      "local M = {}\nfunction M.start() print(\"LuaJIT ready\") end\nreturn M\n");
            setBuffer(projectFile, Project::filePath(*project).string());
            log("Created project: " + project->root.string());
            workspace = Workspace::Scene;
        } catch (const std::exception& e) {
            log(std::string("Create failed: ") + e.what());
        }
    }

    void openProject() {
        try {
            project = Project::load(fs::path(projectFile.data()));
            setBuffer(projectName, project->name);
            setBuffer(projectPath, project->root.string());
            currentScript = project->root / "Scripts/Main.drys";
            if (fs::exists(currentScript)) setBuffer(scriptSource, readText(currentScript));
            log("Opened project: " + Project::filePath(*project).string());
            workspace = Workspace::Scene;
        } catch (const std::exception& e) {
            log(std::string("Open failed: ") + e.what());
        }
    }

    void saveScript() {
        if (currentScript.empty()) {
            if (!project) { log("Create/open a project first."); return; }
            currentScript = project->root / "Scripts/Main.drys";
        }
        try {
            writeText(currentScript, scriptSource.data());
            log("Saved script: " + currentScript.string());
        } catch (const std::exception& e) {
            log(std::string("Save failed: ") + e.what());
        }
    }

    void transpile(drys::OutputLanguage language) {
        try {
            drys::TranspileOptions opts;
            opts.language = language;
            opts.hardenSymbols = hardenSymbols != 0;
            drys::Transpiler transpiler;
            auto result = transpiler.transpile(scriptSource.data(), "GameScript", opts);
            setBuffer(transpileOutput, result.output);
            if (result.ok) log(std::string("DRYS transpiled to ") + (language == drys::OutputLanguage::Cpp ? "C++" : "Lua"));
            for (const auto& d : result.diagnostics) log(d);
        } catch (const std::exception& e) {
            log(std::string("Transpile failed: ") + e.what());
        }
    }

    void build(BuildTarget target) {
        if (!project) {
            log("Create/open a project before building.");
            return;
        }
        try {
            BuildOptions opts;
            opts.target = target;
            opts.scriptMode = scriptMode == 0 ? ScriptMode::Native : ScriptMode::LuaJit;
            opts.engineRoot = engineRoot.data();
            opts.sdkRoot = sdkRoot.data();
            opts.ndkRoot = ndkRoot.data();
            opts.luaJit = luaJitRoot.data();
            opts.upx = upxPath.data();
            opts.hardenSymbols = hardenSymbols != 0;
            opts.useUpx = useUpx != 0;
            auto report = BuildPipeline::build(*project, opts);
            for (const auto& line : report.log) log(line);
            log(report.ok ? "Build succeeded: " + report.output.string() : "Build failed.");
        } catch (const std::exception& e) {
            log(std::string("Build exception: ") + e.what());
        }
    }

    bool navButton(const char* label, Workspace target) {
        std::string text = workspace == target ? std::string(">  ") + label : std::string("   ") + label;
        if (nk_button_label(ctx, text.c_str())) {
            workspace = target;
            return true;
        }
        return false;
    }

    void drawTitleBar(int width) {
        if (nk_begin(ctx, "drayven-titlebar", nk_rect(0, 0, static_cast<float>(width), kTitleHeight), NK_WINDOW_NO_SCROLLBAR)) {
            nk_layout_space_begin(ctx, NK_STATIC, 32, 6);
            nk_layout_space_push(ctx, nk_rect(6, 0, 34, 30));
            nk_button_label(ctx, "D");
            nk_layout_space_push(ctx, nk_rect(48, 2, 310, 28));
            nk_label(ctx, "DRAYVEN ENGINE   /   EDITOR", NK_TEXT_LEFT);
            nk_layout_space_push(ctx, nk_rect(static_cast<float>(width - 150), 0, 42, 30));
            if (nk_button_label(ctx, "-")) SDL_MinimizeWindow(window);
            nk_layout_space_push(ctx, nk_rect(static_cast<float>(width - 104), 0, 42, 30));
            if (nk_button_label(ctx, "[]")) {
                if (SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED) SDL_RestoreWindow(window);
                else SDL_MaximizeWindow(window);
            }
            nk_layout_space_push(ctx, nk_rect(static_cast<float>(width - 58), 0, 42, 30));
            if (nk_button_label(ctx, "X")) running = false;
            nk_layout_space_end(ctx);
        }
        nk_end(ctx);
    }

    void drawSidebar(int height) {
        const int bodyH = height - kTitleHeight - kStatusHeight;
        if (nk_begin(ctx, "drayven-sidebar", nk_rect(0, kTitleHeight, kSideWidth, static_cast<float>(bodyH)), NK_WINDOW_NO_SCROLLBAR)) {
            nk_layout_row_dynamic(ctx, 26, 1);
            nk_label_colored(ctx, "WORKSPACE", NK_TEXT_LEFT, nk_rgb(104, 125, 143));
            nk_layout_row_dynamic(ctx, 38, 1);
            navButton("Dashboard", Workspace::Dashboard);
            navButton("Scene", Workspace::Scene);
            navButton("Scripts", Workspace::Scripts);
            navButton("UI Creator", Workspace::UiCreator);
            navButton("Build", Workspace::Build);
            navButton("Settings", Workspace::Settings);
            nk_layout_row_dynamic(ctx, 18, 1);
            nk_spacing(ctx, 1);
            nk_layout_row_dynamic(ctx, 26, 1);
            nk_label_colored(ctx, "PROJECT", NK_TEXT_LEFT, nk_rgb(104, 125, 143));
            nk_layout_row_dynamic(ctx, 24, 1);
            nk_label(ctx, project ? project->name.c_str() : "No project loaded", NK_TEXT_LEFT);
            nk_layout_row_dynamic(ctx, 18, 1);
            nk_label_colored(ctx, project ? project->root.string().c_str() : "Create or open one", NK_TEXT_LEFT, nk_rgb(121, 137, 151));
        }
        nk_end(ctx);
    }

    void drawInspector(int width, int height) {
        const int bodyH = height - kTitleHeight - kStatusHeight;
        const float x = static_cast<float>(width - kInspectorWidth);
        if (nk_begin(ctx, "drayven-inspector", nk_rect(x, kTitleHeight, kInspectorWidth, static_cast<float>(bodyH)), NK_WINDOW_NO_SCROLLBAR)) {
            nk_layout_row_dynamic(ctx, 27, 1);
            nk_label_colored(ctx, "INSPECTOR", NK_TEXT_LEFT, nk_rgb(104, 125, 143));
            if (workspace == Workspace::UiCreator && selectedUi >= 0 && selectedUi < static_cast<int>(uiItems.size())) {
                UiItem& item = uiItems[static_cast<std::size_t>(selectedUi)];
                nk_layout_row_dynamic(ctx, 25, 1);
                nk_label(ctx, uiKindName(item.kind), NK_TEXT_LEFT);
                nk_layout_row_dynamic(ctx, 30, 1);
                nk_property_int(ctx, "X", 0, &item.x, 1600, 1, 1.0f);
                nk_property_int(ctx, "Y", 0, &item.y, 1000, 1, 1.0f);
                nk_property_int(ctx, "Width", 20, &item.w, 1400, 2, 1.0f);
                nk_property_int(ctx, "Height", 18, &item.h, 900, 2, 1.0f);
                if (item.kind == UiKind::Progress) nk_property_int(ctx, "Value", 0, &item.value, 100, 1, 1.0f);
                nk_layout_row_dynamic(ctx, 30, 1);
                if (nk_button_label(ctx, "Delete element")) {
                    uiItems.erase(uiItems.begin() + selectedUi);
                    selectedUi = -1;
                }
            } else {
                nk_layout_row_dynamic(ctx, 23, 1);
                nk_label(ctx, workspaceName(workspace), NK_TEXT_LEFT);
                nk_layout_row_dynamic(ctx, 20, 1);
                nk_label_colored(ctx, "Select an element to inspect it.", NK_TEXT_LEFT, nk_rgb(121, 137, 151));
            }

            nk_layout_row_dynamic(ctx, 14, 1);
            nk_spacing(ctx, 1);
            nk_layout_row_dynamic(ctx, 27, 1);
            nk_label_colored(ctx, "CONSOLE", NK_TEXT_LEFT, nk_rgb(104, 125, 143));
            const float consoleH = std::max(170.0f, static_cast<float>(bodyH) * 0.47f);
            nk_layout_row_dynamic(ctx, consoleH, 1);
            if (nk_group_begin(ctx, "console-scroll", NK_WINDOW_BORDER)) {
                const std::size_t start = logs.size() > 28 ? logs.size() - 28 : 0;
                for (std::size_t i = start; i < logs.size(); ++i) {
                    nk_layout_row_dynamic(ctx, 19, 1);
                    nk_label(ctx, logs[i].c_str(), NK_TEXT_LEFT);
                }
                nk_group_end(ctx);
            }
        }
        nk_end(ctx);
    }

    void drawStatus(int width, int height) {
        const char* rendererName = renderer ? SDL_GetRendererName(renderer) : "none";
        std::string status = std::string("READY    |    UI: Nuklear / SDL3    |    Renderer: ") +
                             (rendererName ? rendererName : "unknown") +
                             "    |    OpenGL context: OFF";
        if (nk_begin(ctx, "drayven-status", nk_rect(0, static_cast<float>(height - kStatusHeight), static_cast<float>(width), kStatusHeight), NK_WINDOW_NO_SCROLLBAR)) {
            nk_layout_row_dynamic(ctx, 18, 1);
            nk_label_colored(ctx, status.c_str(), NK_TEXT_LEFT, nk_rgb(99, 225, 180));
        }
        nk_end(ctx);
    }

    void drawProjectControls() {
        nk_layout_row_dynamic(ctx, 23, 1);
        nk_label_colored(ctx, "PROJECT NAME", NK_TEXT_LEFT, nk_rgb(105, 125, 143));
        nk_layout_row_dynamic(ctx, 34, 1);
        nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, projectName.data(), static_cast<int>(projectName.size()), nk_filter_default);
        nk_layout_row_dynamic(ctx, 23, 1);
        nk_label_colored(ctx, "PROJECT DIRECTORY", NK_TEXT_LEFT, nk_rgb(105, 125, 143));
        nk_layout_row_dynamic(ctx, 34, 1);
        nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, projectPath.data(), static_cast<int>(projectPath.size()), nk_filter_default);
        nk_layout_row_begin(ctx, NK_DYNAMIC, 36, 2);
        nk_layout_row_push(ctx, 0.5f);
        if (nk_button_label(ctx, "Create 2D")) createProject(ProjectKind::Game2D);
        nk_layout_row_push(ctx, 0.5f);
        if (nk_button_label(ctx, "Create 3D")) createProject(ProjectKind::Game3D);
        nk_layout_row_end(ctx);
        nk_layout_row_dynamic(ctx, 22, 1);
        nk_label_colored(ctx, "OPEN .DRAYVEN FILE", NK_TEXT_LEFT, nk_rgb(105, 125, 143));
        nk_layout_row_dynamic(ctx, 34, 1);
        nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, projectFile.data(), static_cast<int>(projectFile.size()), nk_filter_default);
        nk_layout_row_dynamic(ctx, 34, 1);
        if (nk_button_label(ctx, "Open project")) openProject();
    }

    void drawDashboard(float w, float h) {
        nk_layout_row_dynamic(ctx, 34, 1);
        nk_label(ctx, "Drayven Engine", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 23, 1);
        nk_label_colored(ctx, "FAST NATIVE GAME TOOLCHAIN / DRYS + LUAJIT + C++", NK_TEXT_LEFT, nk_rgb(50, 219, 164));
        nk_layout_row_dynamic(ctx, 16, 1);
        nk_spacing(ctx, 1);
        nk_layout_row_begin(ctx, NK_DYNAMIC, std::max(260.0f, h - 150.0f), 2);
        nk_layout_row_push(ctx, 0.56f);
        if (nk_group_begin(ctx, "dashboard-project", NK_WINDOW_BORDER)) {
            nk_layout_row_dynamic(ctx, 29, 1);
            nk_label(ctx, "Create / Open Project", NK_TEXT_LEFT);
            drawProjectControls();
            nk_group_end(ctx);
        }
        nk_layout_row_push(ctx, 0.44f);
        if (nk_group_begin(ctx, "dashboard-runtime", NK_WINDOW_BORDER)) {
            nk_layout_row_dynamic(ctx, 29, 1);
            nk_label(ctx, "Editor Runtime", NK_TEXT_LEFT);
            nk_layout_row_dynamic(ctx, 21, 1);
            nk_label(ctx, "Immediate UI: Nuklear", NK_TEXT_LEFT);
            nk_label(ctx, "Window backend: SDL3", NK_TEXT_LEFT);
            nk_label(ctx, "Editor OpenGL context: disabled", NK_TEXT_LEFT);
            const char* rn = SDL_GetRendererName(renderer);
            std::string r = std::string("Active renderer: ") + (rn ? rn : "unknown");
            nk_label(ctx, r.c_str(), NK_TEXT_LEFT);
            nk_layout_row_dynamic(ctx, 18, 1);
            nk_spacing(ctx, 1);
            nk_layout_row_dynamic(ctx, 29, 1);
            nk_label(ctx, "Build Targets", NK_TEXT_LEFT);
            nk_layout_row_dynamic(ctx, 21, 1);
            nk_label(ctx, "Windows / Linux / Android", NK_TEXT_LEFT);
            nk_label(ctx, "MinGW static archive: libDrayvenEngine.a", NK_TEXT_LEFT);
            nk_group_end(ctx);
        }
        nk_layout_row_end(ctx);
        (void)w;
    }

    void drawScene(float h) {
        nk_layout_row_dynamic(ctx, 32, 1);
        nk_label(ctx, "Scene Workspace", NK_TEXT_LEFT);
        if (!project) {
            nk_layout_row_dynamic(ctx, 22, 1);
            nk_label_colored(ctx, "No project loaded. Use Dashboard to create or open one.", NK_TEXT_LEFT, nk_rgb(236, 177, 91));
            return;
        }
        nk_layout_row_begin(ctx, NK_DYNAMIC, std::max(300.0f, h - 90.0f), 2);
        nk_layout_row_push(ctx, 0.27f);
        if (nk_group_begin(ctx, "scene-hierarchy", NK_WINDOW_BORDER)) {
            nk_layout_row_dynamic(ctx, 25, 1);
            nk_label_colored(ctx, "HIERARCHY", NK_TEXT_LEFT, nk_rgb(105, 125, 143));
            nk_layout_row_dynamic(ctx, 30, 1);
            nk_button_label(ctx, "Main Camera");
            nk_button_label(ctx, "Player");
            nk_button_label(ctx, "Environment");
            nk_group_end(ctx);
        }
        nk_layout_row_push(ctx, 0.73f);
        if (nk_group_begin(ctx, "scene-viewport", NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR)) {
            nk_layout_row_dynamic(ctx, 24, 1);
            nk_label(ctx, "VIEWPORT", NK_TEXT_LEFT);
            nk_layout_row_dynamic(ctx, std::max(230.0f, h - 155.0f), 1);
            if (nk_group_begin(ctx, "viewport-inner", NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR)) {
                nk_layout_row_dynamic(ctx, 28, 1);
                nk_label_colored(ctx, "3D/2D render surface attaches here", NK_TEXT_CENTERED, nk_rgb(87, 105, 120));
                nk_layout_row_dynamic(ctx, 22, 1);
                nk_label_colored(ctx, "Editor chrome stays independent from OpenGL", NK_TEXT_CENTERED, nk_rgb(50, 219, 164));
                nk_group_end(ctx);
            }
            nk_group_end(ctx);
        }
        nk_layout_row_end(ctx);
    }

    void drawScripts(float h) {
        nk_layout_row_dynamic(ctx, 32, 1);
        nk_label(ctx, "DRYS / LuaJIT Scripts", NK_TEXT_LEFT);
        nk_layout_row_begin(ctx, NK_DYNAMIC, 35, 4);
        nk_layout_row_push(ctx, 0.2f);
        if (nk_button_label(ctx, "Save")) saveScript();
        nk_layout_row_push(ctx, 0.24f);
        if (nk_button_label(ctx, "DRYS -> C++")) transpile(drys::OutputLanguage::Cpp);
        nk_layout_row_push(ctx, 0.24f);
        if (nk_button_label(ctx, "DRYS -> Lua")) transpile(drys::OutputLanguage::Lua);
        nk_layout_row_push(ctx, 0.22f);
        nk_checkbox_label(ctx, "Harden symbols", &hardenSymbols);
        nk_layout_row_end(ctx);

        const float editH = std::max(210.0f, (h - 110.0f) * 0.60f);
        nk_layout_row_dynamic(ctx, editH, 1);
        nk_edit_string_zero_terminated(ctx, NK_EDIT_BOX | NK_EDIT_MULTILINE, scriptSource.data(), static_cast<int>(scriptSource.size()), nk_filter_default);
        nk_layout_row_dynamic(ctx, 22, 1);
        nk_label_colored(ctx, "TRANSPILER OUTPUT", NK_TEXT_LEFT, nk_rgb(105, 125, 143));
        nk_layout_row_dynamic(ctx, std::max(110.0f, h - editH - 145.0f), 1);
        nk_edit_string_zero_terminated(ctx, NK_EDIT_BOX | NK_EDIT_MULTILINE | NK_EDIT_READ_ONLY,
                                      transpileOutput.data(), static_cast<int>(transpileOutput.size()), nk_filter_default);
    }

    void addUiItem(UiKind kind) {
        const int offset = static_cast<int>(uiItems.size()) * 13;
        UiItem item;
        item.kind = kind;
        item.label = uiKindName(kind);
        item.x = 28 + (offset % 160);
        item.y = 28 + (offset % 120);
        if (kind == UiKind::Panel) { item.w = 320; item.h = 180; }
        if (kind == UiKind::Label) { item.w = 120; item.h = 28; }
        if (kind == UiKind::Input) { item.w = 210; item.h = 34; }
        if (kind == UiKind::Progress) { item.w = 220; item.h = 28; item.value = 70; }
        uiItems.push_back(std::move(item));
        selectedUi = static_cast<int>(uiItems.size()) - 1;
    }

    void drawUiCreator(float w, float h) {
        nk_layout_row_dynamic(ctx, 32, 1);
        nk_label(ctx, "UI Creator", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 22, 1);
        nk_label_colored(ctx, "Palette + live canvas + inspector. Runtime widgets can be exported by the engine layer.", NK_TEXT_LEFT, nk_rgb(105, 125, 143));
        nk_layout_row_begin(ctx, NK_STATIC, std::max(330.0f, h - 92.0f), 2);
        nk_layout_row_push(ctx, 150.0f);
        if (nk_group_begin(ctx, "ui-palette", NK_WINDOW_BORDER)) {
            nk_layout_row_dynamic(ctx, 24, 1);
            nk_label_colored(ctx, "PALETTE", NK_TEXT_LEFT, nk_rgb(105, 125, 143));
            nk_layout_row_dynamic(ctx, 32, 1);
            if (nk_button_label(ctx, "+ Panel")) addUiItem(UiKind::Panel);
            if (nk_button_label(ctx, "+ Button")) addUiItem(UiKind::Button);
            if (nk_button_label(ctx, "+ Label")) addUiItem(UiKind::Label);
            if (nk_button_label(ctx, "+ Input")) addUiItem(UiKind::Input);
            if (nk_button_label(ctx, "+ Progress")) addUiItem(UiKind::Progress);
            nk_layout_row_dynamic(ctx, 18, 1);
            nk_spacing(ctx, 1);
            nk_layout_row_dynamic(ctx, 32, 1);
            if (nk_button_label(ctx, "Clear canvas")) { uiItems.clear(); selectedUi = -1; }
            nk_group_end(ctx);
        }
        nk_layout_row_push(ctx, std::max(300.0f, w - 180.0f));
        if (nk_group_begin(ctx, "ui-canvas", NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR)) {
            nk_layout_space_begin(ctx, NK_STATIC, std::max(300.0f, h - 128.0f), static_cast<int>(uiItems.size()));
            for (std::size_t i = 0; i < uiItems.size(); ++i) {
                UiItem& item = uiItems[i];
                nk_layout_space_push(ctx, nk_rect(static_cast<float>(item.x), static_cast<float>(item.y),
                                                  static_cast<float>(item.w), static_cast<float>(item.h)));
                bool clicked = false;
                switch (item.kind) {
                case UiKind::Panel: clicked = nk_button_label(ctx, item.label.c_str()) != 0; break;
                case UiKind::Button: clicked = nk_button_label(ctx, item.label.c_str()) != 0; break;
                case UiKind::Label: clicked = nk_button_label(ctx, item.label.c_str()) != 0; break;
                case UiKind::Input: clicked = nk_button_label(ctx, "Input field") != 0; break;
                case UiKind::Progress: {
                    std::string label = item.label + "  " + std::to_string(item.value) + "%";
                    clicked = nk_button_label(ctx, label.c_str()) != 0;
                    break;
                }
                }
                if (clicked) selectedUi = static_cast<int>(i);
            }
            nk_layout_space_end(ctx);
            nk_group_end(ctx);
        }
        nk_layout_row_end(ctx);
    }

    void pathEdit(const char* label, std::array<char, 512>& buffer) {
        nk_layout_row_dynamic(ctx, 21, 1);
        nk_label_colored(ctx, label, NK_TEXT_LEFT, nk_rgb(105, 125, 143));
        nk_layout_row_dynamic(ctx, 33, 1);
        nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, buffer.data(), static_cast<int>(buffer.size()), nk_filter_default);
    }

    void drawBuild(float h) {
        nk_layout_row_dynamic(ctx, 32, 1);
        nk_label(ctx, "Build & Export", NK_TEXT_LEFT);
        nk_layout_row_begin(ctx, NK_DYNAMIC, std::max(360.0f, h - 85.0f), 2);
        nk_layout_row_push(ctx, 0.56f);
        if (nk_group_begin(ctx, "build-paths", NK_WINDOW_BORDER)) {
            pathEdit("ENGINE ROOT", engineRoot);
            pathEdit("ANDROID SDK", sdkRoot);
            pathEdit("ANDROID NDK", ndkRoot);
            pathEdit("LUAJIT ROOT", luaJitRoot);
            pathEdit("UPX", upxPath);
            nk_group_end(ctx);
        }
        nk_layout_row_push(ctx, 0.44f);
        if (nk_group_begin(ctx, "build-options", NK_WINDOW_BORDER)) {
            nk_layout_row_dynamic(ctx, 25, 1);
            nk_label_colored(ctx, "SCRIPT BACKEND", NK_TEXT_LEFT, nk_rgb(105, 125, 143));
            nk_layout_row_begin(ctx, NK_DYNAMIC, 34, 2);
            nk_layout_row_push(ctx, 0.5f);
            if (nk_button_label(ctx, scriptMode == 0 ? "> Native C++" : "Native C++")) scriptMode = 0;
            nk_layout_row_push(ctx, 0.5f);
            if (nk_button_label(ctx, scriptMode == 1 ? "> LuaJIT" : "LuaJIT")) scriptMode = 1;
            nk_layout_row_end(ctx);
            nk_layout_row_dynamic(ctx, 30, 1);
            nk_checkbox_label(ctx, "Harden DRYS symbols", &hardenSymbols);
            nk_checkbox_label(ctx, "Compress with UPX", &useUpx);
            nk_layout_row_dynamic(ctx, 18, 1);
            nk_spacing(ctx, 1);
            nk_layout_row_dynamic(ctx, 38, 1);
            if (nk_button_label(ctx, "Build Desktop")) build(BuildTarget::Desktop);
            if (nk_button_label(ctx, "Build Android ARM64")) build(BuildTarget::Android);
            nk_layout_row_dynamic(ctx, 20, 1);
            nk_label_colored(ctx, "Android runtime: libDrayvenEngine.so", NK_TEXT_LEFT, nk_rgb(50, 219, 164));
            nk_label_colored(ctx, "MinGW runtime: libDrayvenEngine.a", NK_TEXT_LEFT, nk_rgb(50, 219, 164));
            nk_group_end(ctx);
        }
        nk_layout_row_end(ctx);
    }

    void drawSettings(float h) {
        nk_layout_row_dynamic(ctx, 32, 1);
        nk_label(ctx, "Editor Settings", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 23, 1);
        nk_label_colored(ctx, "RENDERER SAFETY", NK_TEXT_LEFT, nk_rgb(105, 125, 143));
        const char* rn = SDL_GetRendererName(renderer);
        std::string active = std::string("Active: ") + (rn ? rn : "unknown");
        nk_layout_row_dynamic(ctx, 24, 1);
        nk_label(ctx, active.c_str(), NK_TEXT_LEFT);
        nk_label(ctx, "The editor never creates an SDL OpenGL context.", NK_TEXT_LEFT);
        nk_label(ctx, "Fallback chain is native renderer -> software renderer.", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 18, 1);
        nk_spacing(ctx, 1);
        nk_layout_row_dynamic(ctx, 23, 1);
        nk_label_colored(ctx, "OVERRIDE", NK_TEXT_LEFT, nk_rgb(105, 125, 143));
        nk_layout_row_dynamic(ctx, 22, 1);
        nk_label(ctx, "Set DRAYVEN_UI_RENDERER=software to force CPU rendering.", NK_TEXT_LEFT);
#ifdef _WIN32
        nk_label(ctx, "Windows default: Direct3D 12 -> Direct3D 11 -> software", NK_TEXT_LEFT);
#elif defined(__APPLE__)
        nk_label(ctx, "macOS default: Metal -> software", NK_TEXT_LEFT);
#else
        nk_label(ctx, "Linux default: Vulkan -> software", NK_TEXT_LEFT);
#endif
        nk_layout_row_dynamic(ctx, 18, 1);
        nk_spacing(ctx, 1);
        nk_layout_row_dynamic(ctx, 23, 1);
        nk_label_colored(ctx, "ABOUT", NK_TEXT_LEFT, nk_rgb(105, 125, 143));
        nk_layout_row_dynamic(ctx, 22, 1);
        nk_label(ctx, "Drayven Editor / lightweight immediate UI", NK_TEXT_LEFT);
        nk_label(ctx, "No ImGui. No RmlUi. No editor GL3 dependency.", NK_TEXT_LEFT);
        (void)h;
    }

    void drawWorkspace(int width, int height) {
        const int bodyH = height - kTitleHeight - kStatusHeight;
        const int centerW = width - kSideWidth - kInspectorWidth;
        if (centerW < 300 || bodyH < 250) return;
        if (nk_begin(ctx, "drayven-workspace",
                     nk_rect(kSideWidth, kTitleHeight, static_cast<float>(centerW), static_cast<float>(bodyH)),
                     NK_WINDOW_NO_SCROLLBAR)) {
            const float usableW = static_cast<float>(centerW - 20);
            const float usableH = static_cast<float>(bodyH - 20);
            switch (workspace) {
            case Workspace::Dashboard: drawDashboard(usableW, usableH); break;
            case Workspace::Scene: drawScene(usableH); break;
            case Workspace::Scripts: drawScripts(usableH); break;
            case Workspace::UiCreator: drawUiCreator(usableW, usableH); break;
            case Workspace::Build: drawBuild(usableH); break;
            case Workspace::Settings: drawSettings(usableH); break;
            }
        }
        nk_end(ctx);
    }

    void frame() {
        int width = 0, height = 0;
        SDL_GetWindowSize(window, &width, &height);
        SDL_SetRenderDrawColor(renderer, 10, 13, 18, 255);
        SDL_RenderClear(renderer);
        drawTitleBar(width);
        drawSidebar(height);
        drawInspector(width, height);
        drawWorkspace(width, height);
        drawStatus(width, height);
        uiBackend.render();
        SDL_RenderPresent(renderer);
    }

    int run() {
        if (!init()) {
            shutdown();
            return 1;
        }
        while (running) {
            uiBackend.beginInput();
            SDL_Event ev{};
            while (SDL_PollEvent(&ev)) {
                if (ev.type == SDL_EVENT_QUIT || ev.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) running = false;
                uiBackend.handleEvent(ev);
            }
            uiBackend.endInput();
            frame();
            SDL_Delay(1);
        }
        shutdown();
        return 0;
    }
};

EditorApp::EditorApp() : m_impl(std::make_unique<Impl>()) {}
EditorApp::~EditorApp() = default;
int EditorApp::run() { return m_impl->run(); }
}
