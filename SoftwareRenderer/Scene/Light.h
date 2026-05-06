#pragma once
#include "../Math/Vec3.h"
#include <algorithm>
using namespace std;

// ============================================================
// Light.h - 방향광 (Directional Light) 정의
// ============================================================

class Light {
public:
    // 정면에서 직접
    Vec3 direction = { 0.0f, 0.0f, -1.0f };

    // 법선 벡터와 광원 방향의 내적으로 조명 강도 계산 (Lambertian)
    float getIntensity(Vec3 normal) const {
        float i = dot(normalize(direction), normal);
        return max(0.1f, i);  // 최소 ambient 0.1 보장
    }

    // 조명 강도를 색상에 적용 (현재는 람버트로 대체, 레거시 유지)
    COLORREF applyLight(COLORREF baseColor, float intensity) const {
        intensity = max(0.1f, min(1.0f, intensity));
        int r = (int)(GetRValue(baseColor) * intensity);
        int g = (int)(GetGValue(baseColor) * intensity);
        int b = (int)(GetBValue(baseColor) * intensity);
        return RGB(r, g, b);
    }
};