#version 450 core

in vec2 vTexCoord;
out vec4 fragColor;

uniform float uDayTime;   // 0=midnight, 0.5=noon, wraps 0..1
uniform float uSunlight;  // 0..1 precomputed sky brightness

// Sky gradient colours at different times
vec3 skyDay      = vec3(0.38, 0.65, 0.92);   // midday blue
vec3 skyHorizon  = vec3(0.68, 0.82, 0.96);   // horizon lighter
vec3 skyDusk     = vec3(0.85, 0.45, 0.18);   // dusk/dawn orange
vec3 skyNight    = vec3(0.02, 0.02, 0.08);   // night dark blue

// Simple pseudo-random star field
float hash(vec2 p) {
    p = fract(p * vec2(127.1, 311.7));
    p += dot(p, p + 19.19);
    return fract(p.x * p.y);
}

void main() {
    // vTexCoord: (0,0)=bottom-left, (1,1)=top-right  (fullscreen quad)
    float horizonBlend = vTexCoord.y; // 0=top of screen, 1=bottom

    // dayTime: 0=midnight, 0.25=dawn, 0.5=noon, 0.75=dusk
    float sunAngle = uDayTime * 6.28318; // radians
    float sinSun   = sin(sunAngle);  // -1=noon, +1=midnight  (cos = east/west)

    // Night factor: 0=day, 1=night
    float nightFactor = clamp(-sinSun * 2.0 + 0.3, 0.0, 1.0);
    float dawnFactor  = clamp(1.0 - abs(sinSun) * 3.0, 0.0, 1.0); // peaks at dawn/dusk

    // Sky colour
    vec3 topColour    = mix(skyDay, skyNight, nightFactor);
    topColour         = mix(topColour, skyDusk, dawnFactor * 0.7);
    vec3 bottomColour = mix(skyHorizon, skyNight * 1.5, nightFactor);
    bottomColour      = mix(bottomColour, skyDusk * 1.2, dawnFactor * 0.6);

    // Blend top→bottom
    vec3 sky = mix(topColour, bottomColour, horizonBlend);

    // Stars (visible at night)
    if (nightFactor > 0.05) {
        vec2 starCoord = vTexCoord * vec2(80.0, 50.0);
        float star = hash(floor(starCoord));
        float twinkle = hash(floor(starCoord) + vec2(uDayTime * 3.7, 0.0));
        if (star > 0.97) {
            float brightness = (0.5 + 0.5 * sin(twinkle * 62.8)) * nightFactor;
            sky += vec3(brightness * 0.9);
        }
    }

    // Sun disc
    if (nightFactor < 0.8) {
        float sunX = cos(sunAngle + 1.5708); // offset so noon = top
        float sunY = -sinSun;
        vec2 sunDir = normalize(vec2(sunX, sunY));
        vec2 fragDir = normalize(vTexCoord * 2.0 - 1.0 + vec2(0.0, 0.2));
        float sunDot = dot(fragDir, sunDir);
        float sunDisc = smoothstep(0.97, 0.99, sunDot);
        sky = mix(sky, vec3(1.0, 0.98, 0.85), sunDisc * (1.0 - nightFactor));
    }

    // Moon
    if (nightFactor > 0.2) {
        float moonX = cos(sunAngle + 1.5708 + 3.14159);
        float moonY = sinSun;
        vec2 moonDir = normalize(vec2(moonX, moonY));
        vec2 fragDir = normalize(vTexCoord * 2.0 - 1.0 + vec2(0.0, 0.2));
        float moonDot = dot(fragDir, moonDir);
        float moonDisc = smoothstep(0.985, 0.998, moonDot);
        sky = mix(sky, vec3(0.92, 0.94, 1.0), moonDisc * nightFactor);
    }

    fragColor = vec4(sky, 1.0);
}
