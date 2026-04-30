#pragma once
#include <cmath>

struct Vec3 { float x, y, z; };
struct Vec4 { float x, y, z, w; };

inline Vec3 subtract(Vec3 a, Vec3 b) {
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}
inline Vec3 cross(Vec3 a, Vec3 b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}
inline float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
inline Vec3 normalize(Vec3 v) {
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len < 1e-6f) return { 0,0,0 };
    return { v.x / len, v.y / len, v.z / len };
}