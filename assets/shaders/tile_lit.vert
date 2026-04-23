#version 450 core
// tile_lit.vert — Writes tiles to GBuffer (albedo+specular / normal+emissive)
// Instanced: one draw call per chunk layer.

layout(location = 0) in vec2 aVertPos;   // 0..1 quad positions (vertex)
layout(location = 1) in vec2 aVertUV;    // 0..1 quad UVs (vertex)
layout(location = 2) in vec2 aTilePos;   // world-pixel position (instance)
layout(location = 3) in vec2 aAtlasUV;  // atlas top-left UV (instance)
layout(location = 4) in vec3 aLight;    // RGB light value (instance)

layout(std140, binding = 0) uniform CameraUBO {
    mat4 uViewProj;
    vec2 uCameraPos;
    vec2 uViewportSize;
    float uZoom;
    float _pad0[3];
};

layout(std140, binding = 1) uniform FrameDataUBO {
    float uTime;
    float uDayTime;     // 0..1
    float uSunlight;    // 0..1 ambient
    float uWindStrength;
    vec4  uFogColor;
};

uniform vec2  uTileSize;     // e.g. (16, 16)
uniform vec2  uAtlasCellSize;// e.g. (1/16, 1/16) of atlas

out vec2  vUV;
out vec3  vLight;
out vec2  vWorldPos;

void main() {
    vec2 worldPos = aTilePos + aVertPos * uTileSize;
    vWorldPos = worldPos;
    vUV       = aAtlasUV + aVertUV * uAtlasCellSize;
    vLight    = aLight;

    gl_Position = uViewProj * vec4(worldPos, 0.0, 1.0);
}
