#version 450 core

in vec2 vTexCoord;
in vec4 vColor;

out vec4 fragColor;

uniform sampler2D uSprite;

void main() {
    vec4 texel = texture(uSprite, vTexCoord);
    // If no texture bound, texel will be (0,0,0,0) — use solid color instead
    if (texel.a < 0.01) {
        fragColor = vColor;
    } else {
        fragColor = texel * vColor;
        if (fragColor.a < 0.05) discard;
    }
}
