#version 450 core
// particles.frag — Additive-blended billboard particle.

in  vec2 vUV;
in  vec4 vColor;
out vec4 fragColor;

uniform sampler2D uAtlas;

void main() {
    vec4 texel = texture(uAtlas, vUV);
    if (texel.a < 0.01) discard;
    fragColor = texel * vColor;
}
