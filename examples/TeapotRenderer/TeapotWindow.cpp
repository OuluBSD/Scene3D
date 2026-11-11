#include "TeapotWindow.h"
#include <Draw/Draw.h>
#include <Geometry/Geometry.h>

using namespace Upp;
using namespace SoftGraphics;
using namespace TeapotRenderer;

static inline RGBA MakeRGBA(int r, int g, int b, int a = 255) {
    RGBA c;
    c.r = (byte)(r < 0 ? 0 : (r > 255 ? 255 : r));
    c.g = (byte)(g < 0 ? 0 : (g > 255 ? 255 : g));
    c.b = (byte)(b < 0 ? 0 : (b > 255 ? 255 : b));
    c.a = (byte)a;
    return c;
}

static inline Vec3 SafeNormalize(const Vec3& v) {
    float len = v.GetLength();
    if (len < 1e-8f) return Vec3(0, 0, 1);
    return v / len;
}

static inline float Saturate(float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }

// Minimal binary PPM (P6) writer so headless/CI runs can capture the frame
// without depending on image codec plugins.
static bool WritePPM(const String& path, const Image& img) {
    if (img.IsEmpty()) return false;
    Size sz = img.GetSize();
    FileOut out(path);
    if (!out.IsOpen()) return false;
    out.Put("P6\n");
    out << sz.cx << ' ' << sz.cy << "\n255\n";
    const RGBA* s = ~img;
    Vector<byte> row(sz.cx * 3);
    for (int y = 0; y < sz.cy; ++y) {
        for (int x = 0; x < sz.cx; ++x) {
            row[x * 3 + 0] = s->r;
            row[x * 3 + 1] = s->g;
            row[x * 3 + 2] = s->b;
            ++s;
        }
        out.Put(row.Begin(), sz.cx * 3);
    }
    return true;
}

TeapotWindow::TeapotWindow(const Config& cfg) : config(cfg), frameCount(0) {
    Title("TeapotRenderer - SoftGraphics").Sizeable().Zoomable();
    SetRect(0, 0, config.width, config.height);

    instance = sgCreateInstance();
    if (!instance) return;

    Array<Device*> devices = sgEnumerateDevices(instance, config.backend);
    if (devices.GetCount() > 0)
        device = sgCreateDevice(instance, devices[0], config.backend);
    else
        device = sgCreateDevice(instance, nullptr, config.backend);

    if (!device) return;

    mesh = GenerateTeapot(1.0f, 16);

    startTime = GetTickCount() / 1000.0;

    // Establish valid uniforms before the first Paint() call.
    UpdateUniforms();

    SetTimeCallback(16, [=] { RenderFrame(); });
}

TeapotWindow::~TeapotWindow() {
    CleanupGraphics();
}

void TeapotWindow::RenderFrame() {
    frameCount++;
    Refresh();
    if (config.frames > 0 && frameCount >= config.frames) {
        if (!config.outputFile.IsEmpty() && !renderImage.IsEmpty())
            WritePPM(config.outputFile, renderImage);
        Ctrl::CloseTopCtrls();
        return;
    }
    SetTimeCallback(16, [=] { RenderFrame(); });
}

void TeapotWindow::UpdateUniforms() {
    double t = GetTickCount() / 1000.0 - startTime;

    float radius = 7.0f;
    float camX = radius * sin(t * config.rotationSpeed);
    float camZ = radius * cos(t * config.rotationSpeed);
    float camY = 3.0f;

    eye = Vec3(camX, camY, camZ);
    center = Vec3(0, 0.9f, 0);   // teapot is Z-up, 1.8 tall -> center at ~0.9
    up = Vec3(0, 1, 0);

    uniforms.view = LookAt(eye, center, up);
    uniforms.proj = Perspective(45.0f * 3.14159265359f / 180.0f,
                                float(max(1, rtWidth ? rtWidth : config.width)) /
                                float(max(1, rtHeight ? rtHeight : config.height)),
                                0.1f, 100.0f);
    uniforms.viewProj = uniforms.proj * uniforms.view;
    // Teapot source is Z-up: rotate to Y-up first, then spin around world Y.
    uniforms.model = YRotation(t * config.rotationSpeed * 0.5f) * XRotation(-3.14159265359f / 2.0f);
    uniforms.normalMatrix = mat3({
        uniforms.model[0][0], uniforms.model[0][1], uniforms.model[0][2],
        uniforms.model[1][0], uniforms.model[1][1], uniforms.model[1][2],
        uniforms.model[2][0], uniforms.model[2][1], uniforms.model[2][2]
    }).GetInverse().GetTransposed();
    uniforms.lightDir = SafeNormalize(Vec3(1, -1, 0.5f));
    uniforms.lightColor = Vec3(1.0f, 0.95f, 0.9f);
    uniforms.ambientColor = Vec3(0.15f, 0.15f, 0.20f);
    uniforms.baseColor = Vec3(0.85f, 0.25f, 0.22f);
    uniforms.cameraPos = eye;
    uniforms.time = float(t);
}

void TeapotWindow::Paint(Draw& w) {
    UpdateUniforms();
    RenderScene();

    TopWindow::Paint(w);
    w.DrawRect(GetSize(), Color(16, 16, 32));

    if (!renderImage.IsEmpty())
        w.DrawImage(0, 0, renderImage);

    w.DrawText(10, 10, "TeapotRenderer - " + String(device ? device->GetName() : "no device"));
    w.DrawText(10, 30, "Frame: " + AsString(frameCount));
    w.DrawText(10, 50, "Triangles: " + AsString(mesh.indices.GetCount() / 3));
    w.DrawText(10, 70, "Path: CPU rasterizer -> Image -> DrawImage()");
}

void TeapotWindow::RenderScene() {
    Size sz = GetSize();
    int W = max(sz.cx, 1);
    int H = max(sz.cy, 1);

    if (W != rtWidth || H != rtHeight) {
        rtWidth = W;
        rtHeight = H;
        depthBuffer.SetCount(W * H);
    }
    if (mesh.vertices.GetCount() == 0) return;

    ImageBuffer colorBuffer(W, H);

    // Clear color and depth.
    RGBA bg = MakeRGBA(16, 16, 32);
    for (int y = 0; y < H; ++y) {
        RGBA* row = colorBuffer[y];
        float* drow = depthBuffer.Begin() + (size_t)y * W;
        for (int x = 0; x < W; ++x) {
            row[x] = bg;
            drow[x] = 1e30f;
        }
    }

    Mat4 mvp = uniforms.viewProj * uniforms.model;
    Mat4 world = uniforms.model;

    int n = mesh.vertices.GetCount();
    Vector<Vec3> screen(n);
    Vector<Vec3> wpos(n);
    Vector<Vec3> nrm(n);

    for (int i = 0; i < n; ++i) {
        const TeapotRenderer::Vertex& v = mesh.vertices[i];
        Vec4 p(v.position[0], v.position[1], v.position[2], 1.0f);

        Vec4 clip = mvp * p;
        if (fabs(clip[3]) > 1e-8f) {
            float invW = 1.0f / clip[3];
            screen[i] = Vec3((clip[0] * invW + 1.0f) * 0.5f * W,
                             (1.0f - clip[1] * invW) * 0.5f * H,
                             clip[2] * invW);
        } else {
            screen[i] = Vec3(0, 0, 1e30f);
        }

        Vec4 wp = world * p;
        wpos[i] = Vec3(wp[0], wp[1], wp[2]);

        Vec3 nw = uniforms.normalMatrix * Vec3(v.normal[0], v.normal[1], v.normal[2]);
        nrm[i] = SafeNormalize(nw);
    }

    // Per-vertex Gouraud colors (two-sided: light uses the side facing the camera).
    Vector<Vec3> col(n);
    for (int i = 0; i < n; ++i) {
        Vec3 N = nrm[i];

        // Flip the shading normal toward the viewer so back faces are lit, not black.
        if (Dot(N, eye - wpos[i]) < 0) N = N * -1.0f;

        Vec3 D = uniforms.lightDir;
        float diff = Saturate(Dot(N, D));

        Vec3 V = SafeNormalize(eye - wpos[i]);
        Vec3 Hv = SafeNormalize(D + V);
        float spec = powf(Saturate(Dot(N, Hv)), 32.0f);

        col[i] = uniforms.baseColor * uniforms.ambientColor +
                 uniforms.baseColor * uniforms.lightColor * diff +
                 uniforms.lightColor * Vec3(0.25f, 0.25f, 0.25f) * spec;
    }

    int triCount = mesh.indices.GetCount() / 3;
    for (int t = 0; t < triCount; ++t) {
        int a = mesh.indices[t * 3];
        int b = mesh.indices[t * 3 + 1];
        int c = mesh.indices[t * 3 + 2];

        Vec3 s0 = screen[a], s1 = screen[b], s2 = screen[c];
        if (s0[2] > 1e29f || s1[2] > 1e29f || s2[2] > 1e29f) continue;

        // Signed area in screen space.
        float area = (s1[0] - s0[0]) * (s2[1] - s0[1]) -
                     (s2[0] - s0[0]) * (s1[1] - s0[1]);
        if (fabs(area) < 0.5f) continue;

        // Two-sided rasterization: normalize winding so barycentrics are positive.
        Vec3 p0 = s0, p1 = s1, p2 = s2;
        Vec3 k0 = col[a], k1 = col[b], k2 = col[c];
        if (area < 0) {
            Swap(p1, p2);
            Swap(k1, k2);
            area = -area;
        }
        float invArea = 1.0f / area;

        int minx = max(0, (int)floorf(min(p0[0], min(p1[0], p2[0]))));
        int maxx = min(W - 1, (int)ceilf(max(p0[0], max(p1[0], p2[0]))));
        int miny = max(0, (int)floorf(min(p0[1], min(p1[1], p2[1]))));
        int maxy = min(H - 1, (int)ceilf(max(p0[1], max(p1[1], p2[1]))));
        if (minx > maxx || miny > maxy) continue;

        for (int y = miny; y <= maxy; ++y) {
            RGBA* row = colorBuffer[y];
            float* drow = depthBuffer.Begin() + (size_t)y * W;
            float py = y + 0.5f;
            for (int x = minx; x <= maxx; ++x) {
                float px = x + 0.5f;

                float w0 = ((p1[0] - px) * (p2[1] - py) - (p2[0] - px) * (p1[1] - py)) * invArea;
                float w1 = ((p2[0] - px) * (p0[1] - py) - (p0[0] - px) * (p2[1] - py)) * invArea;
                float w2 = 1.0f - w0 - w1;
                if (w0 < 0 || w1 < 0 || w2 < 0) continue;

                float z = w0 * p0[2] + w1 * p1[2] + w2 * p2[2];
                if (z >= drow[x]) continue;   // depth test (nearer wins)
                drow[x] = z;

                Vec3 color = k0 * w0 + k1 * w1 + k2 * w2;
                row[x] = MakeRGBA((int)(color[0] * 255.0f),
                                  (int)(color[1] * 255.0f),
                                  (int)(color[2] * 255.0f));
            }
        }
    }

    renderImage = colorBuffer;   // Image steals the pixels; colorBuffer is per-frame
}

void TeapotWindow::CleanupGraphics() {
    if (device) sgDestroyDevice(device);
    if (instance) sgDestroyInstance(instance);
    device = nullptr;
    instance = nullptr;
}
