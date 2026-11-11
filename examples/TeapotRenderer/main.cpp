// TeapotRenderer - SoftGraphics virtual graphics device demo
//
// Build with backend selection via mainconfig USE flags:
//   USE_CPU (default), USE_VULKAN, USE_METAL, USE_DX12, USE_OPENGL,
//   USE_CINEMATIC, USE_OPENGL_LEGACY, USE_XBOX360, USE_PS1

#include <SoftGraphics/SoftGraphics.h>
#include <SoftGraphics/Device.h>
#include <SoftGraphics/CommandBuffer.h>
#include <SoftGraphics/Buffer.h>
#include <SoftGraphics/Surface.h>
#include <Core/Core.h>
#include "teapot.h"
#include "TeapotWindow.h"

using namespace Upp;
using namespace SoftGraphics;
using namespace TeapotRenderer;

void RunHeadless(const Config& cfg);

Config ParseArgs(const Vector<String>& args) {
    Config cfg;
    for (int i = 0; i < args.GetCount(); ++i) {
        String arg = args[i];
        if (arg == "--help" || arg == "-h") {
            Cout() << "TeapotRenderer - SoftGraphics demo\n";
            Cout() << "Usage: TeapotRenderer [options]\n";
            Cout() << "Options:\n";
            Cout() << "  --backend <cpu|vulkan|metal|dx12|opengl|cinematic|opengl-legacy|xbox360|ps1>\n";
            Cout() << "  --headless          Run without window (CI mode)\n";
            Cout() << "  --width <int>       Window width (default 1024)\n";
            Cout() << "  --height <int>      Window height (default 768)\n";
            Cout() << "  --frames <int>      Number of frames to render (0 = infinite)\n";
            Cout() << "  --output <file>     Output image file (headless mode)\n";
            Cout() << "  --wireframe         Enable wireframe overlay\n";
            Cout() << "  --pbr               Use PBR shader instead of basic\n";
            Cout() << "  --rotation <float>  Rotation speed (default 0.5)\n";
            Cout().Flush();
            exit(0);
        }
        else if (arg == "--backend" && i + 1 < args.GetCount()) {
            String be = args[++i];
            if (be == "cpu") cfg.backend = Backend::CPU;
            else if (be == "vulkan") cfg.backend = Backend::Vulkan;
            else if (be == "metal") cfg.backend = Backend::Metal;
            else if (be == "dx12") cfg.backend = Backend::DX12;
            else if (be == "opengl") cfg.backend = Backend::OpenGL;
            else if (be == "cinematic") cfg.backend = Backend::Cinematic;
            else Cout() << "Unknown backend: " << be << "\n";
        }
        else if (arg == "--headless") cfg.headless = true;
        else if (arg == "--width" && i + 1 < args.GetCount()) cfg.width = ScanInt(args[++i]);
        else if (arg == "--height" && i + 1 < args.GetCount()) cfg.height = ScanInt(args[++i]);
        else if (arg == "--frames" && i + 1 < args.GetCount()) cfg.frames = ScanInt(args[++i]);
        else if (arg == "--output" && i + 1 < args.GetCount()) cfg.outputFile = args[++i];
        else if (arg == "--wireframe") cfg.wireframe = true;
        else if (arg == "--pbr") cfg.pbr = true;
        else if (arg == "--rotation" && i + 1 < args.GetCount()) cfg.rotationSpeed = ScanDouble(args[++i]);
    }
    return cfg;
}

void RunHeadless(const Config& cfg) {
    StdLogSetup(LOG_FILE|LOG_COUT);
    Cout() << "RunHeadless: creating instance" << "\n";
    Cout().Flush();
    
    Instance* instance = sgCreateInstance();
    Cout() << "RunHeadless: instance created" << "\n";
    Cout().Flush();
    if (!instance) {
        Cout() << "RunHeadless: Failed to create instance" << "\n";
        Cout().Flush();
        return;
    }
    
    Array<Device*> devices = sgEnumerateDevices(instance, cfg.backend);
    Cout() << "RunHeadless: Found " << devices.GetCount() << " device(s)" << "\n";
    Cout().Flush();
    
    Device* device = nullptr;
    if (devices.GetCount() > 0) {
        device = sgCreateDevice(instance, devices[0], cfg.backend);
    } else {
        device = sgCreateDevice(instance, nullptr, cfg.backend);
    }
    
    if (!device) {
        Cout() << "RunHeadless: Failed to create device" << "\n";
        Cout().Flush();
        sgDestroyInstance(instance);
        return;
    }
    
    Cout() << "RunHeadless: Device: " << device->GetName() << ", Backend: " << (int)device->GetBackend() << "\n";
    Cout().Flush();
    
    Cout() << "RunHeadless: Generating teapot mesh..." << "\n";
    Cout().Flush();
    MeshData mesh = GenerateTeapot(1.0f, 16);
    Cout() << "RunHeadless: Teapot mesh: " << mesh.vertices.GetCount() << " vertices, " << mesh.indices.GetCount() << " indices" << "\n";
    Cout().Flush();
    
    Cout() << "RunHeadless: Creating vertex buffer..." << "\n";
    Cout().Flush();
    
    // Create vertex buffer
    BufferDesc vbDesc;
    vbDesc.usage = BufferUsage::Vertex;
    vbDesc.size = mesh.vertices.GetCount() * sizeof(TeapotRenderer::Vertex);
    vbDesc.initialData = mesh.vertices.GetCount() ? &mesh.vertices[0] : nullptr;
    SoftGraphics::Buffer* vertexBuffer = sgCreateBuffer(device, vbDesc);
    Cout() << "RunHeadless: Vertex buffer created" << "\n";
    Cout().Flush();
    
    // Create index buffer
    Cout() << "RunHeadless: Creating index buffer..." << "\n";
    Cout().Flush();
    BufferDesc ibDesc;
    ibDesc.usage = BufferUsage::Index;
    ibDesc.size = mesh.indices.GetCount() * sizeof(uint32_t);
    ibDesc.initialData = mesh.indices.GetCount() ? &mesh.indices[0] : nullptr;
    SoftGraphics::Buffer* indexBuffer = sgCreateBuffer(device, ibDesc);
    Cout() << "RunHeadless: Index buffer created" << "\n";
    Cout().Flush();
    
    // Create uniform buffer
    Cout() << "RunHeadless: Creating uniform buffer..." << "\n";
    Cout().Flush();
    BufferDesc ubDesc;
    ubDesc.usage = BufferUsage::Uniform;
    ubDesc.size = sizeof(UniformData);
    SoftGraphics::Buffer* uniformBuffer = sgCreateBuffer(device, ubDesc);
    Cout() << "RunHeadless: Uniform buffer created" << "\n";
    Cout().Flush();
    
    // Create shaders
    Cout() << "RunHeadless: Compiling vertex shader..." << "\n";
    Cout().Flush();
    ShaderDesc vsDesc;
    vsDesc.stage = ShaderStage::Vertex;
    vsDesc.source = R"(
from typing import TypedDict

class Uniforms(TypedDict):
    model: Mat4
    view: Mat4
    proj: Mat4
    normalMatrix: Mat3
    lightDir: Vec3
    lightColor: Vec3
    ambientColor: Vec3
    baseColor: Vec3
    time: float
    cameraPos: Vec3

def vs_main(pos: Vec3, normal: Vec3, uv: Vec2, uniforms: Uniforms) -> (Vec4, Vec3, Vec3, Vec2):
    worldPos = uniforms.model * Vec4(pos, 1.0)
    viewPos = uniforms.view * worldPos
    clipPos = uniforms.proj * viewPos
    worldNormal = normalize(uniforms.normalMatrix * normal)
    return (clipPos, worldPos.xyz, worldNormal, uv)
)";
    vsDesc.entryPoint = "vs_main";
    
    ShaderDesc fsDesc;
    fsDesc.stage = ShaderStage::Fragment;
    fsDesc.source = R"(
from typing import TypedDict

class Uniforms(TypedDict):
    view: Mat4
    proj: Mat4
    viewProj: Mat4
    model: Mat4
    normalMatrix: Mat3
    lightDir: Vec3
    lightColor: Vec3
    ambientColor: Vec3
    baseColor: Vec3
    time: float

def fs_main(worldPos: Vec3, normal: Vec3, uv: Vec2, uniforms: Uniforms) -> Vec4:
    N = normalize(normal)
    L = normalize(-uniforms.lightDir)
    
    diff = max(dot(N, L), 0.0)
    diffuse = uniforms.lightColor * uniforms.baseColor * diff
    ambient = uniforms.ambientColor * uniforms.baseColor
    
    V = normalize(-worldPos)
    H = normalize(L + V)
    spec = pow(max(dot(N, H), 0.0), 32.0)
    specular = uniforms.lightColor * Vec3(0.3, 0.3, 0.3) * spec
    
    color = ambient + diffuse + specular
    return Vec4(color, 1.0)
)";
    fsDesc.entryPoint = "fs_main";
    
    // Compile shaders
    Cout() << "RunHeadless: Calling sgCompileShader for vertex..." << "\n";
    Cout().Flush();
    ShaderCompileResult vsResult = sgCompileShader(device, vsDesc);
    Cout() << "RunHeadless: Vertex shader compile result: success=" << vsResult.success << "\n";
    if (!vsResult.success) {
        Cout() << "Vertex shader compilation failed: " << vsResult.error << "\n";
    } else {
        Cout() << "RunHeadless: Vertex shader compiled successfully" << "\n";
    }
    Cout().Flush();
    
    Cout() << "RunHeadless: Calling sgCompileShader for fragment..." << "\n";
    Cout().Flush();
    ShaderCompileResult fsResult = sgCompileShader(device, fsDesc);
    Cout() << "RunHeadless: Fragment shader compile result: success=" << fsResult.success << "\n";
    if (!fsResult.success) {
        Cout() << "Fragment shader compilation failed: " << fsResult.error << "\n";
    } else {
        Cout() << "RunHeadless: Fragment shader compiled successfully" << "\n";
    }
    Cout().Flush();
    
    Cout() << "RunHeadless: Creating shader objects..." << "\n";
    Cout().Flush();
    Shader* vertexShader = sgCreateShader(device, vsDesc);
    Shader* fragmentShader = sgCreateShader(device, fsDesc);
    Cout() << "RunHeadless: Shaders created" << "\n";
    Cout().Flush();
    
    // Create pipeline
    Cout() << "RunHeadless: Creating pipeline..." << "\n";
    Cout().Flush();
    
    // Create pipeline
    GraphicsPipelineDesc pipeDesc;
    pipeDesc.shaders.Add(vertexShader);
    pipeDesc.shaders.Add(fragmentShader);
    pipeDesc.topology = PrimitiveTopology::Triangles;
    pipeDesc.depthStencil.depthTestEnable = true;
    pipeDesc.depthStencil.depthWriteEnable = true;
    pipeDesc.depthStencil.depthCompareOp = CompareOp::Less;
    
    pipeDesc.vertexInput.stride = sizeof(TeapotRenderer::Vertex);
    pipeDesc.vertexInput.attributes.Add({"pos", TextureFormat::RGB32F, 0, 0});
    pipeDesc.vertexInput.attributes.Add({"normal", TextureFormat::RGB32F, 12, 0});
    pipeDesc.vertexInput.attributes.Add({"uv", TextureFormat::RG32F, 24, 0});
    
    Pipeline* pipeline = sgCreateGraphicsPipeline(device, pick(pipeDesc));
    Cout() << "RunHeadless: Pipeline created" << "\n";
    Cout().Flush();
    
    // Create render target texture
    Cout() << "RunHeadless: Creating textures..." << "\n";
    Cout().Flush();
    TextureDesc colorDesc;
    colorDesc.type = TextureType::Tex2D;
    colorDesc.format = TextureFormat::RGBA8;
    colorDesc.width = cfg.width;
    colorDesc.height = cfg.height;
    Texture* colorTexture = sgCreateTexture(device, colorDesc);
    
    TextureDesc depthDesc;
    depthDesc.type = TextureType::Tex2D;
    depthDesc.format = TextureFormat::D24S8;
    depthDesc.width = cfg.width;
    depthDesc.height = cfg.height;
    Texture* depthTexture = sgCreateTexture(device, depthDesc);
    
    FramebufferDesc fbDesc;
    fbDesc.colorAttachments.Add(colorTexture);
    fbDesc.depthStencilAttachment = depthTexture;
    Framebuffer* framebuffer = sgCreateFramebuffer(device, pick(fbDesc));
    Cout() << "RunHeadless: Framebuffer created" << "\n";
    Cout().Flush();
    
    // Create command buffer
    Cout() << "RunHeadless: Creating command buffer..." << "\n";
    Cout().Flush();
    CommandBuffer* cmdBuffer = sgCreateCommandBuffer(device);
    Cout() << "RunHeadless: Command buffer created" << "\n";
    Cout().Flush();
    
    // Render loop
    Cout() << "RunHeadless: Starting render loop..." << "\n";
    Cout().Flush();
    int frameCount = 0;
    double startTime = GetTickCount() / 1000.0;
    
    while (cfg.frames == 0 || frameCount < cfg.frames) {
        frameCount++;
        double t = GetTickCount() / 1000.0 - startTime;
        
        Cout() << "RunHeadless: Frame " << frameCount << "..." << "\n";
        Cout().Flush();
        
        // Update uniforms
        UniformData uniforms;
        uniforms.time = float(t);
        
        float radius = 5.0f;
        float camX = radius * sin(t * cfg.rotationSpeed);
        float camZ = radius * cos(t * cfg.rotationSpeed);
        float camY = 2.0f;
        
        Vec3 eye(camX, camY, camZ);
        Vec3 center(0, 0.5f, 0);
        Vec3 up(0, 1, 0);
        
        uniforms.view = LookAt(eye, center, up);
        uniforms.proj = Perspective(45.0f, float(cfg.width) / cfg.height, 0.1f, 100.0f);
        uniforms.viewProj = uniforms.proj * uniforms.view;
        uniforms.model = YRotation(t * cfg.rotationSpeed * 0.5f) * XRotation(-3.14159265359f/2);
        mat3 normalMat = mat3({
            uniforms.model[0][0], uniforms.model[0][1], uniforms.model[0][2],
            uniforms.model[1][0], uniforms.model[1][1], uniforms.model[1][2],
            uniforms.model[2][0], uniforms.model[2][1], uniforms.model[2][2]
        }).GetInverse().GetTransposed();
        uniforms.normalMatrix = normalMat;
        uniforms.lightDir = Normalize(Vec3(1, -1, 0.5f));
        uniforms.lightColor = Vec3(1.0f, 0.95f, 0.9f);
        uniforms.ambientColor = Vec3(0.1f, 0.1f, 0.15f);
        uniforms.baseColor = Vec3(0.8f, 0.2f, 0.2f);
        uniforms.cameraPos = eye;
        
        void* data = uniformBuffer->Map();
        memcpy(data, &uniforms, sizeof(UniformData));
        uniformBuffer->Unmap();
        
        // Record commands
        cmdBuffer->Begin();
        
        Viewport viewport = {0, 0, float(cfg.width), float(cfg.height), 0.0f, 1.0f};
        Scissor scissor = {0, 0, cfg.width, cfg.height};
        
        Array<SoftGraphics::Buffer*> vbuffers;
        vbuffers.Add(vertexBuffer);
        Array<uint64_t> voffsets;
        voffsets.Add((uint64_t)0);
        
        cmdBuffer->CmdBeginRenderPass(framebuffer, viewport, scissor);
        cmdBuffer->CmdBindPipeline(pipeline);
        cmdBuffer->CmdBindVertexBuffers(0, pick(vbuffers), pick(voffsets));
        cmdBuffer->CmdBindIndexBuffer(indexBuffer, 0);
        cmdBuffer->CmdDrawIndexed(mesh.indices.GetCount(), 1, 0, 0, 0);
        cmdBuffer->CmdEndRenderPass();
        
        cmdBuffer->End();
        
        Array<CommandBuffer*> cmdBuffers;
        cmdBuffers.Add(cmdBuffer);
        device->QueueSubmit(cmdBuffers);
        device->WaitIdle();
        
        if (frameCount % 60 == 0) {
            LOG("Frame " << frameCount << " rendered");
        }
    }
    
    // Cleanup
    sgDestroyCommandBuffer(cmdBuffer);
    sgDestroyFramebuffer(framebuffer);
    sgDestroyTexture(depthTexture);
    sgDestroyTexture(colorTexture);
    sgDestroyPipeline(pipeline);
    sgDestroyShader(fragmentShader);
    sgDestroyShader(vertexShader);
    sgDestroyBuffer(uniformBuffer);
    sgDestroyBuffer(indexBuffer);
    sgDestroyBuffer(vertexBuffer);
    sgDestroyDevice(device);
    sgDestroyInstance(instance);
    
    LOG("Headless rendering complete: " << frameCount << " frames");
}

// GUI entry point
int main(int argc, char** argv) {
    Vector<String> args;
    for (int i = 1; i < argc; ++i)
        args.Add(argv[i]);
    Config cfg = ParseArgs(args);
    
    // Override backend from USE flags
    #ifdef flagUSE_VULKAN
    if (cfg.backend == Backend::CPU) cfg.backend = Backend::Vulkan;
    #endif
    #ifdef flagUSE_METAL
    if (cfg.backend == Backend::CPU) cfg.backend = Backend::Metal;
    #endif
    #ifdef flagUSE_DX12
    if (cfg.backend == Backend::CPU) cfg.backend = Backend::DX12;
    #endif
    #ifdef flagUSE_OPENGL
    if (cfg.backend == Backend::CPU) cfg.backend = Backend::OpenGL;
    #endif
    #ifdef flagUSE_CINEMATIC
    if (cfg.backend == Backend::CPU) cfg.backend = Backend::Cinematic;
    #endif
    #ifdef flagUSE_OPENGL_LEGACY
    if (cfg.backend == Backend::CPU) cfg.backend = Backend::OpenGL;
    #endif
    #ifdef flagUSE_XBOX360
    if (cfg.backend == Backend::CPU) cfg.backend = Backend::OpenGL;
    #endif
    #ifdef flagUSE_PS1
    if (cfg.backend == Backend::CPU) cfg.backend = Backend::OpenGL;
    #endif
    
    if (cfg.headless) {
        // Headless mode - no GUI initialization
        if (cfg.frames == 0) cfg.frames = 60;
        RunHeadless(cfg);
    } else {
        // GUI mode
        #include <CtrlLib/CtrlLib.h>
        TeapotWindow win(cfg);
        win.OpenMain();
        win.Run();
    }
    return 0;
}