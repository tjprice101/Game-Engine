#version 450 core
// occluder.frag — Writes 1.0 (white) for solid tiles; discards transparent/air.

in vec2 vUV;
out vec4 fragColor;
uniform sampler2D uAtlas;

void main() {
    vec4 texel = texture(uAtlas, vUV);
    if (texel.a < 0.05) discard;
    fragColor = vec4(1.0);
}
