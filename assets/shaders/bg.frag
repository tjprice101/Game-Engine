#version 450 core
// bg.frag — Atmospheric sky background
//
// Features:
//   - Physically-inspired Rayleigh scattering gradient
//   - Procedural FBM cloud layers (animated)
//   - Twinkling star field
//   - Sun disc with halo / corona glow
//   - Moon disc with subtle crescent
//   - Aurora borealis at night
//   - Dawn/dusk sun shaft rays
//   - Smooth day → dusk → night transitions

in vec2 vTexCoord;      // (0,0)=top-left, (1,1)=bottom-right (fullscreen quad)
out vec4 fragColor;

uniform float uDayTime;  // 0=midnight, 0.25=dawn, 0.5=noon, 0.75=dusk, wraps 0..1
uniform float uSunlight; // 0..1 precomputed sky brightness

layout(std140, binding = 1) uniform FrameDataUBO {
    float uTime;
    float uDayTimeUBO;   // same as uDayTime but from UBO
    float uSunlightUBO;
    float uWindStrength;
    vec4  uFogColor;
};

// ---- Hash / noise -----------------------------------------------------------
float hash1(float n) { return fract(sin(n) * 43758.5453123); }
float hash1(vec2  p) { return fract(sin(dot(p, vec2(127.1,311.7))) * 43758.5453); }
vec2  hash2(vec2  p) {
    p = vec2(dot(p,vec2(127.1,311.7)), dot(p,vec2(269.5,183.3)));
    return fract(sin(p)*43758.5453123);
}

// Value noise
float vnoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash1(i+vec2(0,0)), hash1(i+vec2(1,0)), u.x),
               mix(hash1(i+vec2(0,1)), hash1(i+vec2(1,1)), u.x), u.y);
}

// FBM (4 octaves)
float fbm(vec2 p) {
    float v = 0.0, a = 0.5;
    mat2  rot = mat2(cos(0.5),sin(0.5),-sin(0.5),cos(0.5));
    for (int i = 0; i < 4; i++) {
        v += a * vnoise(p);
        p  = rot * p * 2.1;
        a *= 0.5;
    }
    return v;
}

// ---- Sun / moon position (fixed circular orbit) ----------------------------
// dayTime 0=midnight, 0.5=noon.  sin(angle) peaks at dawn/dusk, -sin peaks at noon.
vec2 bodyDir(float angle) {
    return vec2(cos(angle), -sin(angle)); // Y+ = up on screen
}

// ---- Main -------------------------------------------------------------------
void main() {
    // vTexCoord: (0,0)=top-left, (1,1)=bottom-right
    // Remap so y=0 is top, y=1 is bottom (horizon at bottom)
    vec2 uv = vTexCoord;
    // Horizon blend: 0=top of sky, 1=horizon
    float horizonBlend = pow(uv.y, 0.7);

    // Day cycle
    float sunAngle  = (uDayTime - 0.25) * 6.2831853; // 0.25 offset: noon = up
    float nightFrac = clamp(sin(sunAngle - 3.14159) * 1.8 + 0.3, 0.0, 1.0);
    float dawnFrac  = clamp(1.0 - abs(cos(sunAngle)) * 2.5, 0.0, 1.0);
    float dayFrac   = clamp(-sin(sunAngle) * 1.5 + 0.5, 0.0, 1.0);

    // ---- Sky gradient -------------------------------------------------------
    // Rayleigh-inspired: zenith dark blue → horizon lighter, then dusk orange
    vec3 zenithDay     = vec3(0.10, 0.40, 0.88);
    vec3 horizonDay    = vec3(0.55, 0.75, 0.98);
    vec3 zenithDusk    = vec3(0.22, 0.15, 0.35);
    vec3 horizonDusk   = vec3(0.95, 0.40, 0.10);
    vec3 zenithNight   = vec3(0.01, 0.01, 0.06);
    vec3 horizonNight  = vec3(0.03, 0.03, 0.10);

    vec3 zenith   = mix(mix(zenithDay, zenithNight, nightFrac), zenithDusk, dawnFrac * 0.8);
    vec3 horizon  = mix(mix(horizonDay, horizonNight, nightFrac), horizonDusk, dawnFrac * 0.9);

    // Extra warm horizon glow around sun position
    float sunX = cos(sunAngle);
    float horizonGlowStr = dawnFrac * (1.0 - nightFrac) * 0.5;
    float horizonGlow    = exp(-abs(uv.x - (sunX * 0.5 + 0.5)) * 4.0) * horizonGlowStr;
    horizon = mix(horizon, horizonDusk * 1.3, horizonGlow);

    vec3 sky = mix(zenith, horizon, horizonBlend);

    // ---- Stars (night) ------------------------------------------------------
    if (nightFrac > 0.05) {
        // Packed star field using cell hash
        vec2 starUV = uv * vec2(120.0, 70.0);
        vec2 starCell = floor(starUV);
        float starVal  = hash1(starCell);
        if (starVal > 0.92) {
            vec2 within = fract(starUV) - 0.5;
            float distStar = dot(within, within);
            float twinkle  = 0.6 + 0.4 * sin(uTime * (3.0 + starVal * 7.0));
            float bright   = (1.0 - smoothstep(0.0, 0.06, distStar)) * twinkle * nightFrac;
            float colorBias = hash1(starCell + vec2(7.3));
            vec3 starColor  = mix(vec3(0.8, 0.9, 1.0), vec3(1.0, 0.95, 0.75), colorBias);
            sky += starColor * bright * 0.9;
        }
    }

    // ---- Aurora borealis (night, upper sky) ---------------------------------
    if (nightFrac > 0.3 && uv.y < 0.5) {
        float auroraY  = uv.y / 0.5; // 0=top, 1=mid
        float auroraT  = uTime * 0.15;
        float auroraWave  = fbm(vec2(uv.x * 3.5 + auroraT, auroraY * 2.0 + auroraT * 0.3));
        float auroraWave2 = fbm(vec2(uv.x * 2.0 - auroraT * 0.5, auroraY * 1.5 + 0.7));
        float aurora = (auroraWave * auroraWave2);
        aurora = smoothstep(0.2, 0.6, aurora);
        aurora *= (1.0 - auroraY) * (nightFrac - 0.3) / 0.7; // fade at bottom + day fade
        // Colour: green → teal → purple sweep
        float hueShift = fbm(vec2(uv.x + auroraT * 0.2, 1.7));
        vec3 auroraColor = mix(vec3(0.0, 0.9, 0.4), vec3(0.4, 0.2, 0.9), hueShift);
        sky += auroraColor * aurora * 0.35;
    }

    // ---- Procedural clouds (day / dawn) -------------------------------------
    float cloudOpacity = dayFrac * (1.0 - nightFrac * 0.9);
    if (cloudOpacity > 0.02 && uv.y > 0.15) {
        // Two cloud layers at different scales and speeds
        float windX  = uTime * 0.012 * (1.0 + uWindStrength * 0.5);
        vec2  cUV1   = vec2(uv.x * 3.5 + windX, uv.y * 2.5 + 0.3);
        vec2  cUV2   = vec2(uv.x * 2.0 - windX * 0.6, uv.y * 1.8 + 0.8);
        float c1     = fbm(cUV1) + fbm(cUV1 * 1.7) * 0.5;
        float c2     = fbm(cUV2) * 0.7;
        float cloud  = smoothstep(0.55, 0.85, c1) + smoothstep(0.55, 0.90, c2) * 0.5;
        cloud       *= (uv.y - 0.15) / 0.85; // fade near top
        cloud        = clamp(cloud, 0.0, 1.0) * cloudOpacity;

        // Cloud colour: white + hint of sky colour at edges, warm at dawn/dusk
        vec3 cloudColor = mix(vec3(0.93, 0.94, 0.97), vec3(1.0, 0.85, 0.65), dawnFrac * 0.5);
        sky = mix(sky, cloudColor, cloud * 0.85);
        // Cloud shadow (darker underside)
        sky *= 1.0 - cloud * 0.12;
    }

    // ---- Sun disc + corona + god rays ---------------------------------------
    if (nightFrac < 0.95) {
        vec2 sunDir = bodyDir(sunAngle);
        // Map uv to -1..1 space with horizon bias
        vec2 fragDir = vec2(uv.x * 2.0 - 1.0, (1.0 - uv.y) * 1.6 - 0.8);
        float sunDot  = dot(normalize(fragDir), sunDir);
        float sunLen  = length(fragDir);

        // Disc
        float sunDisc = smoothstep(0.985, 0.9975, sunDot);
        // Corona halo (Gaussian)
        float corona  = exp(-max(0.0, 1.0 - sunDot) * 18.0) * 0.3;
        // Atmospheric glow near horizon
        float atmos   = exp(-abs(uv.y - 0.8) * 6.0) * exp(-abs(uv.x - (sunDir.x*0.5+0.5)) * 3.0) * 0.25;

        vec3 sunColor = mix(vec3(1.0, 0.98, 0.90), vec3(1.0, 0.55, 0.15), dawnFrac * 0.6);
        float sunVis  = 1.0 - nightFrac;
        sky = mix(sky, sunColor, sunDisc * sunVis);
        sky += sunColor * (corona + atmos) * sunVis * (1.0 - nightFrac * 0.8);

        // God rays (visible at dawn/dusk near horizon)
        if (dawnFrac > 0.1 && uv.y > 0.4) {
            float rayAngle = atan(fragDir.y, fragDir.x);
            float rayBurst = max(0.0, cos(rayAngle * 18.0)) * 0.5 + 0.5;
            rayBurst = pow(rayBurst, 6.0);
            float rayDist  = 1.0 - clamp(length(fragDir - sunDir) * 0.7, 0.0, 1.0);
            float rays     = rayBurst * rayDist * dawnFrac * 0.18;
            sky += sunColor * rays * sunVis;
        }
    }

    // ---- Moon disc ----------------------------------------------------------
    if (nightFrac > 0.15) {
        float moonAngle = sunAngle + 3.14159;
        vec2  moonDir   = bodyDir(moonAngle);
        vec2  fragDir   = vec2(uv.x * 2.0 - 1.0, (1.0 - uv.y) * 1.6 - 0.8);
        float moonDot   = dot(normalize(fragDir), moonDir);
        float moonDisc  = smoothstep(0.988, 0.999, moonDot);
        // Crescent shadow (offset disc)
        vec2  crescentOff = moonDir * 0.03 + vec2(0.015, 0.0);
        float crescent    = dot(normalize(fragDir - crescentOff), moonDir);
        float shadowDisc  = smoothstep(0.988, 0.999, crescent);
        moonDisc = max(0.0, moonDisc - shadowDisc * 0.7);

        vec3 moonColor = vec3(0.90, 0.93, 1.00);
        sky = mix(sky, moonColor, moonDisc * nightFrac);
        // Soft moon glow
        float moonGlow = exp(-max(0.0, 1.0 - moonDot) * 30.0) * 0.07 * nightFrac;
        sky += moonColor * moonGlow;
    }

    // ---- Final gamma + clamp ------------------------------------------------
    sky = pow(max(sky, vec3(0.0)), vec3(0.9)); // slight gamma lift for vibrancy
    fragColor = vec4(clamp(sky, 0.0, 1.5), 1.0); // allow slight HDR for bloom pickup
}
