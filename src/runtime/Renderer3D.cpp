#include "drayven/Renderer.hpp"
namespace drayven {
void Renderer3D::begin(const Transform&){m_drawCalls=0;}
void Renderer3D::drawCube(const Transform&,Color){++m_drawCalls;}
void Renderer3D::end(){}
}
