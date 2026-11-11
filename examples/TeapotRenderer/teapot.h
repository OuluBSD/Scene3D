#pragma once

#include <SoftGraphics/SoftGraphics.h>

namespace TeapotRenderer {

using SoftGraphics::Vec3;
using SoftGraphics::Vec2;

struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec2 texCoord;
};

struct MeshData {
    Upp::Array<Vertex> vertices;
    Upp::Array<uint32_t> indices;
};

// Generate classic Utah teapot using Newell's algorithm
MeshData GenerateTeapot(float scale = 1.0f, int tessellation = 8);

// Generate unit cube for testing
MeshData GenerateCube(float size = 1.0f);

// Generate fullscreen triangle for post-processing
MeshData GenerateFullscreenTriangle();

} // namespace TeapotRenderer