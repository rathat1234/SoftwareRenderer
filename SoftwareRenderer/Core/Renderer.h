#pragma once
#include <cmath>
#include "Framebuffer.h"
#include "../Math/Mat4.h"
#include <immintrin.h>  // SSE2/AVX2 SIMD

// ============================================================
// Renderer.h - 래스터라이제이션 핵심 함수
// ============================================================

// 스크린 공간 버텍스 (투영 변환 결과)
struct ScreenVert { float sx, sy, depth; };

// 버텍스를 MVP 행렬로 변환 후 스크린 좌표로 투영
// NDC(-1~1) -> 스크린 좌표(0~WIDTH, 0~HEIGHT)
inline ScreenVert projectVertex(const Vec3& v, const Mat4& mvp) {
    Vec4 in;
    in.x = v.x; in.y = v.y; in.z = v.z; in.w = 1.0f;
    Vec4 clip = mvp * in;
    ScreenVert sv;
    sv.sx = -1; sv.sy = -1; sv.depth = 1e9f;
    if (clip.w <= 0) return sv;  // 카메라 뒤 버텍스 제거
    float ndcX = clip.x / clip.w;
    float ndcY = clip.y / clip.w;
    sv.depth = clip.z / clip.w;
    sv.sx = (ndcX + 1.0f) * 0.5f * WIDTH;
    sv.sy = (1.0f - ndcY) * 0.5f * HEIGHT;
    return sv;
}

// 단색 삼각형 래스터라이제이션 (바리센트릭 좌표 기반)
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

// Shadow Map 생성용 래스터라이제이션 (광원 시점 깊이값만 기록)
inline void drawTriangleShadow(Framebuffer& fb,
    float x0, float y0, float z0,
    float x1, float y1, float z1,
    float x2, float y2, float z2)
{
    int minX = max(0, (int)min(x0, min(x1, x2)));
    int maxX = min(WIDTH - 1, (int)max(x0, max(x1, x2)));
    int minY = max(0, (int)min(y0, min(y1, y2)));
    int maxY = min(HEIGHT - 1, (int)max(y0, max(y1, y2)));

    float denom = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2);
    if (fabs(denom) < 1e-6f) return;
    float invDenom = 1.0f / denom;

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            float w0 = ((y1 - y2) * (x - x2) + (x2 - x1) * (y - y2)) * invDenom;
            float w1 = ((y2 - y0) * (x - x2) + (x0 - x2) * (y - y2)) * invDenom;
            float w2 = 1.0f - w0 - w1;
            if (w0 < 0 || w1 < 0 || w2 < 0) continue;

            float depth = w0 * z0 + w1 * z1 + w2 * z2;
            if (depth < fb.shadowMap[y][x])
                fb.shadowMap[y][x] = depth;  // 최소 깊이값 저장
        }
    }
}
// Gouraud Shading + SIMD 최적화 래스터라이제이션
// - SSE2로 x축 4픽셀 병렬 처리
// - startY/endY로 멀티스레드 타일 분할
// - Shadow Map 비교 코드 포함 (Shadow Map 비활성화 시 무영향)
inline void drawTriangleGouraud(Framebuffer& fb,
    float x0, float y0, float z0, COLORREF c0,
    float x1, float y1, float z1, COLORREF c1,
    float x2, float y2, float z2, COLORREF c2,
    float lx0, float ly0, float lz0,
    float lx1, float ly1, float lz1,
    float lx2, float ly2, float lz2,
    int startY, int endY)
{
    int minX = max(0, (int)min(x0, min(x1, x2)));
    int maxX = min(WIDTH - 1, (int)max(x0, max(x1, x2)));
    int minY = max(0, (int)min(y0, min(y1, y2)));
    int maxY = min(HEIGHT - 1, (int)max(y0, max(y1, y2)));

    float denom = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2);
    if (fabs(denom) < 1e-6f) return;
    float invDenom = 1.0f / denom;

    // 색상 채널 미리 추출 (반복 호출 방지)
    float r0 = GetRValue(c0), g0 = GetGValue(c0), b0 = GetBValue(c0);
    float r1 = GetRValue(c1), g1 = GetGValue(c1), b1 = GetBValue(c1);
    float r2 = GetRValue(c2), g2 = GetGValue(c2), b2 = GetBValue(c2);

    // SIMD 상수 세팅 (루프 밖에서 1회만)
    __m128 vInvDenom = _mm_set1_ps(invDenom);
    __m128 vY1Y2 = _mm_set1_ps(y1 - y2);
    __m128 vX2X1 = _mm_set1_ps(x2 - x1);
    __m128 vY2Y0 = _mm_set1_ps(y2 - y0);
    __m128 vX0X2 = _mm_set1_ps(x0 - x2);
    __m128 vX2f = _mm_set1_ps(x2);
    __m128 vY2f = _mm_set1_ps(y2);

    // 멀티스레드 타일 Y 범위 클램핑
    int minYc = max(minY, startY);
    int maxYc = min(maxY, endY - 1);

    for (int y = minYc; y <= maxYc; y++) {
        __m128 vY = _mm_set1_ps((float)y);

        int x = minX;

        // SIMD: x 4개씩 병렬 처리
        for (; x <= maxX - 3; x += 4) {
            __m128 vX = _mm_set_ps((float)(x + 3), (float)(x + 2), (float)(x + 1), (float)x);

            // 바리센트릭 좌표 계산 (4픽셀 동시)
            __m128 vW0 = _mm_mul_ps(
                _mm_add_ps(
                    _mm_mul_ps(vY1Y2, _mm_sub_ps(vX, vX2f)),
                    _mm_mul_ps(vX2X1, _mm_sub_ps(vY, vY2f))
                ), vInvDenom);

            __m128 vW1 = _mm_mul_ps(
                _mm_add_ps(
                    _mm_mul_ps(vY2Y0, _mm_sub_ps(vX, vX2f)),
                    _mm_mul_ps(vX0X2, _mm_sub_ps(vY, vY2f))
                ), vInvDenom);

            __m128 vW2 = _mm_sub_ps(_mm_sub_ps(_mm_set1_ps(1.0f), vW0), vW1);

            // 삼각형 내부 판별 (w0>=0 && w1>=0 && w2>=0)
            __m128 zero = _mm_setzero_ps();
            __m128 simdMask = _mm_and_ps(
                _mm_and_ps(_mm_cmpge_ps(vW0, zero), _mm_cmpge_ps(vW1, zero)),
                _mm_cmpge_ps(vW2, zero));
            int maskBits = _mm_movemask_ps(simdMask);
            if (maskBits == 0) continue;

            float w0[4], w1[4], w2[4];
            _mm_storeu_ps(w0, vW0);
            _mm_storeu_ps(w1, vW1);
            _mm_storeu_ps(w2, vW2);

            for (int i = 0; i < 4; i++) {
                if (!(maskBits & (1 << i))) continue;

                // Z-buffer 테스트
                float depth = w0[i] * z0 + w1[i] * z1 + w2[i] * z2;
                if (depth >= fb.depth[y][x + i]) continue;
                fb.depth[y][x + i] = depth;

                // Gouraud 색상 보간
                int r = (int)(w0[i] * r0 + w1[i] * r1 + w2[i] * r2);
                int g = (int)(w0[i] * g0 + w1[i] * g1 + w2[i] * g2);
                int b = (int)(w0[i] * b0 + w1[i] * b1 + w2[i] * b2);

                // Shadow Map 비교 (광원 공간 좌표로 깊이 비교)
                float lx = w0[i] * lx0 + w1[i] * lx1 + w2[i] * lx2;
                float ly = w0[i] * ly0 + w1[i] * ly1 + w2[i] * ly2;
                float lz = w0[i] * lz0 + w1[i] * lz1 + w2[i] * lz2;
                int sx = (int)lx, sy = (int)ly;
                if (sx >= 0 && sx < WIDTH && sy >= 0 && sy < HEIGHT)
                    if (lz > fb.shadowMap[sy][sx] + 0.01f)
                    {
                        r /= 3; g /= 3; b /= 3;
                    }  // 그림자 = 1/3 밝기

                fb.setPixel(x + i, y, RGB(r, g, b));
            }
        }

        // 나머지 픽셀 스칼라 처리 (4의 배수 나머지)
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

            // Shadow Map 비교
            float lx = w0s * lx0 + w1s * lx1 + w2s * lx2;
            float ly = w0s * ly0 + w1s * ly1 + w2s * ly2;
            float lz = w0s * lz0 + w1s * lz1 + w2s * lz2;
            int sx = (int)lx, sy = (int)ly;
            if (sx >= 0 && sx < WIDTH && sy >= 0 && sy < HEIGHT)
                if (lz > fb.shadowMap[sy][sx] + 0.01f)
                {
                    r /= 3; g /= 3; b /= 3;
                }

            fb.setPixel(x, y, RGB(r, g, b));
        }
    }
}