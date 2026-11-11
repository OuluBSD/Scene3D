#include "teapot.h"
#include <Core/Core.h>

namespace TeapotRenderer {

using namespace Upp;

// Utah teapot control points (Newell's teapot)
static const float teapotPatches[28][16][3] = {
    // Rim
    {{  0.2f,  0.0f,  1.4f }, {  0.2f, -0.112f,  1.4f }, {  0.112f, -0.2f,  1.4f }, {  0.0f, -0.2f,  1.4f },
     {  1.3375f,  0.0f,  1.4f }, {  1.3375f, -0.749f,  1.4f }, {  0.749f, -1.3375f,  1.4f }, {  0.0f, -1.3375f,  1.4f },
     {  1.4375f,  0.0f,  0.8f }, {  1.4375f, -0.805f,  0.8f }, {  0.805f, -1.4375f,  0.8f }, {  0.0f, -1.4375f,  0.8f },
     {  1.5f,  0.0f,  0.2f }, {  1.5f, -0.84f,  0.2f }, {  0.84f, -1.5f,  0.2f }, {  0.0f, -1.5f,  0.2f }},
    // Body
    {{  1.5f,  0.0f,  0.2f }, {  1.5f, -0.84f,  0.2f }, {  0.84f, -1.5f,  0.2f }, {  0.0f, -1.5f,  0.2f },
     {  1.5f,  0.0f, -0.2f }, {  1.5f, -0.84f, -0.2f }, {  0.84f, -1.5f, -0.2f }, {  0.0f, -1.5f, -0.2f },
     {  1.4375f,  0.0f, -0.8f }, {  1.4375f, -0.805f, -0.8f }, {  0.805f, -1.4375f, -0.8f }, {  0.0f, -1.4375f, -0.8f },
     {  1.3375f,  0.0f, -1.4f }, {  1.3375f, -0.749f, -1.4f }, {  0.749f, -1.3375f, -1.4f }, {  0.0f, -1.3375f, -1.4f }},
    // Bottom
    {{  1.3375f,  0.0f, -1.4f }, {  1.3375f, -0.749f, -1.4f }, {  0.749f, -1.3375f, -1.4f }, {  0.0f, -1.3375f, -1.4f },
     {  0.2f,  0.0f, -1.4f }, {  0.2f, -0.112f, -1.4f }, {  0.112f, -0.2f, -1.4f }, {  0.0f, -0.2f, -1.4f },
     {  0.0f,  0.0f, -1.5f }, {  0.0f,  0.0f, -1.5f }, {  0.0f,  0.0f, -1.5f }, {  0.0f,  0.0f, -1.5f },
     {  0.0f,  0.0f, -1.5f }, {  0.0f,  0.0f, -1.5f }, {  0.0f,  0.0f, -1.5f }, {  0.0f,  0.0f, -1.5f }},
    // Lid
    {{  0.2f,  0.0f,  1.4f }, {  0.2f,  0.112f,  1.4f }, {  0.112f,  0.2f,  1.4f }, {  0.0f,  0.2f,  1.4f },
     {  1.0f,  0.0f,  1.8f }, {  1.0f,  0.56f,  1.8f }, {  0.56f,  1.0f,  1.8f }, {  0.0f,  1.0f,  1.8f },
     {  0.8f,  0.0f,  2.2f }, {  0.8f,  0.448f,  2.2f }, {  0.448f,  0.8f,  2.2f }, {  0.0f,  0.8f,  2.2f },
     {  0.0f,  0.0f,  2.4f }, {  0.0f,  0.0f,  2.4f }, {  0.0f,  0.0f,  2.4f }, {  0.0f,  0.0f,  2.4f }},
    // Handle
    {{  1.3375f,  0.0f,  1.4f }, {  1.5f,  0.0f,  1.0f }, {  1.5f,  0.0f,  0.5f }, {  1.4f,  0.0f,  0.0f },
     {  1.3375f, -0.749f,  1.4f }, {  1.5f, -0.84f,  1.0f }, {  1.5f, -0.84f,  0.5f }, {  1.4f, -0.84f,  0.0f },
     {  0.749f, -1.3375f,  1.4f }, {  0.84f, -1.5f,  1.0f }, {  0.84f, -1.5f,  0.5f }, {  0.74f, -1.4f,  0.0f },
     {  0.0f, -1.3375f,  1.4f }, {  0.0f, -1.5f,  1.0f }, {  0.0f, -1.5f,  0.5f }, {  0.0f, -1.4f,  0.0f }},
    // Spout
    {{  1.3375f,  0.0f, -1.4f }, {  1.5f,  0.0f, -1.0f }, {  1.5f,  0.0f, -0.5f }, {  1.4f,  0.0f,  0.0f },
     {  1.3375f,  0.749f, -1.4f }, {  1.5f,  0.84f, -1.0f }, {  1.5f,  0.84f, -0.5f }, {  1.4f,  0.84f,  0.0f },
     {  0.749f,  1.3375f, -1.4f }, {  0.84f,  1.5f, -1.0f }, {  0.84f,  1.5f, -0.5f }, {  0.74f,  1.4f,  0.0f },
     {  0.0f,  1.3375f, -1.4f }, {  0.0f,  1.5f, -1.0f }, {  0.0f,  1.5f, -0.5f }, {  0.0f,  1.4f,  0.0f }},
};

static const int patchCount = 6; // patches provided above (rim, body, bottom, lid, handle, spout)

// Bernstein basis functions
static float Bernstein(int i, int n, float t) {
    static float C[4][4] = {
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    };
    // For cubic: C(3,i) * t^i * (1-t)^(3-i)
    float comb[4] = {1, 3, 3, 1};
    return comb[i] * pow(t, i) * pow(1 - t, 3 - i);
}

// Evaluate Bezier patch
static Vec3 EvaluatePatch(const float patch[16][3], float u, float v) {
    Vec3 result(0, 0, 0);
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            float bu = Bernstein(i, 3, u);
            float bv = Bernstein(j, 3, v);
            result += Vec3(patch[i * 4 + j][0], patch[i * 4 + j][1], patch[i * 4 + j][2]) * (bu * bv);
        }
    }
    return result;
}

// Compute normal via cross product of partial derivatives
static Vec3 ComputeNormal(const float patch[16][3], float u, float v) {
    const float eps = 0.001f;
    Vec3 p  = EvaluatePatch(patch, u, v);
    Vec3 pu = EvaluatePatch(patch, u + eps, v);
    Vec3 pv = EvaluatePatch(patch, u, v + eps);
    Vec3 du = (pu - p) / eps;
    Vec3 dv = (pv - p) / eps;
    Vec3 normal = Cross(du, dv);
    float lenSq = normal[0]*normal[0] + normal[1]*normal[1] + normal[2]*normal[2];
    if (lenSq < 1e-6f) {
        // Degenerate - return a default up vector
        return Vec3(0, 1, 0);
    }
    float invLen = 1.0f / sqrt(lenSq);
    return Vec3(normal[0] * invLen, normal[1] * invLen, normal[2] * invLen);
}

MeshData GenerateTeapot(float scale, int tessellation) {
    MeshData mesh;
    tessellation = Clamp(tessellation, 4, 32);
    
    // Estimate vertex count
    int vertsPerPatch = (tessellation + 1) * (tessellation + 1);
    mesh.vertices.Reserve(patchCount * vertsPerPatch);
    mesh.indices.Reserve(patchCount * tessellation * tessellation * 6);
    
    int baseVertex = 0;
    
    for (int p = 0; p < patchCount; ++p) {
        const float (*patch)[3] = teapotPatches[p];
        
        // Generate vertices for this patch
        for (int i = 0; i <= tessellation; ++i) {
            float v = float(i) / tessellation;
            for (int j = 0; j <= tessellation; ++j) {
                float u = float(j) / tessellation;
                
                Vertex vertex;
                vertex.position = EvaluatePatch(patch, u, v) * scale;
                vertex.normal = ComputeNormal(patch, u, v);
                vertex.texCoord = Vec2(u, 1.0f - v); // Flip V for texture
                mesh.vertices.Add(vertex);
            }
        }
        
        // Generate indices (two triangles per quad)
        for (int i = 0; i < tessellation; ++i) {
            for (int j = 0; j < tessellation; ++j) {
                int v0 = baseVertex + i * (tessellation + 1) + j;
                int v1 = baseVertex + i * (tessellation + 1) + j + 1;
                int v2 = baseVertex + (i + 1) * (tessellation + 1) + j;
                int v3 = baseVertex + (i + 1) * (tessellation + 1) + j + 1;
                
                // Triangle 1
                mesh.indices.Add(v0);
                mesh.indices.Add(v2);
                mesh.indices.Add(v1);
                // Triangle 2
                mesh.indices.Add(v1);
                mesh.indices.Add(v2);
                mesh.indices.Add(v3);
            }
        }
        
        baseVertex += vertsPerPatch;
    }
    
    // Mirror patches for full teapot (rotate around Y axis)
    int originalCount = mesh.vertices.GetCount();
    for (int p = 0; p < originalCount; ++p) {
        Vertex v = mesh.vertices[p];
        // Mirror X and Z for back side
        v.position[0] = -v.position[0];
        v.normal[0] = -v.normal[0];
        v.texCoord[0] = 1.0f - v.texCoord[0];
        mesh.vertices.Add(v);
    }
    
    int mirroredBase = originalCount;
    int origIndices = mesh.indices.GetCount();
    for (int i = 0; i < origIndices; i += 3) {
        // Reverse winding for back faces
        mesh.indices.Add(mesh.indices[i + 2] + mirroredBase);
        mesh.indices.Add(mesh.indices[i + 1] + mirroredBase);
        mesh.indices.Add(mesh.indices[i] + mirroredBase);
    }
    
    return mesh;
}

MeshData GenerateCube(float size) {
    MeshData mesh;
    float s = size * 0.5f;
    
    // 8 vertices of a cube
    Vec3 positions[8] = {
        {-s, -s, -s}, { s, -s, -s}, { s,  s, -s}, {-s,  s, -s},
        {-s, -s,  s}, { s, -s,  s}, { s,  s,  s}, {-s,  s,  s}
    };
    Vec3 normals[6] = {
        { 0,  0, -1}, { 0,  0,  1}, {-1,  0,  0},
        { 1,  0,  0}, { 0, -1,  0}, { 0,  1,  0}
    };
    Vec2 texCoords[4] = {{0,0}, {1,0}, {1,1}, {0,1}};
    
    // 6 faces, 2 triangles each, 4 vertices per face
    int faces[6][4] = {
        {0, 1, 2, 3}, // -Z
        {4, 7, 6, 5}, // +Z
        {0, 3, 7, 4}, // -X
        {1, 5, 6, 2}, // +X
        {0, 4, 5, 1}, // -Y
        {3, 2, 6, 7}, // +Y
    };
    
    for (int f = 0; f < 6; ++f) {
        int base = mesh.vertices.GetCount();
        for (int i = 0; i < 4; ++i) {
            Vertex v;
            v.position = positions[faces[f][i]];
            v.normal = normals[f];
            v.texCoord = texCoords[i];
            mesh.vertices.Add(v);
        }
        mesh.indices.Add(base + 0);
        mesh.indices.Add(base + 2);
        mesh.indices.Add(base + 1);
        mesh.indices.Add(base + 1);
        mesh.indices.Add(base + 2);
        mesh.indices.Add(base + 3);
    }
    
    return mesh;
}

MeshData GenerateFullscreenTriangle() {
    MeshData mesh;
    // Large triangle covering [-1,1]x[-1,1]
    Vertex v0 = {{-3, -1, 0}, {0, 0, 1}, {0, 2}};
    Vertex v1 = {{ 3, -1, 0}, {0, 0, 1}, {2, 2}};
    Vertex v2 = {{-1,  3, 0}, {0, 0, 1}, {0, 0}};
    mesh.vertices.Add(v0);
    mesh.vertices.Add(v1);
    mesh.vertices.Add(v2);
    mesh.indices.Add((uint32_t)0);
    mesh.indices.Add((uint32_t)1);
    mesh.indices.Add((uint32_t)2);
    return mesh;
}

} // namespace TeapotRenderer