#pragma once
#include "../Math/Mat4.h"

// ============================================================
// Camera.h - 카메라 상태 및 뷰/투영 행렬 생성
// ============================================================

class Camera {
public:
    float posZ = -5.0f;  // 카메라 Z 위치 (음수 = 화면 뒤로)
    float rotX = 0.0f;   // X축 회전각 (마우스 상하)
    float rotY = 0.0f;   // Y축 회전각 (마우스 좌우)

    // 뷰 행렬: 카메라 위치만큼 씬을 이동
    Mat4 getViewMatrix() const {
        return makeTranslate(0, 0, posZ);
    }

    // 투영 행렬: 원근 투영 (FOV, 종횡비, Near/Far 클리핑)
    Mat4 getProjectionMatrix(float fov, float aspect) const {
        return makePerspective(fov, aspect, 0.1f, 100.0f);
    }
};