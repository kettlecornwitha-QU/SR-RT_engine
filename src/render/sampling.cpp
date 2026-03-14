#include "render/sampling.h"

#include <cmath>

#include "render/rng.h"

Vec3 random_in_unit_disk() {
    while (true) {
        Vec3 p = {
            random_double() * 2.0 - 1.0,
            random_double() * 2.0 - 1.0,
            0.0
        };
        if (p.norm_squared() >= 1.0)
            continue;
        return p;
    }
}

Vec3 random_cosine_direction() {
    constexpr double kPi = 3.14159265358979323846;
    double r1 = random_double();
    double r2 = random_double();
    double phi = 2.0 * kPi * r1;

    double x = std::cos(phi) * std::sqrt(r2);
    double y = std::sin(phi) * std::sqrt(r2);
    double z = std::sqrt(1.0 - r2);

    return {x, y, z};
}

Vec3 local_to_world(const Vec3& local, const Vec3& normal) {
    Vec3 w = normal.normalize();
    Vec3 a = (std::fabs(w.x) > 0.9) ? Vec3{0, 1, 0} : Vec3{1, 0, 0};
    Vec3 v = w.cross(a).normalize();
    Vec3 u = v.cross(w);

    return u * local.x + v * local.y + w * local.z;
}
