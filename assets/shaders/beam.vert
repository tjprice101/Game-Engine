#version 450 core
// beam.vert — Energy beam quad rendering.
// Each beam is a CPU-generated quad strip.  UV.x = along beam, UV.y = across.

layout(location = 0) in vec2  aPos;   // world position
layout(location = 1) in vec2  aUV;    // (along 0..1, across -1..1 from center)
layout(location = 2) in vec4  aColor; // beam color; alpha = intensity multiplier

layout(std140, binding = 0) uniform CameraUBO {
    mat4  uViewProj;
    vec2  uCameraPos;
    vec2  uViewportSize;
    float uZoom;
    float _pad[3];
};

out vec2 vUV;
out vec4 vColor;

void main() {
    vUV         = aUV;
    vColor      = aColor;
    gl_Position = uViewProj * vec4(aPos, 0.0, 1.0);
}
