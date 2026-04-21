#version 450 core
// bloom_threshold.frag — Extract pixels above luminance threshold.
// Fullscreen pass; outputs pixels with brightness > uThreshold, else black.

in  vec2 vScreenUV;
out vec4 fragColor;

uniform sampler2D uScene;
uniform float     uThreshold; // e.g. 1.0
uniform float     uKnee;      // soft knee (e.g. 0.2)

void main() {
    vec3 color = texture(uScene, vScreenUV).rgb;

    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));

    // Soft threshold with quadratic knee
    float rq = clamp(brightness - uThreshold + uKnee, 0.0, 2.0 * uKnee);
    rq = (rq * rq) / (4.0 * uKnee + 0.00001);
    float weight = max(rq, brightness - uThreshold) / max(brightness, 0.00001);

    fragColor = vec4(color * weight, 1.0);
}
