#pragma once

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#define LN2 0.69314718055994530942f
#define SQRT2 1.414213562373095f

#define PI 3.1415926535897932384626433832795029f
#define TAU (2.0f * PI)
#define HALF_PI (0.5f * PI)
#define INV_TAU (1.0f / TAU)
#define INV_PI (1.0f / PI)
#define INV_HALF_PI (1.0f / HALF_PI)

#define INV_RAND_MAX (float)(1.0 / UINT32_MAX)
#define INV_RAND_MAX2 (float)(2.0 / UINT32_MAX)

static uint32_t ctss__rndX = 0xdecafbad;
static uint32_t ctss__rndY = 0x2fa9d05b;
static uint32_t ctss__rndZ = 0x041f67e3;
static uint32_t ctss__rndW = 0x5c83ec1a;

// xorshift128 - https://en.wikipedia.org/wiki/Xorshift
static inline uint32_t ctss_rand() {

    uint32_t t = ctss__rndX;
    t ^= t << 11;
    t ^= t >> 8;
    ctss__rndX = ctss__rndY; ctss__rndY = ctss__rndZ; ctss__rndZ = ctss__rndW;
    ctss__rndW ^= ctss__rndW >> 19;
    return ctss__rndW ^= t;
}

static inline float ctss_randf(const float min, const float max) {
    return min + (float)ctss_rand() * INV_RAND_MAX * (max - min);
}

static inline float ctss_normrandf() {
    return (float)ctss_rand() * INV_RAND_MAX2 - 1.0f;
}

static inline float ctss_stepf(const float x, const float edge, const float y1,
                      const float y2) {
    return (x < edge ? y1 : y2);
}

static inline float ctss_maddf(const float a, const float b, const float c) {
    return a * b + c;
}

static inline float ctss_mixf(const float a, const float b, const float t) {
    return a + (b - a) * t;
}

#if defined(__ARM_FEATURE_SAT)
static inline int16_t ctss_clamp16(const int32_t x) {
    uint32_t res;
    __asm("ssat %0, #16, %1" : "=r"(res) : "r"(x));
    return res;
}
#else
static inline int16_t ctss_clamp16(const int32_t x) {
    return (int16_t)((x < -0x7fff) ? -0x8000 : (x > 0x7fff ? 0x7fff : x));
}
#endif

// Approximates cos(pi*x) for x in [-1,1]
static inline float ctss_norm_cos(const float x) {
    const float x2 = x * x;
    return 1.0f + x2 * (-4.0f + 2.0f * x2);
}

static inline float ctss_fast_cos_impl(const float x) {
    const float x2 = x * x;
    return 0.99940307f + x2 * (-0.49558072f + 0.03679168f * x2);
}

static inline float ctss_fast_cos(float x) {
    x = fmodf(x, TAU);
    if (x < 0) {
        x = -x;
    }
    switch ((uint8_t)(x * INV_HALF_PI)) {
    case 0:
        return ctss_fast_cos_impl(x);
    case 1:
        return -ctss_fast_cos_impl(PI - x);
    case 2:
        return -ctss_fast_cos_impl(x - PI);
    default:
        return ctss_fast_cos_impl(TAU - x);
    }
}

static inline float ctss_fast_sin(const float x) {
    return ctss_fast_cos(HALF_PI - x);
}

// http://www.kvraudio.com/forum/viewtopic.php?t=375517

static inline float ctss_poly_blep(float t, const float dt) {
    // 0 <= t < 1
    if (t < dt) {
        t /= dt;
        // 2 * (t - t^2/2 - 0.5)
        return t + t - t * t - 1.f;
    }

    // -1 < t < 0
    else if (t > 1.f - dt) {
        t = (t - 1.0f) / dt;
        // 2 * (t^2/2 + t + 0.5)
        return t * t + t + t + 1.;
    }
    // 0 otherwise
    else {
        return 0.0f;
    }
}
