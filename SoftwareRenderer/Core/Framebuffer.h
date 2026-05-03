#pragma once
#include <windows.h>
#include <algorithm>

// ============================================================
// Framebuffer.h - CPU 렌더링 결과를 저장하는 버퍼 관리
// ============================================================

// 렌더링 해상도
const int WIDTH = 800;
const int HEIGHT = 600;

class Framebuffer {
public:
    HBITMAP   hBitmap = nullptr;  // DIBSection 비트맵 핸들
    COLORREF* pixels = nullptr;  // 픽셀 색상 버퍼 (CPU 직접 접근)
    float     depth[HEIGHT][WIDTH];      // Z-buffer (깊이 판별용)
    float     shadowMap[HEIGHT][WIDTH];  // Shadow Map 깊이 버퍼 (광원 시점)

    // DIBSection 초기화 - CPU가 직접 쓸 수 있는 픽셀 버퍼 생성
    void init(HWND hwnd) {
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = WIDTH;
        bmi.bmiHeader.biHeight = -HEIGHT;  // 음수: top-down 방향
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        HDC hdc = GetDC(hwnd);
        hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS,
            (void**)&pixels, NULL, 0);
        ReleaseDC(hwnd, hdc);
    }

    // 프레임 시작 시 픽셀 버퍼와 깊이 버퍼 초기화
    void clear() {
        memset(pixels, 0, WIDTH * HEIGHT * sizeof(COLORREF));
        std::fill(&depth[0][0], &depth[0][0] + WIDTH * HEIGHT, 1e9f);
    }

    // 픽셀 색상 쓰기 (DIBSection 메모리에 직접 접근)
    void setPixel(int x, int y, COLORREF color) {
        if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
            pixels[y * WIDTH + x] = color;
    }

    // Shadow Map 깊이 버퍼 초기화
    void clearShadowMap() {
        std::fill(&shadowMap[0][0], &shadowMap[0][0] + WIDTH * HEIGHT, 1e9f);
    }

    // 렌더링 결과를 화면에 출력 (BitBlt으로 고속 복사)
    void present(HDC hdc) {
        HDC memDC = CreateCompatibleDC(hdc);
        SelectObject(memDC, hBitmap);
        BitBlt(hdc, 0, 0, WIDTH, HEIGHT, memDC, 0, 0, SRCCOPY);
        DeleteDC(memDC);
    }
};