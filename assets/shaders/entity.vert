#version 450 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;

uniform mat4 uViewProj;
uniform mat4 uModel;

out vec2 vTexCoord;
out vec4 vColor;

void main() {
    gl_Position = uViewProj * uModel * vec4(aPos, 0.0, 1.0);
    vTexCoord   = aTexCoord;
    vColor      = aColor;
}
