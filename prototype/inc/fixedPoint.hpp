#pragma once
#include <cstdint>
#include <cmath>

typedef int32_t q16_16;
static const int SCALE = 1 << 16;

q16_16 to_fixed(double x);
double to_float(q16_16 x);

q16_16 qadd(q16_16 a, q16_16 b);
q16_16 qsub(q16_16 a, q16_16 b);
q16_16 qmul(q16_16 a, q16_16 b);
q16_16 qdiv(q16_16 a, q16_16 b);
