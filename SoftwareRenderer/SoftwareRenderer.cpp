#include <windows.h>
#include "Math/Vec3.h"
#include "Math/Mat4.h"
#include "Core/Framebuffer.h"
#include "Core/Renderer.h"
#include "Scene/Camera.h"
#include "Scene/Light.h"
#include "Scene/Mesh.h"
#include <immintrin.h>  // AVX2
#include <emmintrin.h>  // SSE2
#include <thread>
#include <vector>

Framebuffer fb;
Camera      camera;
Light       light;
Mesh        mesh;
HWND        g_hwnd;

// 마우스 상태
int  lastMouseX = 0;
int  lastMouseY = 0;
bool mouseDown = false;

struct Texture {
    int width, height;
    vector<COLORREF> pixels;
};

Texture texture;

// FPS 카운터
int   frameCount = 0;
DWORD lastTime = 0;
float fps = 0.0f;

Texture loadBMP(const char* path) {
    Texture tex;
    tex.width = 0; tex.height = 0;
    FILE* f = nullptr;
    fopen_s(&f, path, "rb");
    if (!f) return tex;

    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;
    fread(&bfh, sizeof(bfh), 1, f);
    fread(&bih, sizeof(bih), 1, f);

    tex.width = bih.biWidth;
    tex.height = abs(bih.biHeight);
    tex.pixels.resize(tex.width * tex.height);

    fseek(f, bfh.bfOffBits, SEEK_SET);
    for (int y = tex.height - 1; y >= 0; y--) {
        for (int x = 0; x < tex.width; x++) {
            BYTE b, g, r;
            fread(&b, 1, 1, f);
            fread(&g, 1, 1, f);
            fread(&r, 1, 1, f);
            tex.pixels[y * tex.width + x] = RGB(r, g, b);
        }
        int padding = (4 - (tex.width * 3) % 4) % 4;
        fseek(f, padding, SEEK_CUR);
    }
    fclose(f);
    return tex;
}

COLORREF sampleTexture(const Texture& tex, float u, float v) {
    if (tex.width == 0) return RGB(180, 180, 180);
    u = u - floorf(u);
    v = v - floorf(v);
    int x = (int)(u * (tex.width - 1));
    int y = (int)(v * (tex.height - 1));
    return tex.pixels[y * tex.width + x];
}

void renderChunk(int startY, int endY, const Mat4& mvp) {
    Vec3 viewDir = { 0.0f, 0.0f, -1.0f };

    for (int i = 0; i + 2 < (int)mesh.indices.size(); i += 3) {
        Vec3 v[3];
        v[0] = mesh.vertices[mesh.indices[i]];
        v[1] = mesh.vertices[mesh.indices[i + 1]];
        v[2] = mesh.vertices[mesh.indices[i + 2]];

        Vec3 edge1 = subtract(v[1], v[0]);
        Vec3 edge2 = subtract(v[2], v[0]);
        Vec3 normal = normalize(cross(edge1, edge2));
        if (dot(normal, viewDir) >= 0) continue;

        ScreenVert sv[3];
        sv[0] = projectVertex(v[0], mvp);
        sv[1] = projectVertex(v[1], mvp);
        sv[2] = projectVertex(v[2], mvp);

        if (sv[0].sx < 0 && sv[1].sx < 0 && sv[2].sx < 0) continue;
        if (sv[0].sx > WIDTH && sv[1].sx > WIDTH && sv[2].sx > WIDTH) continue;
        if (sv[0].sy < 0 && sv[1].sy < 0 && sv[2].sy < 0) continue;
        if (sv[0].sy > HEIGHT && sv[1].sy > HEIGHT && sv[2].sy > HEIGHT) continue;

        float u0 = (v[0].x + 1.0f) * 0.5f;
        float vo0 = (v[0].y + 1.0f) * 0.5f;
        float u1 = (v[1].x + 1.0f) * 0.5f;
        float vo1 = (v[1].y + 1.0f) * 0.5f;
        float u2 = (v[2].x + 1.0f) * 0.5f;
        float vo2 = (v[2].y + 1.0f) * 0.5f;

        COLORREF c0 = sampleTexture(texture, u0, vo0);
        COLORREF c1 = sampleTexture(texture, u1, vo1);
        COLORREF c2 = sampleTexture(texture, u2, vo2);

        drawTriangleGouraud(fb,
            sv[0].sx, sv[0].sy, sv[0].depth, c0,
            sv[1].sx, sv[1].sy, sv[1].depth, c1,
            sv[2].sx, sv[2].sy, sv[2].depth, c2,
            startY, endY);
    }
}

void render() {
        fb.clear();

        Mat4 model = rotateY(camera.rotY) * rotateX(camera.rotX);
        Mat4 view = camera.getViewMatrix();
        Mat4 proj = camera.getProjectionMatrix(3.14159f / 3.0f, (float)WIDTH / HEIGHT);
        Mat4 mvp = proj * view * model;

        const int NUM_THREADS = 1;
        int chunkH = HEIGHT / NUM_THREADS;

        std::vector<std::thread> threads;
        for (int t = 0; t < NUM_THREADS; t++) {
            int startY = t * chunkH;
            int endY = (t == NUM_THREADS - 1) ? HEIGHT : startY + chunkH;
            threads.emplace_back(renderChunk, startY, endY, std::cref(mvp));
        }
        for (auto& th : threads) th.join();

}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        fb.present(hdc);
        EndPaint(hwnd, &ps);
    }
    if (msg == WM_LBUTTONDOWN) {
        mouseDown = true;
        lastMouseX = LOWORD(lParam);
        lastMouseY = HIWORD(lParam);
        return 0;
    }
    if (msg == WM_LBUTTONUP) {
        mouseDown = false;
        return 0;
    }
    if (msg == WM_MOUSEMOVE) {
        if (mouseDown) {
            int mx = LOWORD(lParam);
            int my = HIWORD(lParam);
            camera.rotY += (mx - lastMouseX) * 0.01f;
            camera.rotX += (my - lastMouseY) * 0.01f;
            lastMouseX = mx;
            lastMouseY = my;
        }
        return 0;
    }
    if (msg == WM_MOUSEWHEEL) {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        camera.posZ += delta * 0.005f;
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"SoftwareRenderer";
    RegisterClass(&wc);

    g_hwnd = CreateWindow(L"SoftwareRenderer", L"Software Renderer - Day13",
        WS_OVERLAPPEDWINDOW, 100, 100, WIDTH, HEIGHT,
        NULL, NULL, hInstance, NULL);

    fb.init(g_hwnd);

    mesh.loadOBJ("airboat.obj");

    texture = loadBMP("texture.bmp");
    ShowWindow(g_hwnd, nCmdShow);
    MSG msg = {};
    lastTime = GetTickCount();
    while (true) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) return 0;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        render();

        // WM_PAINT 대신 직접 present
        HDC hdc = GetDC(g_hwnd);
        fb.present(hdc);
        ReleaseDC(g_hwnd, hdc);

        // FPS 카운터
        frameCount++;
        DWORD now = GetTickCount();
        if (now - lastTime >= 1000) {
            fps = frameCount * 1000.0f / (now - lastTime);
            frameCount = 0;
            lastTime = now;
            wchar_t title[64];
            swprintf_s(title, L"Software Renderer - Day15 | FPS: %.1f", fps);
            SetWindowText(g_hwnd, title);
        }
    }
    return 0;
}