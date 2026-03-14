#include "materials/conductor_metal.h"

#include <algorithm>
#include <cmath>

#include "core/hit.h"
#include "render/constants.h"
#include "render/optics.h"
#include "render/rng.h"
#include "render/sampling.h"

namespace {

constexpr double kPi = 3.14159265358979323846;

Vec3 conductor_f0_from_eta_k(const Vec3& eta, const Vec3& k) {
    auto f0c = [](double e, double kk) {
        double em1 = e - 1.0;
        double ep1 = e + 1.0;
        double num = em1 * em1 + kk * kk;
        double den = ep1 * ep1 + kk * kk;
        return num / std::max(1e-8, den);
    };
    return {f0c(eta.x, k.x), f0c(eta.y, k.y), f0c(eta.z, k.z)};
}

} // namespace

ConductorMetal::ConductorMetal(const Vec3& eta_rgb,
                               const Vec3& k_rgb,
                               double r)
    : eta(eta_rgb),
      k(k_rgb),
      roughness(std::max(0.02, std::min(1.0, r))) {}

Vec3 ConductorMetal::aov_albedo(const HitRecord& rec) const {
    (void)rec;
    return conductor_f0_from_eta_k(eta, k);
}

bool ConductorMetal::scatter(const Ray& ray_in,
                             const HitRecord& rec,
                             Vec3& attenuation,
                             Ray& scattered) const
{
    double r1 = random_double();
    double r2 = random_double();
    double alpha = roughness * roughness;
    double phi = 2.0 * kPi * r1;
    double tan2_theta =
        (alpha * alpha * r2) /
        std::max(1e-8, (1.0 - r2));
    double cos_theta = 1.0 / std::sqrt(1.0 + tan2_theta);
    double sin_theta =
        std::sqrt(std::max(0.0, 1.0 - cos_theta * cos_theta));

    Vec3 h_local = {
        std::cos(phi) * sin_theta,
        std::sin(phi) * sin_theta,
        cos_theta
    };
    Vec3 h = local_to_world(h_local, rec.normal).normalize();
    if (h.dot(rec.normal) < 0.0)
        h = -1.0 * h;

    Vec3 wi = ray_in.direction.normalize();
    Vec3 v = (-1.0 * wi).normalize();
    double n_dot_v = std::max(0.0, rec.normal.dot(v));
    if (n_dot_v <= 0.0)
        return false;

    Vec3 wo = reflect(wi, h).normalize();
    double n_dot_l = std::max(0.0, rec.normal.dot(wo));
    if (n_dot_l <= 0.0)
        return false;

    double n_dot_h = std::max(0.0, rec.normal.dot(h));
    double v_dot_h = std::max(0.0, v.dot(h));
    if (n_dot_h <= 0.0 || v_dot_h <= 0.0)
        return false;

    double g =
        smith_ggx_g1(n_dot_v, roughness) *
        smith_ggx_g1(n_dot_l, roughness);
    Vec3 fresnel = fresnel_conductor(v_dot_h, eta, k);
    double throughput_weight =
        (g * v_dot_h) /
        std::max(1e-8, n_dot_v * n_dot_h);

    scattered.origin = rec.point + rec.normal * kRayEpsilon;
    scattered.direction = wo;
    attenuation = fresnel * throughput_weight;
    return true;
}

