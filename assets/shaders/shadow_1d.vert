#version 450 core
// shadow_1d.vert — Fullscreen quad for 1D polar shadow map generation.

layout(location = 0) in vec2 aPos;  // NDC -1..1

out vec2 vPos; // NDC

void main() {
    vPos = aPos;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
