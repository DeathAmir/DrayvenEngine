#include "RmlBackend.hpp"
#include "RmlUi_Platform_SDL.h"
#include "RmlUi_Renderer_GL3.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Math.h>
#include <RmlUi/Core/Profiling.h>
#include <SDL3/SDL.h>
#include <memory>

namespace drayven::editor_backend {
namespace {
struct Data {
    SystemInterface_SDL system;
    RenderInterface_GL3 renderer;
    SDL_Window* window{};
    SDL_GLContext gl{};
    bool running{true};
};
std::unique_ptr<Data> data;

SDL_HitTestResult SDLCALL hitTest(SDL_Window* window, const SDL_Point* p, void*) {
    int w = 0, h = 0;
    SDL_GetWindowSize(window, &w, &h);
    constexpr int edge = 6;
    constexpr int title = 48;
    constexpr int controls = 156;

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
    if (p->y < title && p->x < w - controls) return SDL_HITTEST_DRAGGABLE;
    return SDL_HITTEST_NORMAL;
}
}

bool Initialize(const char* title, int width, int height) {
    if (data) return true;
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
        Rml::Log::Message(Rml::Log::LT_ERROR, "SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, title);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, width);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, height);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN, true);
    SDL_Window* window = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);
    if (!window) {
        SDL_Quit();
        return false;
    }

    auto gl = SDL_GL_CreateContext(window);
    if (!gl) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }
    SDL_GL_MakeCurrent(window, gl);
    SDL_GL_SetSwapInterval(1);
    SDL_SetWindowMinimumSize(window, 960, 620);
    SDL_SetWindowHitTest(window, hitTest, nullptr);

    data = std::make_unique<Data>();
    if (!data->renderer) {
        data.reset();
        SDL_GL_DestroyContext(gl);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }
    data->window = window;
    data->gl = gl;
    data->system.SetWindow(window);
    data->renderer.SetViewport(width, height);
    return true;
}

void Shutdown() {
    if (!data) return;
    SDL_SetWindowHitTest(data->window, nullptr, nullptr);
    SDL_GL_DestroyContext(data->gl);
    SDL_DestroyWindow(data->window);
    data.reset();
    SDL_Quit();
}

Rml::SystemInterface* GetSystemInterface() { return data ? &data->system : nullptr; }
Rml::RenderInterface* GetRenderInterface() { return data ? &data->renderer : nullptr; }
SDL_Window* Window() { return data ? data->window : nullptr; }

bool ProcessEvents(Rml::Context* context) {
    if (!data || !context) return false;
    bool result = data->running;
    data->running = true;

    SDL_Event ev;
    bool hasEvent = SDL_PollEvent(&ev);
    while (hasEvent) {
        bool propagate = true;
        switch (ev.type) {
        case SDL_EVENT_QUIT:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            propagate = false;
            result = false;
            break;
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            data->renderer.SetViewport(ev.window.data1, ev.window.data2);
            break;
        default:
            break;
        }
        if (propagate) RmlSDL::InputEventHandler(context, data->window, ev);
        hasEvent = SDL_PollEvent(&ev);
    }
    return result;
}

void RequestExit() {
    if (data) data->running = false;
}

void BeginFrame() {
    if (!data) return;
    data->renderer.Clear();
    data->renderer.BeginFrame();
}

void PresentFrame() {
    if (!data) return;
    data->renderer.EndFrame();
    SDL_GL_SwapWindow(data->window);
    RMLUI_FrameMark;
}
}
