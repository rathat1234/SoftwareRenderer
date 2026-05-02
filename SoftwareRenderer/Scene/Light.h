#pragma once
#include "../Math/Vec3.h"
#include <algorithm>
using namespace std;

class Light {
public:

    Vec3 direction = { 0.0f, -1.0f, -1.0f };

    float getIntensity(Vec3 normal) const {
        float i = dot(normalize(direction), normal);
        return max(0.1f, i);
    }

    COLORREF applyLight(COLORREF baseColor, float intensity) const {
        intensity = max(0.1f, min(1.0f, intensity));
        int r = (int)(GetRValue(baseColor) * intensity);
        int g = (int)(GetGValue(baseColor) * intensity);
        int b = (int)(GetBValue(baseColor) * intensity);
        return RGB(r, g, b);
    }
};