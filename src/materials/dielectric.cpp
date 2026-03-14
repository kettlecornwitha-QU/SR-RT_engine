#include "materials/dielectric.h"
#include "core/hit.h"

#include <algorithm>
#include <cmath>

#include "render/constants.h"
#include "render/optics.h"
#include "render/rng.h"

Dielectric::Dielectric(double index_of_refraction)
    : ior(index_of_refraction) {}

Vec3 Dielectric::aov_albedo(const HitRecord& rec) const {
    (void)rec;
    return {1, 1, 1};
}

bool Dielectric::scatter(const Ray& ray_in,
                         const HitRecord& rec,
                         Vec3& attenuation,
                         Ray& scattered) const
{
    attenuation = {1, 1, 1};

    double refraction_ratio =
        rec.front_face ? (1.0 / ior) : ior;

    Vec3 unit_direction = ray_in.direction.normalize();
    double cos_theta =
        std::min((-1.0 * unit_direction).dot(rec.normal), 1.0);
    double sin_theta =
        std::sqrt(std::max(0.0, 1.0 - cos_theta * cos_theta));

    bool cannot_refract = refraction_ratio * sin_theta > 1.0;
    bool must_reflect =
        schlick_reflectance(cos_theta, refraction_ratio) > random_double();

    Vec3 direction;
    if (cannot_refract || must_reflect) {
        direction = reflect(unit_direction, rec.normal);
    } else {
        direction = refract(unit_direction,
                            rec.normal,
                            refraction_ratio);
    }

    Vec3 offset_normal =
        direction.dot(rec.normal) > 0.0 ?
        rec.normal : (-1.0 * rec.normal);
    scattered.origin = rec.point + offset_normal * kRayEpsilon;
    scattered.direction = direction.normalize();
    return true;
}
