#pragma once
#include <cmath>
#include "Framebuffer.h"
#include "../Math/Mat4.h"

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

            int r = (int)(w0 * GetRValue(c0) + w1 * GetRValue(c1) + w2 * GetRValue(c2));
            int g = (int)(w0 * GetGValue(c0) + w1 * GetGValue(c1) + w2 * GetGValue(c2));
            int b = (int)(w0 * GetBValue(c0) + w1 * GetBValue(c1) + w2 * GetBValue(c2));
            fb.setPixel(x, y, RGB(r, g, b));
        }
    }
}