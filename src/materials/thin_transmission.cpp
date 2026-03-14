#include "materials/thin_transmission.h"

#include <algorithm>
#include <cmath>

#include "core/hit.h"
#include "render/constants.h"
#include "render/optics.h"
#include "render/rng.h"
#include "render/sampling.h"

ThinTransmission::ThinTransmission(const Vec3& transmittance_tint,
                                   double index_of_refraction,
                                   double surface_roughness)
    : tint(transmittance_tint),
      ior(std::max(1.01, index_of_refraction)),
      roughness(std::max(0.0, std::min(1.0, surface_roughness))) {}

Vec3 ThinTransmission::aov_albedo(const HitRecord& rec) const {
    (void)rec;
    return tint;
}

bool ThinTransmission::scatter(const Ray& ray_in,
                               const HitRecord& rec,
                               Vec3& attenuation,
                               Ray& scattered) const
{
    Vec3 n = rec.normal.normalize();
    Vec3 wi = ray_in.direction.normalize();
    double eta = rec.front_face ? (1.0 / ior) : ior;

    double cos_theta = std::min((-1.0 * wi).dot(n), 1.0);
    double sin_theta = std::sqrt(std::max(0.0, 1.0 - cos_theta * cos_theta));
    bool cannot_refract = eta * sin_theta > 1.0;
    bool reflect_event = cannot_refract ||
                         (schlick_reflectance(cos_theta, eta) > random_double());

    Vec3 dir;
    if (reflect_event) {
        dir = reflect(wi, n);
        attenuation = Vec3{1.0, 1.0, 1.0};
    } else {
        dir = refract(wi, n, eta);
        attenuation = tint;
    }

    // Optional micro-roughness broadening.
    if (roughness > 1e-4) {
        Vec3 jitter = local_to_world(random_cosine_direction(), n);
        dir = (dir * (1.0 - roughness) + jitter * roughness).normalize();
    }

    Vec3 offset_normal = dir.dot(n) > 0.0 ? n : (-1.0 * n);
    scattered.origin = rec.point + offset_normal * kRayEpsilon;
    scattered.direction = dir;
    return true;
}

