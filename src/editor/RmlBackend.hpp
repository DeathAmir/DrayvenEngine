#pragma once
#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/SystemInterface.h>

struct SDL_Window;

namespace Rml { class Context; }

namespace drayven::editor_backend {
bool Initialize(const char* title, int width, int height);
void Shutdown();
Rml::SystemInterface* GetSystemInterface();
Rml::RenderInterface* GetRenderInterface();
bool ProcessEvents(Rml::Context* context);
void RequestExit();
void BeginFrame();
void PresentFrame();
SDL_Window* Window();
}
