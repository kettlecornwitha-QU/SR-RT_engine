#pragma once

#include <algorithm>
#include <cmath>

struct Vec3 {
    double x, y, z;

    Vec3 operator+(const Vec3& v) const { return {x+v.x, y+v.y, z+v.z}; }
    Vec3 operator-(const Vec3& v) const { return {x-v.x, y-v.y, z-v.z}; }
    Vec3 operator*(double s) const { return {x*s, y*s, z*s}; }
    Vec3 operator/(double s) const { return {x/s, y/s, z/s}; }
    Vec3 operator*(const Vec3& v) const {
        return {x * v.x, y * v.y, z * v.z};
    }

    Vec3 cross(const Vec3& v) const {
        return {
            y*v.z - z*v.y,
            z*v.x - x*v.z,
            x*v.y - y*v.x
        };
    }

    double dot(const Vec3& v) const {
        return x*v.x + y*v.y + z*v.z;
    }

    double norm_squared() const { return dot(*this); }
    double norm() const { return std::sqrt(norm_squared()); }

    Vec3 normalize() const {
        double n = norm();
        if (n == 0) return {0,0,0};
        return *this / n;
    }
};

inline Vec3 operator*(double s, const Vec3& v) {
    return v * s;
}

inline Vec3 min_vec3(const Vec3& a, const Vec3& b) {
    return {
        std::min(a.x, b.x),
        std::min(a.y, b.y),
        std::min(a.z, b.z)
    };
}

inline Vec3 max_vec3(const Vec3& a, const Vec3& b) {
    return {
        std::max(a.x, b.x),
        std::max(a.y, b.y),
        std::max(a.z, b.z)
    };
}
