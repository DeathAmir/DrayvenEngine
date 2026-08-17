#pragma once
#include "drayven/Math.hpp"
#include <string>
#include <vector>

namespace drayven {
class Renderer2D {
public:
    void begin();
    void drawQuad(Vec2 position, Vec2 size, Color color={});
    void end();
    std::size_t drawCalls() const { return m_drawCalls; }
private:
    std::size_t m_drawCalls{};
};

class Renderer3D {
public:
    void begin(const Transform& camera);
    void drawCube(const Transform& transform, Color color={});
    void end();
    std::size_t drawCalls() const { return m_drawCalls; }
private:
    std::size_t m_drawCalls{};
};
}
