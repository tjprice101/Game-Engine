#version 450 core
// tile_lit.frag — GBuffer output for deferred lighting
// attachment 0: albedo.rgb + specular.a  (RGBA8)
// attachment 1: normal.xy + emissive.z   (RGB16F)

in vec2  vUV;
in vec3  vLight;
in vec2  vWorldPos;

layout(location = 0) out vec4 gAlbedoSpec;
layout(location = 1) out vec3 gNormalEmissive;

uniform sampler2D uAtlas;

layout(std140, binding = 1) uniform FrameDataUBO {
    float uTime;
    float uDayTime;
    float uSunlight;
    float uWindStrength;
    vec4  uFogColor;
};

// ---- Sobel normal derivation ------------------------------------------------
// Computes a 2D surface normal from luminance differences in the atlas.
// Returns encoded XY normal (nz is reconstructed in lighting pass).
vec2 sobelNormal(vec2 uv, vec2 cellSize) {
    float tl = dot(texture(uAtlas, uv + cellSize * vec2(-1,-1)).rgb, vec3(0.299,0.587,0.114));
    float  l = dot(texture(uAtlas, uv + cellSize * vec2(-1, 0)).rgb, vec3(0.299,0.587,0.114));
    float bl = dot(texture(uAtlas, uv + cellSize * vec2(-1, 1)).rgb, vec3(0.299,0.587,0.114));
    float  t = dot(texture(uAtlas, uv + cellSize * vec2( 0,-1)).rgb, vec3(0.299,0.587,0.114));
    float  b = dot(texture(uAtlas, uv + cellSize * vec2( 0, 1)).rgb, vec3(0.299,0.587,0.114));
    float tr = dot(texture(uAtlas, uv + cellSize * vec2( 1,-1)).rgb, vec3(0.299,0.587,0.114));
    float  r = dot(texture(uAtlas, uv + cellSize * vec2( 1, 0)).rgb, vec3(0.299,0.587,0.114));
    float br = dot(texture(uAtlas, uv + cellSize * vec2( 1, 1)).rgb, vec3(0.299,0.587,0.114));

    float gx = -tl - 2.0*l - bl + tr + 2.0*r + br;
    float gy = -tl - 2.0*t - tr + bl + 2.0*b + br;
    return vec2(gx, gy) * 0.5;
}

void main() {
    vec4 texel = texture(uAtlas, vUV);
    if (texel.a < 0.05) discard;

    // Albedo modulated by BFS light
    vec3 albedo = texel.rgb;

    // Sobel normal from atlas texel neighbourhood (atlas texel size = 1/atlasW)
    vec2 texelSize = vec2(1.0/128.0, 1.0/128.0); // atlas is 128x128 (8x8 grid of 16px tiles)
    vec2 n2 = sobelNormal(vUV, texelSize);
    // Encode to 0..1 range for 16F buffer storage
    vec2 encodedN = n2 * 0.5 + 0.5;

    // Specular: derived from texture brightness (bright tiles = more specular)
    float spec = dot(albedo, vec3(0.2126, 0.7152, 0.0722)) * 0.4;

    // Emissive: tiles with high red channel and low blue/green are torches/lava
    float emissive = max(0.0, albedo.r - albedo.b - albedo.g * 0.5) * 0.8;

    gAlbedoSpec     = vec4(albedo, spec);
    gNormalEmissive = vec3(encodedN, emissive);
}
