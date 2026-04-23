#version 450 core
// beam.frag — Glowing energy beam with five style variants.
// UV.x = along beam (0..1),  UV.y = across beam (-1..1 center = 0).

in  vec2 vUV;
in  vec4 vColor;
out vec4 fragColor;

uniform float uTime;   // cumulative seconds (for animated styles)
uniform int   uStyle;  // 0=laser, 1=ribbon, 2=spiral, 3=chain, 4=radiant

void main() {
    float along  = vUV.x;           // 0..1 along beam length
    float across = vUV.y;           // -1..1 across beam (0 = center)
    float dist   = abs(across);     // 0=center, 1=edge

    // ---- End fade -----------------------------------------------------------
    float endFade = smoothstep(0.0, 0.08, along) * smoothstep(1.0, 0.92, along);

    // ---- Core glow (shared by all styles) -----------------------------------
    float core = exp(-dist * dist * 8.0);        // sharp bright core
    float halo = exp(-dist * dist * 2.0) * 0.4;  // wide soft halo

    float totalGlow = core + halo;

    // ---- Style variants -----------------------------------------------------
    if (uStyle == 1) {
        // Ribbon: sinusoidal cross-section wave
        float wave = sin(along * 14.0 + uTime * 5.0) * 0.25 + 1.0;
        totalGlow *= wave;

    } else if (uStyle == 2) {
        // Spiral: helical bands
        float angle  = along * 24.0 + uTime * 7.0;
        float spiral = sin(angle + across * 8.0) * 0.5 + 0.5;
        totalGlow *= 0.4 + spiral * 0.6;

    } else if (uStyle == 3) {
        // Chain: discrete pulsing segments
        float seg = abs(sin(along * 9.0 - uTime * 4.0));
        totalGlow *= 0.2 + seg * 0.8;

    } else if (uStyle == 4) {
        // Radiant: star-ray cross pattern
        float ray1 = exp(-dist * dist * 3.0);
        float ray2 = exp(-pow(abs(across) - 0.5, 2.0) * 20.0) * 0.3;
        totalGlow  = max(ray1, ray2);
    }
    // Style 0 (Laser): unmodified sharp core — default path above.

    totalGlow *= endFade;

    float finalAlpha = clamp(totalGlow, 0.0, 1.0) * vColor.a;
    if (finalAlpha < 0.004) discard;

    // Brighten the core (HDR headroom since we're in the HDR FBO)
    vec3 col = vColor.rgb * (1.0 + core * 1.5);
    fragColor = vec4(col, finalAlpha);
}
