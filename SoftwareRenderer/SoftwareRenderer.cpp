#include <windows.h>
#include "Math/Vec3.h"
#include "Math/Mat4.h"
#include "Core/Framebuffer.h"
#include "Core/Renderer.h"
#include "Scene/Camera.h"
#include "Scene/Light.h"
#include "Scene/Mesh.h"
#include "Scene/ECS.h"  // ECS
#include <immintrin.h>  // AVX2
#include <emmintrin.h>  // SSE2
#include <thread>
#include <vector>

Framebuffer fb;
Camera      camera;
Light       light;
Mesh        mesh;
HWND        g_hwnd;
ECSWorld world;
EntityID mainEntity;
EntityID lightEntity;

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
Texture normalMap;

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

Vec3 sampleNormalMap(const Texture& tex, float u, float v) {
    if (tex.width == 0) return { 0.0f, 0.0f, 1.0f };  // 기본 법선
    u = u - floorf(u);
    v = v - floorf(v);
    int x = (int)(u * (tex.width - 1));
    int y = (int)(v * (tex.height - 1));
    COLORREF c = tex.pixels[y * tex.width + x];
    // RGB [0~255] → 법선 [-1~1]
    float nx = (GetRValue(c) / 255.0f) * 2.0f - 1.0f;
    float ny = (GetGValue(c) / 255.0f) * 2.0f - 1.0f;
    float nz = (GetBValue(c) / 255.0f) * 2.0f - 1.0f;
    return normalize({ nx, ny, nz });
}

// GGX Normal Distribution Function
float DistributionGGX(float NdotH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float denom = (NdotH * NdotH) * (a2 - 1.0f) + 1.0f;
    return a2 / (3.14159f * denom * denom);
}

// Schlick Fresnel 근사
Vec3 FresnelSchlick(float cosTheta, Vec3 F0) {
    float f = powf(1.0f - cosTheta, 5.0f);
    return {
        F0.x + (1.0f - F0.x) * f,
        F0.y + (1.0f - F0.y) * f,
        F0.z + (1.0f - F0.z) * f
    };
}

// Smith Geometry Function
float GeometrySmith(float NdotV, float NdotL, float roughness) {
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    float ggx1 = NdotV / (NdotV * (1.0f - k) + k);
    float ggx2 = NdotL / (NdotL * (1.0f - k) + k);
    return ggx1 * ggx2;
}

// PBR 색상 계산
COLORREF calcPBR(Vec3 albedo, Vec3 N, Vec3 L, Vec3 V, float metallic, float roughness) {
    Vec3 H = normalize({ L.x + V.x, L.y + V.y, L.z + V.z });

    float NdotL = max(0.2f, dot(N, L));
    float NdotV = max(0.0f, dot(N, V));
    float NdotH = max(0.0f, dot(N, H));
    float HdotV = max(0.0f, dot(H, V));

    // F0: 비금속은 0.04, 금속은 albedo
    Vec3 F0 = { 0.04f, 0.04f, 0.04f };
    F0 = {
        F0.x + (albedo.x - F0.x) * metallic,
        F0.y + (albedo.y - F0.y) * metallic,
        F0.z + (albedo.z - F0.z) * metallic
    };

    float D = DistributionGGX(NdotH, roughness);
    Vec3  F = FresnelSchlick(HdotV, F0);
    float G = GeometrySmith(NdotV, NdotL, roughness);

    // Specular
    float denom = 4.0f * NdotV * NdotL + 0.001f;
    Vec3 specular = {
        D * F.x * G / denom,
        D * F.y * G / denom,
        D * F.z * G / denom
    };

    // Diffuse (kd = 1 - F, 금속은 diffuse 없음)
    Vec3 kd = {
        (1.0f - F.x) * (1.0f - metallic),
        (1.0f - F.y) * (1.0f - metallic),
        (1.0f - F.z) * (1.0f - metallic)
    };

    Vec3 diffuse = {
        kd.x * albedo.x / 3.14159f,
        kd.y * albedo.y / 3.14159f,
        kd.z * albedo.z / 3.14159f
    };

    // ambient
    Vec3 ambient = { albedo.x * 0.3f, albedo.y * 0.3f, albedo.z * 0.3f };

    float lightIntensity = 5.0f;
    Vec3 Lo = {
        (diffuse.x + specular.x) * NdotL * lightIntensity + ambient.x,
        (diffuse.y + specular.y) * NdotL * lightIntensity + ambient.y,
        (diffuse.z + specular.z) * NdotL * lightIntensity + ambient.z
    };

    // Tone mapping (HDR → LDR)
    Lo.x = Lo.x / (Lo.x + 1.0f);
    Lo.y = Lo.y / (Lo.y + 1.0f);
    Lo.z = Lo.z / (Lo.z + 1.0f);

    int r = (int)(min(Lo.x, 1.0f) * 255);
    int g = (int)(min(Lo.y, 1.0f) * 255);
    int b = (int)(min(Lo.z, 1.0f) * 255);
    return RGB(r, g, b);
}

void renderChunk(int startY, int endY, const Mat4& mvp, const Mat4& lightMVP) {
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

        // Normal Map에서 법선 샘플링
        Vec3 n0 = sampleNormalMap(normalMap, u0, vo0);
        Vec3 n1 = sampleNormalMap(normalMap, u1, vo1);
        Vec3 n2 = sampleNormalMap(normalMap, u2, vo2);

        // 조명 방향
        Vec3 lightDir = normalize({ -light.direction.x, -light.direction.y, -light.direction.z });
        Vec3 viewDirV = { 0.0f, 0.0f, 1.0f };

        // PBR 파라미터 (텍스처 없으니 상수)
        float metallic = 0.0f;
        float roughness = 0.5f;

        // 버텍스별 PBR 색상 계산
        auto toPBRAlbedo = [](COLORREF c) -> Vec3 {
            return {
                GetRValue(c) / 255.0f,
                GetGValue(c) / 255.0f,
                GetBValue(c) / 255.0f
            };
            };

        c0 = calcPBR(toPBRAlbedo(c0), n0, lightDir, viewDirV, metallic, roughness);
        c1 = calcPBR(toPBRAlbedo(c1), n1, lightDir, viewDirV, metallic, roughness);
        c2 = calcPBR(toPBRAlbedo(c2), n2, lightDir, viewDirV, metallic, roughness);
        // 광원 공간 버텍스 계산
        ScreenVert lv[3];
        lv[0] = projectVertex(v[0], lightMVP);
        lv[1] = projectVertex(v[1], lightMVP);
        lv[2] = projectVertex(v[2], lightMVP);

        drawTriangleGouraud(fb,
            sv[0].sx, sv[0].sy, sv[0].depth, c0,
            sv[1].sx, sv[1].sy, sv[1].depth, c1,
            sv[2].sx, sv[2].sy, sv[2].depth, c2,
            lv[0].sx, lv[0].sy, lv[0].depth,
            lv[1].sx, lv[1].sy, lv[1].depth,
            lv[2].sx, lv[2].sy, lv[2].depth,
            startY, endY);
    }
}

void render() {
        fb.clear();

        TransformComponent* transform = world.getTransform(mainEntity);
        Mat4 model = rotateY(camera.rotY) * rotateX(camera.rotX);
        if (transform) {
            // Transform 컴포넌트에서 추가 회전/위치 반영 가능
            model = rotateY(camera.rotY + transform->rotation.y)
                * rotateX(camera.rotX + transform->rotation.x);
        }

        Mat4 view = camera.getViewMatrix();
        Mat4 proj = camera.getProjectionMatrix(3.14159f / 3.0f, (float)WIDTH / HEIGHT);
        Mat4 mvp = proj * view * model;

        // ECS에서 조명 방향 읽기
        Vec3 lightDirECS = light.direction;
        for (auto& [id, lc] : world.lights) {
            lightDirECS = lc.direction;
            break;
        }

        // 광원 위치
        Vec3 lightPos = { -lightDirECS.x * 5.0f,
                          -lightDirECS.y * 5.0f,
                          -lightDirECS.z * 5.0f };

        // 광원 시점 View 행렬
        Mat4 lightView = lookAt(lightPos, { 0,0,0 }, { 0,1,0 });
        Mat4 lightProj = camera.getProjectionMatrix(3.14159f / 3.0f, (float)WIDTH / HEIGHT);
        Mat4 lightMVP = lightProj * lightView * model;

        // 1패스: Shadow Map 생성
        fb.clearShadowMap();
        for (int i = 0; i + 2 < (int)mesh.indices.size(); i += 3) {
            Vec3 v[3];
            v[0] = mesh.vertices[mesh.indices[i]];
            v[1] = mesh.vertices[mesh.indices[i + 1]];
            v[2] = mesh.vertices[mesh.indices[i + 2]];

            ScreenVert sv[3];
            sv[0] = projectVertex(v[0], lightMVP);
            sv[1] = projectVertex(v[1], lightMVP);
            sv[2] = projectVertex(v[2], lightMVP);

            drawTriangleShadow(fb,
                sv[0].sx, sv[0].sy, sv[0].depth,
                sv[1].sx, sv[1].sy, sv[1].depth,
                sv[2].sx, sv[2].sy, sv[2].depth);
        }

        // 2패스: 기존 렌더링 (멀티스레드)
        const int NUM_THREADS = 8;
        int chunkH = HEIGHT / NUM_THREADS;
        std::vector<std::thread> threads;
        for (int t = 0; t < NUM_THREADS; t++) {
            int startY = t * chunkH;
            int endY = (t == NUM_THREADS - 1) ? HEIGHT : startY + chunkH;
            threads.emplace_back(renderChunk, startY, endY, std::cref(mvp), std::cref(lightMVP));
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

    g_hwnd = CreateWindow(L"SoftwareRenderer", L"Software Renderer - Day20",
        WS_OVERLAPPEDWINDOW, 100, 100, WIDTH, HEIGHT,
        NULL, NULL, hInstance, NULL);

    fb.init(g_hwnd);

    mesh.loadOBJ("airboat.obj");
    // ECS 엔티티 생성
    mainEntity = world.createEntity();
    world.addTransform(mainEntity, { {0,0,0}, {0,0,0}, {1,1,1} });
    world.addMesh(mainEntity, { 0 });
    lightEntity = world.createEntity();
    world.addLight(lightEntity, { {0,-1,-1}, 1.0f });
    normalMap = loadBMP("normal_map.bmp");
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
            swprintf_s(title, L"Software Renderer - Day20 | FPS: %.1f", fps);
            SetWindowText(g_hwnd, title);
        }
    }
    return 0;
}