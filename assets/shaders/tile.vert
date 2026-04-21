#version 450 core

// ---- Per-vertex (shared quad, 4 vertices) ----
layout(location = 0) in vec2 aVertPos;   // -0.5 .. +0.5
layout(location = 1) in vec2 aVertUV;    //  0   ..  1

// ---- Per-instance ----
layout(location = 2) in vec2  aTilePos;    // World-space top-left of tile (pixels)
layout(location = 3) in vec2  aAtlasUV;   // Top-left UV in atlas texture
layout(location = 4) in vec3  aLight;     // RGB light value 0..1

uniform mat4  uViewProj;
uniform float uTileSize;        // Pixels per tile (e.g. 16.0)
uniform float uAtlasCellSize;   // 1.0 / atlas grid dimension  (e.g. 0.125 for 8x8)

out vec2 vTexCoord;
out vec3 vLight;

void main() {
    // aVertPos is 0..1; scale to tile pixel size and offset from tile origin
    vec2 worldPos = aTilePos + aVertPos * uTileSize;
    gl_Position   = uViewProj * vec4(worldPos, 0.0, 1.0);
    vTexCoord     = aAtlasUV + aVertUV * uAtlasCellSize;
    vLight        = aLight;
}
