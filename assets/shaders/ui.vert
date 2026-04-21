#version 450 core
// ui.vert — Screen-space UI rendering.

layout(location = 0) in vec2     aPos;     // screen pixels
layout(location = 1) in vec2     aUV;
layout(location = 2) in vec4     aColor;
layout(location = 3) in uint     aTexSlot;

uniform mat4 uProj; // orthographic projection (set by UISystem::endFrame)

out vec2  vUV;
out vec4  vColor;
flat out uint vTexSlot;

void main() {
    vUV      = aUV;
    vColor   = aColor;
    vTexSlot = aTexSlot;
    gl_Position = uProj * vec4(aPos, 0.0, 1.0);
}
