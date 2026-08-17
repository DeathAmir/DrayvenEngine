#include "drayven/Scene.hpp"
#include <algorithm>
namespace drayven {
Entity& Scene::createEntity(std::string name){ Entity e; e.id=m_next++; e.name=std::move(name); m_entities.push_back(std::move(e)); return m_entities.back(); }
bool Scene::destroyEntity(EntityId id){ auto it=std::remove_if(m_entities.begin(),m_entities.end(),[&](auto& e){return e.id==id;}); if(it==m_entities.end()) return false; m_entities.erase(it,m_entities.end()); return true; }
Entity* Scene::find(EntityId id){ for(auto& e:m_entities) if(e.id==id) return &e; return nullptr; }
void Scene::clear(){ m_entities.clear(); m_next=1; }
}
