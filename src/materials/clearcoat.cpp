#include "materials/clearcoat.h"

#include <algorithm>
#include <cmath>

#include "core/hit.h"
#include "render/constants.h"
#include "render/optics.h"
#include "render/rng.h"
#include "render/sampling.h"

Clearcoat::Clearcoat(const Material* base,
                     double ior,
                     double roughness)
    : base_material(base),
      coat_ior(std::max(1.01, ior)),
      coat_roughness(std::max(0.01, std::min(1.0, roughness))) {}

Vec3 Clearcoat::aov_albedo(const HitRecord& rec) const {
    if (base_material)
        return base_material->aov_albedo(rec);
    return {1, 1, 1};
}

bool Clearcoat::scatter(const Ray& ray_in,
                        const HitRecord& rec,
                        Vec3& attenuation,
                        Ray& scattered) const
{
    if (!base_material)
        return false;

    Vec3 n = rec.normal.normalize();
    Vec3 wi = ray_in.direction.normalize();
    Vec3 v = (-1.0 * wi).normalize();
    double cos_theta = std::max(0.0, n.dot(v));
    Vec3 f = fresnel_schlick(cos_theta, Vec3{0.04, 0.04, 0.04});
    double f_luma = 0.2126 * f.x + 0.7152 * f.y + 0.0722 * f.z;
    double p_coat = std::max(0.05, std::min(0.95, f_luma));

    if (random_double() < p_coat) {
        constexpr double kPi = 3.14159265358979323846;
        double u1 = random_double();
        double u2 = random_double();
        double alpha = coat_roughness * coat_roughness;
        double phi = 2.0 * kPi * u1;
        double tan2_theta = (alpha * alpha * u2) / std::max(1e-8, (1.0 - u2));
        double cos_h = 1.0 / std::sqrt(1.0 + tan2_theta);
        double sin_h = std::sqrt(std::max(0.0, 1.0 - cos_h * cos_h));

        Vec3 h_local = {
            std::cos(phi) * sin_h,
            std::sin(phi) * sin_h,
            cos_h
        };
        Vec3 h = local_to_world(h_local, n).normalize();
        if (h.dot(n) < 0.0)
            h = -1.0 * h;

        Vec3 wo = reflect(wi, h).normalize();
        if (wo.dot(n) <= 0.0)
            return false;

        scattered.origin = rec.point + n * kRayEpsilon;
        scattered.direction = wo;
        attenuation = f * (1.0 / p_coat);
        return true;
    }

    if (!base_material->scatter(ray_in, rec, attenuation, scattered))
        return false;

    attenuation = attenuation * (Vec3{1, 1, 1} - f) * (1.0 / (1.0 - p_coat));
    return true;
}

