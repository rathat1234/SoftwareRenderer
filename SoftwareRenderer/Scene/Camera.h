#pragma once
#include "../Math/Mat4.h"

class Camera {
public:
    float posZ = -5.0f;
    float rotX = 0.0f;
    float rotY = 0.0f;

    Mat4 getViewMatrix() const {
        return makeTranslate(0, 0, posZ);
    }
    Mat4 getProjectionMatrix(float fov, float aspect) const {
        return makePerspective(fov, aspect, 0.1f, 100.0f);
    }
};