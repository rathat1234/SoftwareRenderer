#pragma once
#include "Vec3.h"

// ============================================================
// Mat4.h - 4x4 행렬 자료구조 및 변환 행렬 생성 함수
// ============================================================

// 4x4 행렬 (MVP 변환, 카메라, 조명 등에 사용)
struct Mat4 {
    float m[4][4];

    // 기본 생성자: 모든 원소 0으로 초기화
    Mat4() {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                m[i][j] = 0;
    }

    // 행렬 곱셈 (변환 합성: proj * view * model)
    Mat4 operator*(const Mat4& o) const {
        Mat4 r;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                for (int k = 0; k < 4; k++)
                    r.m[i][j] += m[i][k] * o.m[k][j];
        return r;
    }

    // 행렬 * 벡터 (버텍스를 클립 공간으로 변환)
    Vec4 operator*(const Vec4& v) const {
        Vec4 r;
        r.x = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * v.w;
        r.y = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * v.w;
        r.z = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * v.w;
        r.w = m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] * v.w;
        return r;
    }
};

// 단위 행렬 생성
inline Mat4 makeIdentity() {
    Mat4 m;
    m.m[0][0] = m.m[1][1] = m.m[2][2] = m.m[3][3] = 1.0f;
    return m;
}

// Y축 회전 행렬 (마우스 좌우 회전)
inline Mat4 rotateY(float a) {
    Mat4 m = makeIdentity();
    m.m[0][0] = cosf(a); m.m[0][2] = sinf(a);
    m.m[2][0] = -sinf(a); m.m[2][2] = cosf(a);
    return m;
}

// X축 회전 행렬 (마우스 상하 회전)
inline Mat4 rotateX(float a) {
    Mat4 m = makeIdentity();
    m.m[1][1] = cosf(a); m.m[1][2] = -sinf(a);
    m.m[2][1] = sinf(a); m.m[2][2] = cosf(a);
    return m;
}

// 이동 행렬
inline Mat4 makeTranslate(float tx, float ty, float tz) {
    Mat4 m = makeIdentity();
    m.m[0][3] = tx; m.m[1][3] = ty; m.m[2][3] = tz;
    return m;
}

// 스케일 행렬
inline Mat4 makeScale(float sx, float sy, float sz) {
    Mat4 m = makeIdentity();
    m.m[0][0] = sx; m.m[1][1] = sy; m.m[2][2] = sz;
    return m;
}

// 원근 투영 행렬 (3D -> 2D, FOV/종횡비/Near/Far 클리핑)
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

// 뷰 행렬 생성 (카메라/광원 시점 설정)
inline Mat4 lookAt(Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 f = normalize(subtract(center, eye));
    Vec3 r = normalize(cross(f, up));
    Vec3 u = cross(r, f);

    Mat4 m = {};
    m.m[0][0] = r.x; m.m[0][1] = r.y; m.m[0][2] = r.z; m.m[0][3] = -dot(r, eye);
    m.m[1][0] = u.x; m.m[1][1] = u.y; m.m[1][2] = u.z; m.m[1][3] = -dot(u, eye);
    m.m[2][0] = -f.x; m.m[2][1] = -f.y; m.m[2][2] = -f.z; m.m[2][3] = dot(f, eye);
    m.m[3][3] = 1.0f;
    return m;
}