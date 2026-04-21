#pragma once
// Fast and portable Perlin noise (improved noise, Ken Perlin 2002).
// Self-contained header-only implementation.

#include <cmath>
#include <array>

class Noise {
public:
    explicit Noise(unsigned seed = 0) { reseed(seed); }

    void reseed(unsigned seed) {
        for (int i = 0; i < 256; ++i) m_p[i] = (uint8_t)i;
        // Fisher-Yates shuffle with LCG
        unsigned s = seed;
        for (int i = 255; i > 0; --i) {
            s = s * 1664525u + 1013904223u;
            int j = (int)((s >> 8) % (i+1));
            std::swap(m_p[i], m_p[j]);
        }
        for (int i = 0; i < 256; ++i) m_p[i+256] = m_p[i];
    }

    // 2D Perlin noise, returns [-1, 1]
    float noise2(float x, float y) const {
        int xi = (int)std::floor(x) & 255;
        int yi = (int)std::floor(y) & 255;
        float dx = x - std::floor(x);
        float dy = y - std::floor(y);
        float u = fade(dx), v = fade(dy);

        int aa = m_p[m_p[xi]+yi];
        int ab = m_p[m_p[xi]+yi+1];
        int ba = m_p[m_p[xi+1]+yi];
        int bb = m_p[m_p[xi+1]+yi+1];

        return lerp(v,
               lerp(u, grad2(aa, dx,   dy  ),
                       grad2(ba, dx-1, dy  )),
               lerp(u, grad2(ab, dx,   dy-1),
                       grad2(bb, dx-1, dy-1)));
    }

    // Fractal Brownian Motion — octaves of noise stacked together
    float fbm2(float x, float y,
               int   octaves   = 6,
               float lacunarity= 2.0f,
               float gain      = 0.5f) const
    {
        float value = 0.f, amp = 0.5f, freq = 1.f;
        for (int i = 0; i < octaves; ++i) {
            value += amp * noise2(x * freq, y * freq);
            amp  *= gain;
            freq *= lacunarity;
        }
        return value;
    }

    // Convenience: remap to [0, 1]
    float noise2_01(float x, float y) const { return noise2(x,y) * 0.5f + 0.5f; }
    float fbm2_01  (float x, float y, int o=6, float l=2.f, float g=.5f) const {
        return fbm2(x,y,o,l,g) * 0.5f + 0.5f;
    }

private:
    static float fade(float t) { return t*t*t*(t*(t*6.f-15.f)+10.f); }
    static float lerp(float t, float a, float b) { return a + t*(b-a); }

    static float grad2(int hash, float x, float y) {
        switch (hash & 3) {
            case 0: return  x + y;
            case 1: return -x + y;
            case 2: return  x - y;
            default:return -x - y;
        }
    }

    uint8_t m_p[512] = {};
};
