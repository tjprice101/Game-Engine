#version 450 core
// trail.vert — Vertex-strip trail rendering.
// Each vertex encodes world position, trail-progress t, and interpolated color.
// Vertices are built CPU-side as a ribbon of quad-strip pairs per frame.

layout(location = 0) in vec2  aPos;    // world position
layout(location = 1) in float aT;      // 0 = head (newest), 1 = tail (oldest)
layout(location = 2) in vec4  aColor;  // lerped trail color (RGBA)

layout(std140, binding = 0) uniform CameraUBO {
    mat4  uViewProj;
    vec2  uCameraPos;
    vec2  uViewportSize;
    float uZoom;
    float _pad[3];
};

out float vT;
out vec4  vColor;

void main() {
    vT          = aT;
    vColor      = aColor;
    gl_Position = uViewProj * vec4(aPos, 0.0, 1.0);
}
