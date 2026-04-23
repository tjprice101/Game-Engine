#version 450 core
// particles.frag — Procedural particle shapes (no atlas required).
// Shape 0: soft glow     — smooth Gaussian falloff, bright core
// Shape 1: solid circle  — hard-edged circle with slight soft rim
// Shape 2: bloom ring    — hollow expanding ring
// Shape 3: sparkle/cross — four-pointed star / cross motif (music-note inspired)

in  vec2  vQuadUV;  // -0.5..0.5 local quad coords
in  vec4  vColor;
in  float vShape;
out vec4  fragColor;

void main() {
    // Distance from center (0=center, 1=corner, 0.5=edge)
    float d = length(vQuadUV) * 2.0;  // 0..sqrt(2)

    float alpha;
    vec3  col = vColor.rgb;

    int shape = int(vShape + 0.5);

    if (shape == 0) {
        // ---- Soft glow (Gaussian) -------------------------------------------
        float g = exp(-d * d * 3.5);
        g = g * g;
        alpha = g;
        // Bright core: HDR-compatible (value > 1 where bloom picks it up)
        col = vColor.rgb * (1.0 + g * 1.8);

    } else if (shape == 1) {
        // ---- Solid circle ---------------------------------------------------
        if (d > 1.0) discard;
        alpha = 1.0 - smoothstep(0.6, 1.0, d);
        col   = vColor.rgb;

    } else if (shape == 2) {
        // ---- Bloom ring (expanding halo) ------------------------------------
        float ring = abs(d - 0.6);  // 0 at ring radius 0.6
        alpha = 1.0 - smoothstep(0.0, 0.25, ring);
        col   = vColor.rgb * 2.0;   // bright bloom ring

    } else {
        // ---- Sparkle / cross ------------------------------------------------
        vec2 uv = abs(vQuadUV);
        // Four-pointed star using smoothed cross + diagonal rays
        float crossH = (1.0 - smoothstep(0.0, 0.07, uv.y)) * (1.0 - smoothstep(0.0, 0.45, uv.x));
        float crossV = (1.0 - smoothstep(0.0, 0.07, uv.x)) * (1.0 - smoothstep(0.0, 0.45, uv.y));
        float diag1  = (1.0 - smoothstep(0.0, 0.04, abs(uv.x - uv.y)))
                     * (1.0 - smoothstep(0.0, 0.3, uv.x + uv.y));
        alpha = clamp(crossH + crossV + diag1 * 0.5, 0.0, 1.0);
        col   = vColor.rgb * (1.0 + alpha * 1.2);
    }

    if (alpha < 0.005) discard;
    fragColor = vec4(col, vColor.a * alpha);
}
