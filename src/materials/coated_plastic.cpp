#include "materials/coated_plastic.h"

#include <algorithm>
#include <cmath>

#include "core/hit.h"
#include "render/constants.h"
#include "render/optics.h"
#include "render/rng.h"
#include "render/sampling.h"

namespace {

constexpr double kPi = 3.14159265358979323846;

double luminance(const Vec3& c) {
    return 0.2126 * c.x + 0.7152 * c.y + 0.0722 * c.z;
}

} // namespace

CoatedPlastic::CoatedPlastic(const Vec3& base,
                             double ior,
                             double roughness,
                             double strength)
    : base_color(base),
      coat_ior(std::max(1.01, ior)),
      coat_roughness(std::max(0.01, std::min(1.0, roughness))),
      coat_strength(std::max(0.0, std::min(1.0, strength))) {}

Vec3 CoatedPlastic::aov_albedo(const HitRecord& rec) const {
    (void)rec;
    return base_color;
}

bool CoatedPlastic::scatter(const Ray& ray_in,
                            const HitRecord& rec,
                            Vec3& attenuation,
                            Ray& scattered) const
{
    Vec3 n = rec.normal.normalize();
    Vec3 wi = ray_in.direction.normalize();
    Vec3 v = (-1.0 * wi).normalize();
    double cos_theta = std::max(0.0, n.dot(v));

    // Dielectric clearcoat Fresnel (F0 derived from IOR).
    const double f0_scalar = std::pow((coat_ior - 1.0) / (coat_ior + 1.0), 2.0);
    Vec3 coat_fresnel = fresnel_schlick(cos_theta, Vec3{f0_scalar, f0_scalar, f0_scalar}) * coat_strength;
    double coat_fresnel_luma = std::max(0.0, std::min(1.0, luminance(coat_fresnel)));

    const double p_coat = std::max(0.02, std::min(0.98, coat_fresnel_luma));
    if (random_double() < p_coat) {
        // GGX microfacet reflection lobe for the coat.
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
        attenuation = coat_fresnel * (1.0 / p_coat);
        return true;
    }

    // Base diffuse event, energy-weighted by the coat transmittance.
    Vec3 local_dir = random_cosine_direction();
    Vec3 wo = local_to_world(local_dir, n).normalize();
    scattered.origin = rec.point + n * kRayEpsilon;
    scattered.direction = wo;

    Vec3 base_weight = (Vec3{1.0, 1.0, 1.0} - coat_fresnel);
    attenuation = base_color * base_weight * (1.0 / (1.0 - p_coat));
    return true;
}

