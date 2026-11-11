// PBR shader - typed Python

// Uniform block layout
struct Uniforms:
    view: Mat4
    proj: Mat4
    viewProj: Mat4
    model: Mat4
    normalMatrix: Mat3
    cameraPos: Vec3
    lightDir: Vec3
    lightColor: Vec3
    ambientColor: Vec3
    baseColor: Vec3
    metallic: float
    roughness: float
    ao: float
    time: float

// BRDF helpers
def DistributionGGX(N: Vec3, H: Vec3, roughness: float) -> float:
    float a = roughness * roughness
    float a2 = a * a
    float NdotH = max(dot(N, H), 0.0)
    float NdotH2 = NdotH * NdotH
    float denom = (NdotH2 * (a2 - 1.0) + 1.0)
    denom = 3.14159265 * denom * denom
    return a2 / denom

def GeometrySchlickGGX(NdotV: float, roughness: float) -> float:
    float r = roughness + 1.0
    float k = (r * r) / 8.0
    return NdotV / (NdotV * (1.0 - k) + k)

def GeometrySmith(N: Vec3, V: Vec3, L: Vec3, roughness: float) -> float:
    float NdotV = max(dot(N, V), 0.0)
    float NdotL = max(dot(N, L), 0.0)
    float ggx1 = GeometrySchlickGGX(NdotV, roughness)
    float ggx2 = GeometrySchlickGGX(NdotL, roughness)
    return ggx1 * ggx2

def FresnelSchlick(cosTheta: float, F0: Vec3) -> Vec3:
    return F0 + (Vec3(1.0) - F0) * pow(1.0 - cosTheta, 5.0)

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
    V = normalize(uniforms.cameraPos - worldPos)
    L = normalize(-uniforms.lightDir)
    H = normalize(V + L)
    
    // Base reflectivity
    Vec3 F0 = mix(Vec3(0.04), uniforms.baseColor, uniforms.metallic)
    
    // Normal Distribution
    float NDF = DistributionGGX(N, H, uniforms.roughness)
    
    // Geometry
    float G = GeometrySmith(N, V, L, uniforms.roughness)
    
    // Fresnel
    Vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0)
    
    // Cook-Torrance BRDF
    Vec3 numerator = NDF * G * F
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001
    Vec3 specular = numerator / denominator
    
    // Diffuse
    Vec3 kD = (Vec3(1.0) - F) * (1.0 - uniforms.metallic)
    Vec3 diffuse = kD * uniforms.baseColor / 3.14159265
    
    // Ambient (simplified)
    Vec3 ambient = uniforms.ambientColor * uniforms.baseColor * uniforms.ao
    
    // Light contribution
    float NdotL = max(dot(N, L), 0.0)
    Vec3 radiance = uniforms.lightColor * NdotL
    Vec3 color = (diffuse + specular) * radiance + ambient
    
    // Tone mapping (ACES filmic)
    color = color / (color + Vec3(1.0))
    color = pow(color, Vec3(1.0 / 2.2)) // Gamma correction
    
    return Vec4(color, 1.0)