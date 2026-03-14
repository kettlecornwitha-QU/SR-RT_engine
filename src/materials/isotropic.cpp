#include "materials/isotropic.h"

#include <cmath>

#include "core/hit.h"
#include "render/constants.h"
#include "render/rng.h"

namespace {

Vec3 random_in_unit_sphere() {
    while (true) {
        Vec3 p = {
            random_double() * 2.0 - 1.0,
            random_double() * 2.0 - 1.0,
            random_double() * 2.0 - 1.0
        };
        if (p.norm_squared() >= 1.0)
            continue;
        return p;
    }
}

} // namespace

Isotropic::Isotropic(const Vec3& a) : albedo(a) {}

Vec3 Isotropic::aov_albedo(const HitRecord& rec) const {
    (void)rec;
    return albedo;
}

bool Isotropic::scatter(const Ray& ray_in,
                        const HitRecord& rec,
                        Vec3& attenuation,
                        Ray& scattered) const
{
    (void)ray_in;
    Vec3 dir = random_in_unit_sphere();
    if (dir.norm_squared() <= 1e-12)
        dir = Vec3{1.0, 0.0, 0.0};

    scattered.origin = rec.point + dir.normalize() * kRayEpsilon;
    scattered.direction = dir.normalize();
    attenuation = albedo;
    return true;
}
