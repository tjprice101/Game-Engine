#version 450 core
// trail.frag — Gradient + soft-edge fade for ribbon trails.

in  float vT;
in  vec4  vColor;
out vec4  fragColor;

uniform float uFadeStart; // t at which alpha begins fading (default 0.35)

void main() {
    // Fade alpha towards tail
    float alpha = 1.0 - smoothstep(uFadeStart, 1.0, vT);

    // Boost the core brightness slightly for a glow-like appearance
    vec3 boosted = vColor.rgb * (1.0 + (1.0 - vT) * 0.6);

    fragColor = vec4(boosted, vColor.a * alpha);

    if (fragColor.a < 0.005) discard;
}
