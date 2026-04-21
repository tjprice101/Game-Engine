#version 450 core
// bloom_blur.frag — Dual-Kawase downsample OR upsample pass.
// A single shader handles both directions via uMode:
//   uMode = 0 → downsample  (uses 4-tap kernel at half-pixel offsets)
//   uMode = 1 → upsample    (uses 8-tap tent filter)

in  vec2  vScreenUV;
out vec4  fragColor;

uniform sampler2D uSrc;
uniform vec2      uTexelSize; // 1.0 / src texture size
uniform int       uMode;      // 0 = downsample, 1 = upsample
uniform float     uBlendFactor; // for upsample blend (0..1)

vec4 downsample(vec2 uv) {
    // 4-tap at offset ± half texel in both axes, plus centre
    vec4 sum = vec4(0.0);
    sum += texture(uSrc, uv + vec2(-uTexelSize.x,  uTexelSize.y) * 0.5);
    sum += texture(uSrc, uv + vec2( uTexelSize.x,  uTexelSize.y) * 0.5);
    sum += texture(uSrc, uv + vec2(-uTexelSize.x, -uTexelSize.y) * 0.5);
    sum += texture(uSrc, uv + vec2( uTexelSize.x, -uTexelSize.y) * 0.5);

    // Additional ring for quality
    float d = 0.5;
    sum += texture(uSrc, uv + vec2(-uTexelSize.x * d, 0.0));
    sum += texture(uSrc, uv + vec2( uTexelSize.x * d, 0.0));
    sum += texture(uSrc, uv + vec2(0.0, -uTexelSize.y * d));
    sum += texture(uSrc, uv + vec2(0.0,  uTexelSize.y * d));

    return sum / 8.0;
}

vec4 upsample(vec2 uv) {
    // 9-tap tent filter (3×3 weighted)
    vec4 sum = vec4(0.0);
    float ox = uTexelSize.x, oy = uTexelSize.y;
    sum += texture(uSrc, uv + vec2(-ox,  oy)) * 1.0;
    sum += texture(uSrc, uv + vec2( 0., oy)) * 2.0;
    sum += texture(uSrc, uv + vec2( ox,  oy)) * 1.0;
    sum += texture(uSrc, uv + vec2(-ox,  0.)) * 2.0;
    sum += texture(uSrc, uv                 ) * 4.0;
    sum += texture(uSrc, uv + vec2( ox,  0.)) * 2.0;
    sum += texture(uSrc, uv + vec2(-ox, -oy)) * 1.0;
    sum += texture(uSrc, uv + vec2( 0., -oy)) * 2.0;
    sum += texture(uSrc, uv + vec2( ox, -oy)) * 1.0;
    return sum / 16.0;
}

void main() {
    if (uMode == 0)
        fragColor = downsample(vScreenUV);
    else
        fragColor = upsample(vScreenUV);
}
