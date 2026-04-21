#version 450 core
// ui.frag — Screen-space UI with multi-texture support.
// Up to 8 texture slots; slot 0 means colour-only (no texture).

in  vec2  vUV;
in  vec4  vColor;
flat in uint vTexSlot;

out vec4 fragColor;

uniform sampler2D uTextures[8];

void main() {
    if (vTexSlot == 0u) {
        // Solid colour, no texture
        fragColor = vColor;
    } else {
        uint s = vTexSlot - 1u; // convert 1-based slot to 0-based array index
        vec4 texel;
        if      (s == 0u) texel = texture(uTextures[0], vUV);
        else if (s == 1u) texel = texture(uTextures[1], vUV);
        else if (s == 2u) texel = texture(uTextures[2], vUV);
        else if (s == 3u) texel = texture(uTextures[3], vUV);
        else if (s == 4u) texel = texture(uTextures[4], vUV);
        else if (s == 5u) texel = texture(uTextures[5], vUV);
        else if (s == 6u) texel = texture(uTextures[6], vUV);
        else               texel = texture(uTextures[7], vUV);

        fragColor = texel * vColor;
    }

    if (fragColor.a < 0.004) discard;
}
