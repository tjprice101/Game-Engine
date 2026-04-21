#version 450 core

in vec2 vTexCoord;
in vec3 vLight;

out vec4 fragColor;

uniform sampler2D uAtlas;

void main() {
    vec4 texel = texture(uAtlas, vTexCoord);
    if (texel.a < 0.05) discard;

    // Apply per-tile lighting (Terraria-style block lighting)
    fragColor = vec4(texel.rgb * vLight, texel.a);
}
