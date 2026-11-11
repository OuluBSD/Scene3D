// Basic lit shader - typed Python

// Uniform block layout
struct Uniforms:
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

// Vertex shader
def vs_main(pos: Vec3, normal: Vec3, uv: Vec2) -> (Vec4, Vec3, Vec3, Vec2):
    worldPos = mul(uniforms.model, Vec4(pos, 1.0))
    viewPos = mul(uniforms.view, worldPos)
    clipPos = mul(uniforms.proj, viewPos)
    
    worldNormal = normalize(mul(uniforms.normalMatrix, normal))
    
    return (clipPos, worldPos.xyz, worldNormal, uv)

// Fragment shader
def fs_main(worldPos: Vec3, normal: Vec3, uv: Vec2) -> Vec4:
    N = normalize(normal)
    L = normalize(-uniforms.lightDir)
    
    // Diffuse
    float diff = max(dot(N, L), 0.0)
    Vec3 diffuse = uniforms.lightColor * uniforms.baseColor * diff
    
    // Ambient
    Vec3 ambient = uniforms.ambientColor * uniforms.baseColor
    
    // Specular (Blinn-Phong)
    Vec3 V = normalize(-worldPos)
    Vec3 H = normalize(L + V)
    float spec = pow(max(dot(N, H), 0.0), 32.0)
    Vec3 specular = uniforms.lightColor * Vec3(0.3, 0.3, 0.3) * spec
    
    Vec3 color = ambient + diffuse + specular
    
    return Vec4(color, 1.0)