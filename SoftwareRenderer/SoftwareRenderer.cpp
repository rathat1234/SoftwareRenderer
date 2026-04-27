#include <windows.h>

// 프레임버퍼 설정
const int WIDTH = 800;
const int HEIGHT = 600;
COLORREF framebuffer[HEIGHT][WIDTH];

// 픽셀 하나 찍는 함수
void setPixel(int x, int y, COLORREF color) {
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
        framebuffer[y][x] = color;
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

    // 테스트: 화면 가운데 흰 점 100개 찍기
    for (int i = 0; i < 100; i++)
        setPixel(400 + i, 300, RGB(255, 255, 255));

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