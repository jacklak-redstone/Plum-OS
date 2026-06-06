#include "trigonometry.hpp"

namespace std {
    float sin(float x) {
        while (x >= TWO_PI) x -= TWO_PI;
        while (x < 0) x += TWO_PI;

        const float idx = x * STEP;

        const int i0 = static_cast<int>(idx) & 1023;
        const int i1 = (i0+1) & 1023;
        const float frac = idx - i0;

        const float a = sinTable[i0];
        const float b = sinTable[i1];

        return a + (b - a) * frac;
    }
}
