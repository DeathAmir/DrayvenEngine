#pragma once
#include "drayven/Project.hpp"
#include "drayven/Scene.hpp"
#include <memory>
#include <string>

struct SDL_Window;
struct SDL_GLContextState;

namespace drayven {
class Application {
public:
    Application();
    ~Application();
    bool init(const std::string& title, int width, int height, bool resizable=true);
    void shutdown();
    bool poll();
    void beginFrame(float r=0.07f, float g=0.08f, float b=0.10f, float a=1.f);
    void endFrame();
    SDL_Window* window() const { return m_window; }
    float deltaSeconds() const { return m_delta; }
private:
    SDL_Window* m_window{};
    void* m_gl{};
    bool m_running{false};
    std::uint64_t m_lastTicks{};
    float m_delta{0.f};
};
}
