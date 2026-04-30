#pragma once
#include <cmath>
#include "Framebuffer.h"
#include "../Math/Mat4.h"
#include <immintrin.h>

struct ScreenVert { float sx, sy, depth; };

inline ScreenVert projectVertex(const Vec3& v, const Mat4& mvp) {
    Vec4 in;
    in.x = v.x; in.y = v.y; in.z = v.z; in.w = 1.0f;
    Vec4 clip = mvp * in;
    ScreenVert sv;
    sv.sx = -1; sv.sy = -1; sv.depth = 1e9f;
    if (clip.w <= 0) return sv;
    float ndcX = clip.x / clip.w;
    float ndcY = clip.y / clip.w;
    sv.depth = clip.z / clip.w;
    sv.sx = (ndcX + 1.0f) * 0.5f * WIDTH;
    sv.sy = (1.0f - ndcY) * 0.5f * HEIGHT;
    return sv;
}

inline void drawTriangle(Framebuffer& fb,
    float x0, float y0, float z0,
    float x1, float y1, float z1,
    float x2, float y2, float z2,
    COLORREF color)
{
    int minX = max(0, (int)min(x0, min(x1, x2)));
    int maxX = min(WIDTH - 1, (int)max(x0, max(x1, x2)));
    int minY = max(0, (int)min(y0, min(y1, y2)));
    int maxY = min(HEIGHT - 1, (int)max(y0, max(y1, y2)));

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            float denom = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2);
            if (fabs(denom) < 1e-6f) continue;
            float w0 = ((y1 - y2) * (x - x2) + (x2 - x1) * (y - y2)) / denom;
            float w1 = ((y2 - y0) * (x - x2) + (x0 - x2) * (y - y2)) / denom;
            float w2 = 1.0f - w0 - w1;
            if (w0 < 0 || w1 < 0 || w2 < 0) continue;

            float depth = w0 * z0 + w1 * z1 + w2 * z2;
            if (depth >= fb.depth[y][x]) continue;
            fb.depth[y][x] = depth;
            fb.setPixel(x, y, color);
        }
    }
}

inline void drawTriangleGouraud(Framebuffer& fb,
    float x0, float y0, float z0, COLORREF c0,
    float x1, float y1, float z1, COLORREF c1,
    float x2, float y2, float z2, COLORREF c2)
{
    int minX = max(0, (int)min(x0, min(x1, x2)));
    int maxX = min(WIDTH - 1, (int)max(x0, max(x1, x2)));
    int minY = max(0, (int)min(y0, min(y1, y2)));
    int maxY = min(HEIGHT - 1, (int)max(y0, max(y1, y2)));

    // denom 삼각형당 1회만 계산
    float denom = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2);
    if (fabs(denom) < 1e-6f) return;
    float invDenom = 1.0f / denom;

    // 색상 채널 미리 추출
    float r0 = GetRValue(c0), g0 = GetGValue(c0), b0 = GetBValue(c0);
    float r1 = GetRValue(c1), g1 = GetGValue(c1), b1 = GetBValue(c1);
    float r2 = GetRValue(c2), g2 = GetGValue(c2), b2 = GetBValue(c2);

    // SIMD: x를 4개씩 처리
    __m128 vInvDenom = _mm_set1_ps(invDenom);
    __m128 vX0 = _mm_set1_ps(x0), vX1 = _mm_set1_ps(x1), vX2 = _mm_set1_ps(x2);
    __m128 vY1Y2 = _mm_set1_ps(y1 - y2);
    __m128 vX2X1 = _mm_set1_ps(x2 - x1);
    __m128 vY2Y0 = _mm_set1_ps(y2 - y0);
    __m128 vX0X2 = _mm_set1_ps(x0 - x2);
    __m128 vX2f = _mm_set1_ps(x2);
    __m128 vY2f = _mm_set1_ps(y2);

    for (int y = minY; y <= maxY; y++) {
        __m128 vY = _mm_set1_ps((float)y);

        int x = minX;
        for (; x <= maxX - 3; x += 4) {
            // [x, x+1, x+2, x+3]
            __m128 vX = _mm_set_ps((float)(x + 3), (float)(x + 2), (float)(x + 1), (float)x);

            // w0 = ((y1-y2)*(x-x2) + (x2-x1)*(y-y2)) * invDenom
            __m128 vW0 = _mm_mul_ps(
                _mm_add_ps(
                    _mm_mul_ps(vY1Y2, _mm_sub_ps(vX, vX2f)),
                    _mm_mul_ps(vX2X1, _mm_sub_ps(vY, vY2f))
                ), vInvDenom);

            // w1 = ((y2-y0)*(x-x2) + (x0-x2)*(y-y2)) * invDenom
            __m128 vW1 = _mm_mul_ps(
                _mm_add_ps(
                    _mm_mul_ps(vY2Y0, _mm_sub_ps(vX, vX2f)),
                    _mm_mul_ps(vX0X2, _mm_sub_ps(vY, vY2f))
                ), vInvDenom);

            __m128 vW2 = _mm_sub_ps(_mm_sub_ps(_mm_set1_ps(1.0f), vW0), vW1);

            // 삼각형 내부 판별
            __m128 zero = _mm_setzero_ps();
            __m128 simdMask = _mm_and_ps(
                _mm_and_ps(_mm_cmpge_ps(vW0, zero), _mm_cmpge_ps(vW1, zero)),
                _mm_cmpge_ps(vW2, zero));
            int maskBits = _mm_movemask_ps(simdMask);
            if (maskBits == 0) continue;

            // w 값 스칼라로 추출
            float w0[4], w1[4], w2[4];
            _mm_storeu_ps(w0, vW0);
            _mm_storeu_ps(w1, vW1);
            _mm_storeu_ps(w2, vW2);

            for (int i = 0; i < 4; i++) {
                if (!(maskBits & (1 << i))) continue;

                float depth = w0[i] * z0 + w1[i] * z1 + w2[i] * z2;
                if (depth >= fb.depth[y][x + i]) continue;
                fb.depth[y][x + i] = depth;

                int r = (int)(w0[i] * r0 + w1[i] * r1 + w2[i] * r2);
                int g = (int)(w0[i] * g0 + w1[i] * g1 + w2[i] * g2);
                int b = (int)(w0[i] * b0 + w1[i] * b1 + w2[i] * b2);
                fb.setPixel(x + i, y, RGB(r, g, b));
            }
        }

        // 나머지 픽셀 스칼라 처리
        for (; x <= maxX; x++) {
            float w0s = ((y1 - y2) * (x - x2) + (x2 - x1) * (y - y2)) * invDenom;
            float w1s = ((y2 - y0) * (x - x2) + (x0 - x2) * (y - y2)) * invDenom;
            float w2s = 1.0f - w0s - w1s;
            if (w0s < 0 || w1s < 0 || w2s < 0) continue;

            float depth = w0s * z0 + w1s * z1 + w2s * z2;
            if (depth >= fb.depth[y][x]) continue;
            fb.depth[y][x] = depth;

            int r = (int)(w0s * r0 + w1s * r1 + w2s * r2);
            int g = (int)(w0s * g0 + w1s * g1 + w2s * g2);
            int b = (int)(w0s * b0 + w1s * b1 + w2s * b2);
            fb.setPixel(x, y, RGB(r, g, b));
        }
    }
}