#include "fixedPoint.hpp"
#include <cstdint>
#include <cmath>

q16_16 to_fixed(double x) {
    return (q16_16) llround(x * SCALE);
}

double to_float(q16_16 x) {
    return (double)x / SCALE;
}

q16_16 qadd(q16_16 a, q16_16 b) { return a + b; }
q16_16 qsub(q16_16 a, q16_16 b) { return a - b; }

q16_16 qmul(q16_16 a, q16_16 b) {
    return (q16_16)(((int64_t)a * (int64_t)b) >> 16);
}

q16_16 qdiv(q16_16 a, q16_16 b) {
    return (q16_16)(((int64_t)a << 16) / b);
}
