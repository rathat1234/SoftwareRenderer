#pragma once
#include <vector>
#include <unordered_map>
#include <cstdint>
#include "../Scene/Mesh.h"
#include "../Math/Vec3.h"

using EntityID = uint32_t;

// 컴포넌트들
struct TransformComponent {
    Vec3 position = { 0, 0, 0 };
    Vec3 rotation = { 0, 0, 0 };
    Vec3 scale = { 1, 1, 1 };
};

struct MeshComponent {
    int meshIndex = -1;  // 메시 배열 인덱스
};

struct LightComponent {
    Vec3 direction = { 0, -1, 0 };
    float intensity = 1.0f;
};

// 간단한 ECS World
class ECSWorld {
public:
    EntityID nextID = 0;

    std::vector<EntityID>                        entities;
    std::unordered_map<EntityID, TransformComponent> transforms;
    std::unordered_map<EntityID, MeshComponent>      meshes;
    std::unordered_map<EntityID, LightComponent>     lights;

    EntityID createEntity() {
        EntityID id = nextID++;
        entities.push_back(id);
        return id;
    }

    void addTransform(EntityID id, TransformComponent t) { transforms[id] = t; }
    void addMesh(EntityID id, MeshComponent m) { meshes[id] = m; }
    void addLight(EntityID id, LightComponent l) { lights[id] = l; }

    TransformComponent* getTransform(EntityID id) {
        auto it = transforms.find(id);
        return it != transforms.end() ? &it->second : nullptr;
    }
};