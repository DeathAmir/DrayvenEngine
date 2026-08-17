#pragma once
#include "drayven/Math.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace drayven {
using EntityId = std::uint64_t;
struct SpriteComponent { std::string texture; Color tint{}; };
struct MeshComponent { std::string mesh; std::string material; };
struct CameraComponent { float fov{60.f}; bool primary{false}; };
struct AudioSourceComponent { std::string clip; float volume{1.f}; bool loop{false}; };
struct ScriptComponent { std::string script; };

struct Entity {
    EntityId id{};
    std::string name{"Entity"};
    Transform transform{};
    bool hasSprite{false}; SpriteComponent sprite{};
    bool hasMesh{false}; MeshComponent mesh{};
    bool hasCamera{false}; CameraComponent camera{};
    bool hasAudio{false}; AudioSourceComponent audio{};
    bool hasScript{false}; ScriptComponent script{};
};

class Scene {
public:
    Entity& createEntity(std::string name = "Entity");
    bool destroyEntity(EntityId id);
    Entity* find(EntityId id);
    const std::vector<Entity>& entities() const { return m_entities; }
    std::vector<Entity>& entities() { return m_entities; }
    void clear();
private:
    EntityId m_next{1};
    std::vector<Entity> m_entities;
};
}
