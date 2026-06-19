#pragma once

namespace glm {
    struct vec2 {
        union {
            struct { float x, y; };
            struct { float r, g; };
        };

        vec2() = default;
        vec2(const float x, const float y) : x(x), y(y) {}
        explicit vec2(const float a) : x(a), y(a) {}

        vec2& operator+=(const vec2& other) {
            x += other.x;
            y += other.y;
            return *this;
        }
        vec2& operator-=(const vec2& other) {
            x -= other.x;
            y -= other.y;
            return *this;
        }
        vec2& operator*=(const vec2& other) {
            x *= other.x;
            y *= other.y;
            return *this;
        }
        vec2& operator/=(const vec2& other) {
            x /= other.x;
            y /= other.y;
            return *this;
        }
        friend vec2 operator+(const vec2& l, const vec2& r) {
            vec2 result = l;
            result += r;
            return result;
        }
        friend vec2 operator-(const vec2& l, const vec2& r) {
            vec2 result = l;
            result -= r;
            return result;
        }
        friend vec2 operator*(const vec2& l, const vec2& r) {
            vec2 result = l;
            result *= r;
            return result;
        }
        friend vec2 operator/(const vec2& l, const vec2& r) {
            vec2 result = l;
            result /= r;
            return result;
        }
        friend bool operator==(const vec2& l, const vec2& r) {
            return (l.x == r.x) && (l.y == r.y);
        }
        float& operator[](const int index) {
            return (&x)[index];
        }
        const float &operator[](const int index) const {
            return (&x)[index];
        }
    };

    struct ivec2 {
        union {
            struct { int x, y; };
            struct { int r, g; };
        };

        ivec2() = default;
        ivec2(const int x, const int y) : x(x), y(y) {}
        explicit ivec2(const int a) : x(a), y(a) {}

        ivec2& operator+=(const ivec2& other) {
            x += other.x;
            y += other.y;
            return *this;
        }
        ivec2& operator-=(const ivec2& other) {
            x -= other.x;
            y -= other.y;
            return *this;
        }
        ivec2& operator*=(const ivec2& other) {
            x *= other.x;
            y *= other.y;
            return *this;
        }
        ivec2& operator/=(const ivec2& other) {
            x /= other.x;
            y /= other.y;
            return *this;
        }
        friend ivec2 operator+(const ivec2& l, const ivec2& r) {
            ivec2 result = l;
            result += r;
            return result;
        }
        friend ivec2 operator-(const ivec2& l, const ivec2& r) {
            ivec2 result = l;
            result -= r;
            return result;
        }
        friend ivec2 operator*(const ivec2& l, const ivec2& r) {
            ivec2 result = l;
            result *= r;
            return result;
        }
        friend ivec2 operator/(const ivec2& l, const ivec2& r) {
            ivec2 result = l;
            result /= r;
            return result;
        }
        friend bool operator==(const ivec2& l, const ivec2& r) {
            return (l.x == r.x) && (l.y == r.y);
        }
        int& operator[](const int index) {
            return (&x)[index];
        }
        const int &operator[](const int index) const {
            return (&x)[index];
        }
    };

    struct vec3 {
        union {
            struct { float x, y, z; };
            struct { float r, g, b; };
        };

        vec3() = default;
        vec3(const float x, const float y, const float z) : x(x), y(y), z(z) {}
        // vec2 float
        vec3(const float x, const vec2 v2) : x(x), y(v2.x), z(v2.y) {}
        vec3(const vec2 v2, const float z) : x(v2.x), y(v2.y), z(z) {}

        explicit vec3(const float a) : x(a), y(a), z(a) {}

        vec2 get_v2() const {
            return vec2(x, y);
        }

        vec3& operator+=(const vec3& other) {
            x += other.x;
            y += other.y;
            z += other.z;
            return *this;
        }
        vec3& operator-=(const vec3& other) {
            x -= other.x;
            y -= other.y;
            z -= other.z;
            return *this;
        }
        vec3& operator*=(const vec3& other) {
            x *= other.x;
            y *= other.y;
            z *= other.z;
            return *this;
        }
        vec3& operator/=(const vec3& other) {
            x /= other.x;
            y /= other.y;
            z /= other.z;
            return *this;
        }
        friend vec3 operator+(const vec3& l, const vec3& r) {
            vec3 result = l;
            result += r;
            return result;
        }
        friend vec3 operator-(const vec3& l, const vec3& r) {
            vec3 result = l;
            result -= r;
            return result;
        }
        friend vec3 operator*(const vec3& l, const vec3& r) {
            vec3 result = l;
            result *= r;
            return result;
        }
        friend vec3 operator/(const vec3& l, const vec3& r) {
            vec3 result = l;
            result /= r;
            return result;
        }
        friend bool operator==(const vec3& l, const vec3& r) {
            return (l.x == r.x) && (l.y == r.y) && (l.z == r.z);
        }
        float& operator[](const int index) {
            return (&x)[index];
        }
        const float &operator[](const int index) const {
            return (&x)[index];
        }
    };

    struct vec4 {
        union {
            struct { float x, y, z, w; };
            struct { float r, g, b, a; };
        };

        vec4() = default;
        vec4(const float x, const float y, const float z, const float w) : x(x), y(y), z(z), w(w) {}
        // vec3 float
        vec4(const float x, const vec3 v3) : x(x), y(v3.x), z(v3.y), w(v3.z) {}
        vec4(const vec3 v3, const float w) : x(v3.x), y(v3.y), z(v3.z), w(w) {}
        // vec2 float
        vec4(const float x, const float y, const vec2 v2) : x(x), y(y), z(v2.x), w(v2.y) {}
        vec4(const vec2 v2, const float z, const float w) : x(v2.x), y(v2.y), z(z), w(w) {}
        vec4(const float x, const vec2 v2, const float w) : x(x), y(v2.x), z(v2.y), w(w) {}
        // vec2 vec2
        vec4(const vec2 v2_1, const vec2 v2_2) : x(v2_1.x), y(v2_1.y), z(v2_2.x), w(v2_2.y) {}
        explicit vec4(const float a) : x(a), y(a), z(a), w(a) {}

        vec2 get_v2() const {
            return vec2(x, y);
        }

        vec3 get_v3() const {
            return vec3(x,y,z);
        }

        vec4& operator+=(const vec4& other) {
            x += other.x;
            y += other.y;
            z += other.z;
            w += other.w;
            return *this;
        }
        vec4& operator-=(const vec4& other) {
            x -= other.x;
            y -= other.y;
            z -= other.z;
            w -= other.w;
            return *this;
        }
        vec4& operator*=(const vec4& other) {
            x *= other.x;
            y *= other.y;
            z *= other.z;
            w *= other.w;
            return *this;
        }
        vec4& operator/=(const vec4& other) {
            x /= other.x;
            y /= other.y;
            z /= other.z;
            w /= other.w;
            return *this;
        }
        friend vec4 operator+(const vec4& l, const vec4& r) {
            vec4 result = l;
            result += r;
            return result;
        }
        friend vec4 operator-(const vec4& l, const vec4& r) {
            vec4 result = l;
            result -= r;
            return result;
        }
        friend vec4 operator*(const vec4& l, const vec4& r) {
            vec4 result = l;
            result *= r;
            return result;
        }
        friend vec4 operator/(const vec4& l, const vec4& r) {
            vec4 result = l;
            result /= r;
            return result;
        }
        friend bool operator==(const vec4& l, const vec4& r) {
            return (l.x == r.x) && (l.y == r.y) && (l.z == r.z) && (l.w == r.w);
        }
        float& operator[](const int index) {
            return (&x)[index];
        }
        const float &operator[](const int index) const {
            return (&x)[index];
        }
    };
}