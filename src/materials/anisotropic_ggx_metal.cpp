#include "materials/anisotropic_ggx_metal.h"

#include <algorithm>
#include <cmath>

#include "core/hit.h"
#include "render/constants.h"
#include "render/optics.h"
#include "render/rng.h"

namespace {

constexpr double kPi = 3.14159265358979323846;

Vec3 orthonormal_tangent(const Vec3& n, const Vec3& hint) {
    Vec3 t = hint - n * hint.dot(n);
    if (t.norm_squared() < 1e-10) {
        Vec3 fallback = (std::fabs(n.x) < 0.9) ? Vec3{1, 0, 0} : Vec3{0, 1, 0};
        t = fallback - n * fallback.dot(n);
    }
    return t.normalize();
}

double smith_ggx_g1_aniso(const Vec3& w,
                          const Vec3& n,
                          const Vec3& t,
                          const Vec3& b,
                          double ax,
                          double ay)
{
    const double n_dot_w = std::max(0.0, n.dot(w));
    if (n_dot_w <= 1e-8)
        return 0.0;

    const double tx = t.dot(w);
    const double by = b.dot(w);
    const double alpha2 = (ax * tx) * (ax * tx) + (ay * by) * (ay * by);
    const double tan2_theta = std::max(0.0, (1.0 - n_dot_w * n_dot_w) / (n_dot_w * n_dot_w));
    const double root = std::sqrt(1.0 + alpha2 * tan2_theta);
    return 2.0 / (1.0 + root);
}

} // namespace

AnisotropicGGXMetal::AnisotropicGGXMetal(const Vec3& base_f0,
                                         double rx,
                                         double ry,
                                         const Vec3& tangent)
    : f0(base_f0),
      roughness_x(std::max(0.02, std::min(1.0, rx))),
      roughness_y(std::max(0.02, std::min(1.0, ry))),
      tangent_hint(tangent.normalize()) {}

Vec3 AnisotropicGGXMetal::aov_albedo(const HitRecord& rec) const {
    (void)rec;
    return f0;
}

bool AnisotropicGGXMetal::scatter(const Ray& ray_in,
                                  const HitRecord& rec,
                                  Vec3& attenuation,
                                  Ray& scattered) const
{
    Vec3 n = rec.normal.normalize();
    Vec3 t = orthonormal_tangent(n, tangent_hint);
    Vec3 b = n.cross(t).normalize();

    const double u1 = random_double();
    const double u2 = random_double();

    double phi = std::atan((roughness_y / roughness_x) * std::tan(2.0 * kPi * u1));
    if (u1 > 0.5)
        phi += kPi;

    double cos_phi = std::cos(phi);
    double sin_phi = std::sin(phi);
    double inv_alpha2 =
        (cos_phi * cos_phi) / (roughness_x * roughness_x) +
        (sin_phi * sin_phi) / (roughness_y * roughness_y);
    double tan2_theta = u2 / std::max(1e-8, (1.0 - u2) * inv_alpha2);
    double cos_theta = 1.0 / std::sqrt(1.0 + tan2_theta);
    double sin_theta = std::sqrt(std::max(0.0, 1.0 - cos_theta * cos_theta));

    Vec3 h = (t * (cos_phi * sin_theta) +
              b * (sin_phi * sin_theta) +
              n * cos_theta).normalize();
    if (h.dot(n) < 0.0)
        h = -1.0 * h;

    Vec3 wi = ray_in.direction.normalize();
    Vec3 v = (-1.0 * wi).normalize();
    double n_dot_v = std::max(0.0, n.dot(v));
    if (n_dot_v <= 0.0)
        return false;

    Vec3 wo = reflect(wi, h).normalize();
    double n_dot_l = std::max(0.0, n.dot(wo));
    if (n_dot_l <= 0.0)
        return false;

    double n_dot_h = std::max(0.0, n.dot(h));
    double v_dot_h = std::max(0.0, v.dot(h));
    if (n_dot_h <= 0.0 || v_dot_h <= 0.0)
        return false;

    double g =
        smith_ggx_g1_aniso(v, n, t, b, roughness_x, roughness_y) *
        smith_ggx_g1_aniso(wo, n, t, b, roughness_x, roughness_y);
    Vec3 fresnel = fresnel_schlick(v_dot_h, f0);
    double throughput_weight =
        (g * v_dot_h) /
        std::max(1e-8, n_dot_v * n_dot_h);

    scattered.origin = rec.point + n * kRayEpsilon;
    scattered.direction = wo;
    attenuation = fresnel * throughput_weight;
    return true;
}
