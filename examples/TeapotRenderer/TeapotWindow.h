#pragma once

#include <SoftGraphics/SoftGraphics.h>
#include <SoftGraphics/Device.h>
#include <SoftGraphics/CommandBuffer.h>
#include <SoftGraphics/Buffer.h>
#include <SoftGraphics/Surface.h>
#include <CtrlLib/CtrlLib.h>
#include <Core/Core.h>
#include "teapot.h"

using namespace Upp;
using namespace SoftGraphics;
using namespace TeapotRenderer;

// Uniform buffer data (matches shader uniforms)
struct UniformData {
    Mat4 view;
    Mat4 proj;
    Mat4 viewProj;
    Mat4 model;
    mat3 normalMatrix;
    Vec3 lightDir;
    Vec3 lightColor;
    Vec3 ambientColor;
    Vec3 baseColor;
    Vec3 cameraPos;
    float metallic = 0.0f;
    float roughness = 0.5f;
    float ao = 1.0f;
    float time = 0.0f;
};

// Configuration
struct Config {
    Backend backend = Backend::CPU;
    bool headless = false;
    int width = 1024;
    int height = 768;
    int frames = 0; // 0 = infinite
    String outputFile;
    bool wireframe = false;
    bool pbr = false;
    bool showDepth = false;
    float rotationSpeed = 0.5f;
};

// GUI Application - creates window and runs render loop.
//
// Rendering approach: the SoftGraphics CPU backend rasterizes the scene into an
// RGBA8 render target (an in-memory Image). The GUI only presents that image:
// Paint() blits it with DrawImage(). No OpenGL/X11 GL context is used.
class TeapotWindow : public TopWindow {
public:
    TeapotWindow(const Config& cfg);
    ~TeapotWindow();

    void RenderFrame();
    void Paint(Draw& w) override;

private:
    void UpdateUniforms();
    void RenderScene();          // rasterize into an Image render target
    void CleanupGraphics();

    Config config;
    Instance* instance = nullptr;
    Device* device = nullptr;
    Surface* surface = nullptr;
    MeshData mesh;
    UniformData uniforms;

    Vector<float> depthBuffer;
    Image renderImage;
    int rtWidth = 0;
    int rtHeight = 0;

    int frameCount = 0;
    double startTime = 0;

    Vec3 eye;
    Vec3 center;
    Vec3 up;
};
