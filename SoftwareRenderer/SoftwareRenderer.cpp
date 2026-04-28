#include <windows.h>
#include <algorithm>
using namespace std;

// 프레임버퍼 설정
const int WIDTH = 800;
const int HEIGHT = 600;
COLORREF framebuffer[HEIGHT][WIDTH];

// 픽셀 하나 찍는 함수
void setPixel(int x, int y, COLORREF color) {
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
        framebuffer[y][x] = color;
}

void drawLine(int x0, int y0, int x1, int y1, COLORREF color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        setPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

// 바리센트릭 좌표로 점이 삼각형 안에 있는지 확인
bool isInsideTriangle(int px, int py,
    int x0, int y0, int x1, int y1, int x2, int y2,
    float& w0, float& w1, float& w2) {

    float denom = (float)((y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2));
    w0 = ((y1 - y2) * (px - x2) + (x2 - x1) * (py - y2)) / denom;
    w1 = ((y2 - y0) * (px - x2) + (x0 - x2) * (py - y2)) / denom;
    w2 = 1.0f - w0 - w1;

    return (w0 >= 0 && w1 >= 0 && w2 >= 0);
}

// 삼각형 채우기
void drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2,
    COLORREF c0, COLORREF c1, COLORREF c2) {

    // 바운딩 박스 계산
    int minX = max(0, min(x0, min(x1, x2)));
    int maxX = min(WIDTH - 1, max(x0, max(x1, x2)));
    int minY = max(0, min(y0, min(y1, y2)));
    int maxY = min(HEIGHT - 1, max(y0, max(y1, y2)));

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            float w0, w1, w2;
            if (isInsideTriangle(x, y, x0, y0, x1, y1, x2, y2, w0, w1, w2)) {
                // 세 꼭짓점 색상을 바리센트릭 좌표로 보간
                int r = (int)(w0 * GetRValue(c0) + w1 * GetRValue(c1) + w2 * GetRValue(c2));
                int g = (int)(w0 * GetGValue(c0) + w1 * GetGValue(c1) + w2 * GetGValue(c2));
                int b = (int)(w0 * GetBValue(c0) + w1 * GetBValue(c1) + w2 * GetBValue(c2));
                setPixel(x, y, RGB(r, g, b));
            }
        }
    }
}




// 윈도우 메시지 처리
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // 프레임버퍼 → 화면에 그리기
        for (int y = 0; y < HEIGHT; y++)
            for (int x = 0; x < WIDTH; x++)
                SetPixel(hdc, x, y, framebuffer[y][x]);

        EndPaint(hwnd, &ps);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // 프레임버퍼 초기화 (검정)
    memset(framebuffer, 0, sizeof(framebuffer));

    // 테스트: 색상 보간 삼각형 2개
    drawTriangle(
        400, 100,   // 꼭짓점 0 (빨강)
        150, 500,   // 꼭짓점 1 (초록)
        650, 500,   // 꼭짓점 2 (파랑)
        RGB(255, 0, 0), RGB(0, 255, 0), RGB(0, 0, 255)
    );
    drawTriangle(
        400, 550,   // 꼭짓점 0 (노랑)
        200, 200,   // 꼭짓점 1 (하늘)
        600, 200,   // 꼭짓점 2 (분홍)
        RGB(255, 255, 0), RGB(0, 255, 255), RGB(255, 0, 255)
    );

    // 윈도우 생성
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"SoftwareRenderer";
    RegisterClass(&wc);

    HWND hwnd = CreateWindow(L"SoftwareRenderer", L"Software Renderer",
        WS_OVERLAPPEDWINDOW, 100, 100, WIDTH, HEIGHT,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);

    // 메시지 루프
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}