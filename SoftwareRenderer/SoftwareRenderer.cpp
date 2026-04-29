#include <windows.h>
#include <algorithm>
#include <cmath>
using namespace std;

const int WIDTH = 800;
const int HEIGHT = 600;
COLORREF framebuffer[HEIGHT][WIDTH];
float zbuffer[HEIGHT][WIDTH];

// =====================
// 수학 구조체
// =====================
struct Vec3 { float x, y, z; };
struct Vec4 { float x, y, z, w; };

struct Mat4 {
    float m[4][4];
    Mat4() { for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) m[i][j] = 0; }

    Mat4 operator*(const Mat4& o) const {
        Mat4 r;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                for (int k = 0; k < 4; k++)
                    r.m[i][j] += m[i][k] * o.m[k][j];
        return r;
    }
    Vec4 operator*(const Vec4& v) const {
        Vec4 r;
        r.x = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * v.w;
        r.y = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * v.w;
        r.z = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * v.w;
        r.w = m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] * v.w;
        return r;
    }
};

Mat4 makeIdentity() {
    Mat4 m;
    m.m[0][0] = m.m[1][1] = m.m[2][2] = m.m[3][3] = 1.0f;
    return m;
}
Mat4 rotateY(float a) {
    Mat4 m = makeIdentity();
    m.m[0][0] = cosf(a); m.m[0][2] = sinf(a);
    m.m[2][0] = -sinf(a); m.m[2][2] = cosf(a);
    return m;
}
Mat4 rotateX(float a) {
    Mat4 m = makeIdentity();
    m.m[1][1] = cosf(a); m.m[1][2] = -sinf(a);
    m.m[2][1] = sinf(a); m.m[2][2] = cosf(a);
    return m;
}
Mat4 makeTranslate(float tx, float ty, float tz) {
    Mat4 m = makeIdentity();
    m.m[0][3] = tx; m.m[1][3] = ty; m.m[2][3] = tz;
    return m;
}
Mat4 makePerspective(float fov, float aspect, float zNear, float zFar) {
    Mat4 m;
    float f = 1.0f / tanf(fov / 2.0f);
    m.m[0][0] = f / aspect;
    m.m[1][1] = f;
    m.m[2][2] = (zFar + zNear) / (zNear - zFar);
    m.m[2][3] = (2.0f * zFar * zNear) / (zNear - zFar);
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

struct ScreenVert { float sx, sy, depth; };

ScreenVert projectVertex(const Vec3& v, const Mat4& mvp) {
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

void drawTriangle3D(
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
            if (depth < zbuffer[y][x]) {
                zbuffer[y][x] = depth;
                setPixel(x, y, color);
            }
        }
    }
}

// =====================
// 정육면체 데이터
// =====================
Vec3 cubeVerts[8] = {
    {-1,-1,-1}, { 1,-1,-1}, { 1, 1,-1}, {-1, 1,-1},
    {-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1}, {-1, 1, 1}
};

struct Face { int v[4]; COLORREF color; };
Face cubeFaces[6] = {
    {{0,1,2,3}, RGB(255, 80,  80)},
    {{5,4,7,6}, RGB(80,  255, 80)},
    {{4,0,3,7}, RGB(80,  80,  255)},
    {{1,5,6,2}, RGB(255, 255, 80)},
    {{3,2,6,7}, RGB(80,  255, 255)},
    {{4,5,1,0}, RGB(255, 80,  255)},
};

float angle = 0.0f;

// 벡터 연산 헬퍼
Vec3 subtract(Vec3 a, Vec3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
Vec3 cross(Vec3 a, Vec3 b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}
float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Vec3 normalize(Vec3 v) {
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len < 1e-6f) return { 0,0,0 };
    return { v.x / len, v.y / len, v.z / len };
}

COLORREF applyLight(COLORREF baseColor, float intensity) {
    intensity = max(0.1f, min(1.0f, intensity)); // ambient 0.1 보장
    int r = (int)(GetRValue(baseColor) * intensity);
    int g = (int)(GetGValue(baseColor) * intensity);
    int b = (int)(GetBValue(baseColor) * intensity);
    return RGB(r, g, b);
}

void render() {
    memset(framebuffer, 0, sizeof(framebuffer));
    for (int y = 0; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++)
            zbuffer[y][x] = 1e9f;

    Mat4 model = rotateY(angle) * rotateX(angle * 0.5f);
    Mat4 view = makeTranslate(0, 0, -5);
    Mat4 proj = makePerspective(3.14159f / 3.0f, (float)WIDTH / HEIGHT, 0.1f, 100.0f);
    Mat4 mvp = proj * view * model;

    // 빛 방향 (월드 공간, 정규화)
    Vec3 lightDir = normalize({ 1.0f, 1.0f, -1.0f });

    for (int f = 0; f < 6; f++) {
        // 월드 공간에서 법선 계산 (조명용)
        Vec3 worldVerts[4];
        for (int i = 0; i < 4; i++) {
            Vec3 v = cubeVerts[cubeFaces[f].v[i]];
            Vec4 in; in.x = v.x; in.y = v.y; in.z = v.z; in.w = 1.0f;
            Vec4 w = view * model * in;
            worldVerts[i] = { w.x, w.y, w.z };
        }

        // 면 법선 = 두 엣지의 외적
        Vec3 edge1 = subtract(worldVerts[1], worldVerts[0]);
        Vec3 edge2 = subtract(worldVerts[2], worldVerts[0]);
        Vec3 normal = normalize(cross(edge1, edge2));

        // Lambert 조명
        float intensity = dot(normal, lightDir);
        COLORREF litColor = applyLight(cubeFaces[f].color, intensity);

        // 스크린 변환
        ScreenVert sv[4];
        for (int i = 0; i < 4; i++)
            sv[i] = projectVertex(cubeVerts[cubeFaces[f].v[i]], mvp);

        drawTriangle3D(
            sv[0].sx, sv[0].sy, sv[0].depth,
            sv[1].sx, sv[1].sy, sv[1].depth,
            sv[2].sx, sv[2].sy, sv[2].depth,
            litColor);
        drawTriangle3D(
            sv[0].sx, sv[0].sy, sv[0].depth,
            sv[2].sx, sv[2].sy, sv[2].depth,
            sv[3].sx, sv[3].sy, sv[3].depth,
            litColor);
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

    g_hwnd = CreateWindow(L"SoftwareRenderer", L"Software Renderer - Day6",
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
        angle += 0.02f;
        render();
        InvalidateRect(g_hwnd, NULL, FALSE);
        Sleep(16);
    }
    return 0;
}