#pragma once

namespace std {
    template<typename T>
    T min(T a, T b) {
        return a < b ? a : b;
    }

    template<typename T>
    T max(T a, T b) {
        return a > b ? a : b;
    }

    template<typename T>
    T abs(T a) {
        return a >= 0 ? a : -a;
    }

    template<typename T>
    T clamp(T a, T minimum, T maximum) {
        if (a < minimum) return minimum;
        if (a > maximum) return maximum;
        return a;
    }

    template<typename T, typename U>
    T lerp(T a, T b, U t) {
        return a + (b - a) * t;
    }

    template<typename T>
    T pow(T a, const int t) {
        if (t <= 0) return 1; // no x^-1 yet
        T ret = a;
        for (int i = 1; i < t; i++) {
            ret *= a;
        }
        return ret;
    }

    inline int floor(float a) {
        int i = static_cast<int>(a);
        if (a < 0 && a != static_cast<float>(i))
            return i - 1;
        return i;
    }

    inline float fract(const float a) {
        return a - floor(a);
    }

    inline int ceiling(float a) {
        int i = static_cast<int>(a);
        if (a > i)
            return i + 1;
        return i;
    }
}
