#include "materials/checker_lambertian.h"

#include <cmath>

#include "core/hit.h"
#include "render/constants.h"
#include "render/sampling.h"

CheckerLambertian::CheckerLambertian(const Vec3& a,
                                     const Vec3& b,
                                     double su,
                                     double sv)
    : Lambertian(a),
      color_a(a),
      color_b(b),
      scale_u(std::max(1e-6, su)),
      scale_v(std::max(1e-6, sv)) {}

Vec3 CheckerLambertian::aov_albedo(const HitRecord& rec) const {
    double u = std::floor(rec.u * scale_u);
    double v = std::floor(rec.v * scale_v);
    int parity = static_cast<int>(u + v);
    return (parity & 1) ? color_b : color_a;
}

bool CheckerLambertian::scatter(const Ray& ray_in,
                                const HitRecord& rec,
                                Vec3& attenuation,
                                Ray& scattered) const
{
    (void)ray_in;
    Vec3 local_dir = random_cosine_direction();
    Vec3 scatter_direction = local_to_world(local_dir, rec.normal).normalize();

    scattered.origin = rec.point + rec.normal * kRayEpsilon;
    scattered.direction = scatter_direction;
    attenuation = aov_albedo(rec);
    return true;
}

