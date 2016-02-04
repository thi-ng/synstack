#include "ct_math.h"

#if defined(__ARM_ARCH)
void ct_math_init() {
    // tinymt32_init(&ct_math_rnd, 0xdecafbad);
}

float ct_randf(const float min, const float max) {
    return min; // + tinymt32_generate_float(&ct_math_rnd) * (max - min);
}

float ct_normrandf() {
    return 0; // tinymt32_generate_float(&ct_math_rnd) * 2.0f - 1.0f;
}
#endif
