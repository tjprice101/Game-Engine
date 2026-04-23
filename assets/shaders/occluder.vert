#version 450 core
// occluder.vert — Renders solid tiles as white into the occluder buffer.
// Used as input for 1D polar shadow map generation.

layout(location = 0) in vec2 aVertPos;
layout(location = 1) in vec2 aVertUV;
layout(location = 2) in vec2 aTilePos;
layout(location = 3) in vec2 aAtlasUV;
layout(location = 4) in vec3 aLight;

layout(std140, binding = 0) uniform CameraUBO {
    mat4  uViewProj;
    vec2  uCameraPos;
    vec2  uViewportSize;
    float uZoom;
    float _pad[3];
};

uniform vec2 uTileSize;
uniform vec2 uAtlasCellSize;

out vec2 vUV;

void main() {
    vec2 worldPos = aTilePos + aVertPos * uTileSize;
    vUV = aAtlasUV + aVertUV * uAtlasCellSize;
    gl_Position = uViewProj * vec4(worldPos, 0.0, 1.0);
}
