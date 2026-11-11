// Wireframe shader - typed Python

// Uniform block layout
struct Uniforms:
    view: Mat4
    proj: Mat4
    viewProj: Mat4
    model: Mat4
    normalMatrix: Mat3
    wireColor: Vec3
    fillColor: Vec3
    lineWidth: float
    time: float

// Vertex shader
def vs_main(pos: Vec3, normal: Vec3, uv: Vec2) -> (Vec4, Vec3, Vec3, Vec2):
    worldPos = mul(uniforms.model, Vec4(pos, 1.0))
    viewPos = mul(uniforms.view, worldPos)
    clipPos = mul(uniforms.proj, viewPos)
    
    worldNormal = normalize(mul(uniforms.normalMatrix, normal))
    
    return (clipPos, worldPos.xyz, worldNormal, uv)

// Fragment shader - solid fill
def fs_fill(worldPos: Vec3, normal: Vec3, uv: Vec2) -> Vec4:
    N = normalize(normal)
    L = normalize(Vec3(0.5, 1.0, 0.3))
    float diff = max(dot(N, L), 0.0)
    Vec3 color = uniforms.fillColor * (0.3 + 0.7 * diff)
    return Vec4(color, 1.0)

// Fragment shader - wireframe overlay
def fs_wire(worldPos: Vec3, normal: Vec3, uv: Vec2) -> Vec4:
    // Barycentric coordinates for wireframe
    // This is a simplified version - real wireframe needs geometry shader
    // or barycentric coordinates from vertex attributes
    return Vec4(uniforms.wireColor, 1.0)

// Compute shader for wireframe edge detection (optional)
def cs_edge_detect() -> void:
    // Not implemented in this example
    pass