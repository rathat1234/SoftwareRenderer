#pragma once
#include <vector>
#include <unordered_map>
#include <cstdint>
#include "../Scene/Mesh.h"
#include "../Math/Vec3.h"

// ============================================================
// ECS.h - Entity Component System 구조
// Entity: 고유 ID / Component: 데이터 / System: render()에서 처리
// ============================================================

// 엔티티 고유 식별자
using EntityID = uint32_t;

// 위치/회전/스케일 컴포넌트
struct TransformComponent {
    Vec3 position = { 0, 0, 0 };
    Vec3 rotation = { 0, 0, 0 };
    Vec3 scale = { 1, 1, 1 };
};

// 메시 참조 컴포넌트 (메시 배열 인덱스로 참조)
struct MeshComponent {
    int meshIndex = -1;
};

// 방향광 컴포넌트
struct LightComponent {
    Vec3  direction = { 0, -1, 0 };
    float intensity = 1.0f;
};

// ECS World: 엔티티 생성 및 컴포넌트 관리
class ECSWorld {
public:
    EntityID nextID = 0;  // 다음 엔티티 ID (자동 증가)

    std::vector<EntityID>                            entities;
    std::unordered_map<EntityID, TransformComponent> transforms;
    std::unordered_map<EntityID, MeshComponent>      meshes;
    std::unordered_map<EntityID, LightComponent>     lights;

    // 새 엔티티 생성 후 ID 반환
    EntityID createEntity() {
        EntityID id = nextID++;
        entities.push_back(id);
        return id;
    }

    // 컴포넌트 추가
    void addTransform(EntityID id, TransformComponent t) { transforms[id] = t; }
    void addMesh(EntityID id, MeshComponent m) { meshes[id] = m; }
    void addLight(EntityID id, LightComponent l) { lights[id] = l; }

    // Transform 컴포넌트 조회 (없으면 nullptr 반환)
    TransformComponent* getTransform(EntityID id) {
        auto it = transforms.find(id);
        return it != transforms.end() ? &it->second : nullptr;
    }
};