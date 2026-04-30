#pragma once
#include "Vec3.h"

struct Mat4 {
    float m[4][4];
    Mat4() {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                m[i][j] = 0;
    }
    Mat4 operator*(const Mat4& o) const {
        Mat4 r;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                for (int k = 0; k < 4; k++)
                    r.m[i][j] += m[i][k] * o.m[k][j];
        return r;
    }
    Vec4 operator*(const Vec4& v) const {
        Vec4 r;
        r.x = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * v.w;
        r.y = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * v.w;
        r.z = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * v.w;
        r.w = m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] * v.w;
        return r;
    }
};

inline Mat4 makeIdentity() {
    Mat4 m;
    m.m[0][0] = m.m[1][1] = m.m[2][2] = m.m[3][3] = 1.0f;
    return m;
}
inline Mat4 rotateY(float a) {
    Mat4 m = makeIdentity();
    m.m[0][0] = cosf(a); m.m[0][2] = sinf(a);
    m.m[2][0] = -sinf(a); m.m[2][2] = cosf(a);
    return m;
}
inline Mat4 rotateX(float a) {
    Mat4 m = makeIdentity();
    m.m[1][1] = cosf(a); m.m[1][2] = -sinf(a);
    m.m[2][1] = sinf(a); m.m[2][2] = cosf(a);
    return m;
}
inline Mat4 makeTranslate(float tx, float ty, float tz) {
    Mat4 m = makeIdentity();
    m.m[0][3] = tx; m.m[1][3] = ty; m.m[2][3] = tz;
    return m;
}
inline Mat4 makeScale(float sx, float sy, float sz) {
    Mat4 m = makeIdentity();
    m.m[0][0] = sx; m.m[1][1] = sy; m.m[2][2] = sz;
    return m;
}
inline Mat4 makePerspective(float fov, float aspect, float zNear, float zFar) {
    Mat4 m;
    float f = 1.0f / tanf(fov / 2.0f);
    m.m[0][0] = f / aspect;
    m.m[1][1] = f;
    m.m[2][2] = (zFar + zNear) / (zNear - zFar);
    m.m[2][3] = (2.0f * zFar * zNear) / (zNear - zFar);
    m.m[3][2] = -1.0f;
    return m;
}