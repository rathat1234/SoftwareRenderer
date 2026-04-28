#include <windows.h>
#include <algorithm>
#include <cmath>
#include <vector>
using namespace std;

const int WIDTH = 800;
const int HEIGHT = 600;
COLORREF framebuffer[HEIGHT][WIDTH];

// =====================
// 수학 구조체
// =====================
struct Vec3 {
    float x, y, z;
};

struct Vec4 {
    float x, y, z, w;
};

struct Mat4 {
    float m[4][4] = {};

    Mat4 operator*(const Mat4& o) const {
        Mat4 r;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                for (int k = 0; k < 4; k++)
                    r.m[i][j] += m[i][k] * o.m[k][j];
        return r;
    }

    Vec4 operator*(const Vec4& v) const {
        return {
            m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * v.w,
            m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * v.w,
            m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * v.w,
            m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] * v.w
        };
    }
};

// 단위 행렬
Mat4 makeIdentity () {
    Mat4 m;
    m.m[0][0] = m.m[1][1] = m.m[2][2] = m.m[3][3] = 1.0f;
    return m;
}

// Y축 회전 행렬
Mat4 rotateY(float angle) {
    Mat4 m = makeIdentity ();
    m.m[0][0] = cosf(angle);
    m.m[0][2] = sinf(angle);
    m.m[2][0] = -sinf(angle);
    m.m[2][2] = cosf(angle);
    return m;
}

// X축 회전 행렬
Mat4 rotateX(float angle) {
    Mat4 m = makeIdentity ();
    m.m[1][1] = cosf(angle);
    m.m[1][2] = -sinf(angle);
    m.m[2][1] = sinf(angle);
    m.m[2][2] = cosf(angle);
    return m;
}

// 이동 행렬
Mat4 translate(float tx, float ty, float tz) {
    Mat4 m = makeIdentity ();
    m.m[0][3] = tx;
    m.m[1][3] = ty;
    m.m[2][3] = tz;
    return m;
}

// 원근 투영 행렬
Mat4 perspective(float fov, float aspect, float zNear, float zFar) {
    Mat4 m;
    float f = 1.0f / tanf(fov / 2.0f);
    m.m[0][0] = f / aspect;
    m.m[1][1] = f;
    m.m[2][2] = (zFar + zNear) / (zNear - zFar);
    m.m[2][3] = (2 * zFar * zNear) / (zNear - zFar);
    m.m[3][2] = -1.0f;
    return m;
}

// =====================
// 렌더링 함수
// =====================
void setPixel(int x, int y, COLORREF color) {
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
        framebuffer[y][x] = color;
}

void drawLine(int x0, int y0, int x1, int y1, COLORREF color) {
    int dx = abs(x1 - x0), dy = abs(y1 - y0);
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

// 3D 정점 → 화면 좌표 변환
bool projectVertex(const Vec3& v, const Mat4& mvp, int& sx, int& sy) {
    Vec4 clip = mvp * Vec4{ v.x, v.y, v.z, 1.0f };
    if (clip.w <= 0) return false;

    // NDC 좌표
    float ndcX = clip.x / clip.w;
    float ndcY = clip.y / clip.w;

    // 화면 좌표
    sx = (int)((ndcX + 1.0f) * 0.5f * WIDTH);
    sy = (int)((1.0f - ndcY) * 0.5f * HEIGHT);
    return true;
}

// =====================
// 정육면체 데이터
// =====================
Vec3 cubeVertices[8] = {
    {-1,-1,-1}, { 1,-1,-1}, { 1, 1,-1}, {-1, 1,-1},  // 앞면
    {-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1}, {-1, 1, 1}   // 뒷면
};

// 엣지 (선으로 그릴 연결)
int cubeEdges[12][2] = {
    {0,1},{1,2},{2,3},{3,0},  // 앞면
    {4,5},{5,6},{6,7},{7,4},  // 뒷면
    {0,4},{1,5},{2,6},{3,7}   // 연결
};

float angle = 0.0f;

void render() {
    memset(framebuffer, 0, sizeof(framebuffer));

    // MVP 행렬 구성
    Mat4 model = rotateY(angle) * rotateX(angle * 0.5f);
    Mat4 view = translate(0, 0, -5);
    Mat4 proj = perspective(3.14159f / 3.0f, (float)WIDTH / HEIGHT, 0.1f, 100.0f);
    Mat4 mvp = proj * view * model;

    // 정육면체 엣지 그리기
    for (auto& e : cubeEdges) {
        int x0, y0, x1, y1;
        if (projectVertex(cubeVertices[e[0]], mvp, x0, y0) &&
            projectVertex(cubeVertices[e[1]], mvp, x1, y1)) {
            drawLine(x0, y0, x1, y1, RGB(0, 255, 128));
        }
    }
}

// =====================
// 윈도우
// =====================
HWND g_hwnd;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        for (int y = 0; y < HEIGHT; y++)
            for (int x = 0; x < WIDTH; x++)
                SetPixel(hdc, x, y, framebuffer[y][x]);
        EndPaint(hwnd, &ps);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"SoftwareRenderer";
    RegisterClass(&wc);

    g_hwnd = CreateWindow(L"SoftwareRenderer", L"Software Renderer - Day4",
        WS_OVERLAPPEDWINDOW, 100, 100, WIDTH, HEIGHT,
        NULL, NULL, hInstance, NULL);
    ShowWindow(g_hwnd, nCmdShow);

    MSG msg = {};
    while (true) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) return 0;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        // 애니메이션 루프
        angle += 0.02f;
        render();
        InvalidateRect(g_hwnd, NULL, FALSE);
        Sleep(16); // ~60fps
    }
    return 0;
}