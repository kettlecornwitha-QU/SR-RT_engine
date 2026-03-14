#include "materials/sheen.h"

#include <algorithm>
#include <cmath>

#include "core/hit.h"
#include "render/constants.h"
#include "render/sampling.h"

namespace {

double clamp01(double x) {
    return std::max(0.0, std::min(1.0, x));
}

} // namespace

Sheen::Sheen(const Vec3& base,
             const Vec3& sheen,
             double weight)
    : base_color(base),
      sheen_color(sheen),
      sheen_weight(clamp01(weight)) {}

Vec3 Sheen::aov_albedo(const HitRecord& rec) const {
    (void)rec;
    return base_color;
}

bool Sheen::scatter(const Ray& ray_in,
                    const HitRecord& rec,
                    Vec3& attenuation,
                    Ray& scattered) const
{
    Vec3 local_dir = random_cosine_direction();
    Vec3 wo = local_to_world(local_dir, rec.normal).normalize();

    Vec3 v = (-1.0 * ray_in.direction).normalize();
    double ndotv = std::max(0.0, rec.normal.dot(v));
    double grazing = std::pow(1.0 - ndotv, 5.0);
    Vec3 sheen_term = sheen_color * (sheen_weight * grazing);

    attenuation = base_color * (1.0 - sheen_weight) + sheen_term;
    attenuation = {
        clamp01(attenuation.x),
        clamp01(attenuation.y),
        clamp01(attenuation.z)
    };

    scattered.origin = rec.point + rec.normal * kRayEpsilon;
    scattered.direction = wo;
    return true;
}

