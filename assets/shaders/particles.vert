#version 450 core
// particles.vert — Instanced billboard particles with procedural shape support.
// Fixed vertex attribute layout matching ParticleSystem.cpp VAO setup.

layout(location = 0) in vec2  aQuadPos;  // unit quad: -0.5..0.5 (non-instanced)

// Per-instance attributes (attrib divisor = 1)
layout(location = 1) in vec2  aPos;      // world position
layout(location = 2) in float aSize;     // pixel size
layout(location = 3) in float aRotation; // rotation in radians
layout(location = 4) in vec4  aColor;    // RGBA
layout(location = 5) in float aShape;    // 0=glow, 1=circle, 2=ring, 3=sparkle

layout(std140, binding = 0) uniform CameraUBO {
    mat4  uViewProj;
    vec2  uCameraPos;
    vec2  uViewportSize;
    float uZoom;
    float _pad[3];
};

out vec2  vQuadUV;  // -0.5..0.5 local quad coordinates
out vec4  vColor;
out float vShape;

void main() {
    // Rotate the local quad position
    float c      = cos(aRotation);
    float s      = sin(aRotation);
    mat2  rot    = mat2(c, -s, s, c);
    vec2  local  = rot * aQuadPos * aSize;

    vQuadUV     = aQuadPos;
    vColor      = aColor;
    vShape      = aShape;

    gl_Position = uViewProj * vec4(aPos + local, 0.0, 1.0);
}
