#pragma once
#include <windows.h>
#include <algorithm>

const int WIDTH = 800;
const int HEIGHT = 600;

class Framebuffer {
public:
    HBITMAP   hBitmap = nullptr;
    COLORREF* pixels = nullptr;
    float     depth[HEIGHT][WIDTH];

    void init(HWND hwnd) {
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = WIDTH;
        bmi.bmiHeader.biHeight = -HEIGHT;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        HDC hdc = GetDC(hwnd);
        hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS,
            (void**)&pixels, NULL, 0);
        ReleaseDC(hwnd, hdc);
    }

    void clear() {
        memset(pixels, 0, WIDTH * HEIGHT * sizeof(COLORREF));
        std::fill(&depth[0][0], &depth[0][0] + WIDTH * HEIGHT, 1e9f);
    }

    void setPixel(int x, int y, COLORREF color) {
        if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
            pixels[y * WIDTH + x] = color;
    }

    void present(HDC hdc) {
        HDC memDC = CreateCompatibleDC(hdc);
        SelectObject(memDC, hBitmap);
        BitBlt(hdc, 0, 0, WIDTH, HEIGHT, memDC, 0, 0, SRCCOPY);
        DeleteDC(memDC);
    }
};