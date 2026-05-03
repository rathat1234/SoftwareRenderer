#pragma once
#include <cmath>

// ============================================================
// Vec3.h - 3D/4D 벡터 자료구조 및 연산
// ============================================================

// 3차원 벡터 (위치, 방향, 법선 등에 사용)
struct Vec3 { float x, y, z; };

// 4차원 벡터 (동차 좌표계 클립 공간 변환에 사용)
struct Vec4 { float x, y, z, w; };

// 벡터 뺄셈: a - b
inline Vec3 subtract(Vec3 a, Vec3 b) {
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}

// 외적: 두 벡터에 수직인 벡터 반환 (법선 계산, Back-face Culling에 사용)
inline Vec3 cross(Vec3 a, Vec3 b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

// 내적: 두 벡터의 코사인 유사도 (조명 계산, Back-face Culling에 사용)
inline float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// 정규화: 길이를 1로 만든 단위 벡터 반환
inline Vec3 normalize(Vec3 v) {
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len < 1e-6f) return { 0, 0, 0 };
    return { v.x / len, v.y / len, v.z / len };
}