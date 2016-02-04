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

#define INV_RAND_MAX2 (float)(2.0 / RAND_MAX)

#if defined(__ARM_ARCH)
//#include "tinymt32.h"

// tinymt32_t ct_math_rnd;

void ct_math_init();
float ct_randf(const float min, const float max);
float ct_normrandf();

#else
inline float ct_randf(const float min, const float max) {
    return min + (float)rand() / RAND_MAX * (max - min);
}

inline float ct_normrandf() {
    return (float)rand() * INV_RAND_MAX2 - 1.0f;
}
#endif

inline float ct_stepf(const float x, const float edge, const float y1,
                      const float y2) {
    return (x < edge ? y1 : y2);
}

inline float ct_maddf(const float a, const float b, const float c) {
    return a * b + c;
}

inline float ct_mixf(const float a, const float b, const float t) {
    return a + (b - a) * t;
}

#if defined(__ARM_FEATURE_SAT)
inline int16_t ct_clamp16(const int32_t x) {
    uint32_t res;
    __asm("ssat %0, #16, %1" : "=r"(res) : "r"(x));
    return res;
}
#else
inline int16_t ct_clamp16(const int32_t x) {
    return (int16_t)((x < -0x7fff) ? -0x8000 : (x > 0x7fff ? 0x7fff : x));
}
#endif

// Approximates cos(pi*x) for x in [-1,1]
inline float ct_norm_cos(const float x) {
    const float x2 = x * x;
    return 1.0f + x2 * (-4.0f + 2.0f * x2);
}

inline float ct_fast_cos_impl(const float x) {
    const float x2 = x * x;
    return 0.99940307f + x2 * (-0.49558072f + 0.03679168f * x2);
}

inline float ct_fast_cos(float x) {
    x = fmodf(x, TAU);
    if (x < 0) {
        x = -x;
    }
    switch ((uint8_t)(x * INV_HALF_PI)) {
    case 0:
        return ct_fast_cos_impl(x);
    case 1:
        return -ct_fast_cos_impl(PI - x);
    case 2:
        return -ct_fast_cos_impl(x - PI);
    default:
        return ct_fast_cos_impl(TAU - x);
    }
}

inline float ct_fast_sin(const float x) {
    return ct_fast_cos(HALF_PI - x);
}

// http://www.kvraudio.com/forum/viewtopic.php?t=375517

inline float ct_poly_blep(float t, const float dt) {
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
