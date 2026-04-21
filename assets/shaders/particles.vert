#version 450 core
// particles.vert — Instanced billboard particles.

layout(location = 0) in vec2  aQuadPos;     // unit quad -0.5..0.5

// Per-instance attributes
layout(location = 1) in vec2  aParticlePos;
layout(location = 2) in float aSize;
layout(location = 3) in float aRotation;
layout(location = 4) in vec4  aColor;

layout(std140, binding = 0) uniform CameraUBO {
    mat4  uViewProj;
    vec2  uCameraPos;
    vec2  uViewportSize;
    float uZoom;
    float _pad[3];
};

uniform sampler2D uAtlas;
uniform vec2      uAtlasUVMin; // UV of particle sprite in atlas
uniform vec2      uAtlasUVMax;

out vec2 vUV;
out vec4 vColor;

void main() {
    // Rotate the quad
    float c = cos(aRotation), s = sin(aRotation);
    mat2  rot = mat2(c, -s, s, c);
    vec2  localPos = rot * aQuadPos * aSize;

    vec2  worldPos = aParticlePos + localPos;

    // UV: map quad -0.5..0.5 → atlas UV
    vUV   = mix(uAtlasUVMin, uAtlasUVMax, aQuadPos + 0.5);
    vColor = aColor;

    gl_Position = uViewProj * vec4(worldPos, 0.0, 1.0);
}
