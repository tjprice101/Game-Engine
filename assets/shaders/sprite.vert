#version 450 core
// sprite.vert — Sprite to GBuffer (with optional normal map)

layout(location = 0) in vec2  aPos;      // world pixels
layout(location = 1) in vec2  aUV;
layout(location = 2) in vec2  aUVNorm;   // normal map UV
layout(location = 3) in vec4  aColor;
layout(location = 4) in float aTexIdx;
layout(location = 5) in float aEmissive;

layout(std140, binding = 0) uniform CameraUBO {
    mat4  uViewProj;
    vec2  uCameraPos;
    vec2  uViewportSize;
    float uZoom;
    float _pad[3];
};

out vec2  vUV;
out vec2  vUVNorm;
out vec4  vColor;
out float vTexIdx;
out float vEmissive;

void main() {
    vUV       = aUV;
    vUVNorm   = aUVNorm;
    vColor    = aColor;
    vTexIdx   = aTexIdx;
    vEmissive = aEmissive;
    gl_Position = uViewProj * vec4(aPos, 0.0, 1.0);
}
