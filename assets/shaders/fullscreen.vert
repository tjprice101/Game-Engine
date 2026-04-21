#version 450 core
// fullscreen.vert — Shared fullscreen triangle/quad vertex shader.
// Used by: bloom_threshold, bloom_blur, tonemap, shadow_1d, light_point.

layout(location = 0) in vec2 aPos; // NDC -1..1

out vec2 vScreenUV;

void main() {
    vScreenUV   = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
