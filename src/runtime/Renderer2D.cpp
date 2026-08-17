#include "drayven/Renderer.hpp"
namespace drayven {
void Renderer2D::begin(){m_drawCalls=0;}
void Renderer2D::drawQuad(Vec2,Vec2,Color){++m_drawCalls;}
void Renderer2D::end(){}
}
