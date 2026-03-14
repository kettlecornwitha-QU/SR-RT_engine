#include "render/optics.h"

#include <algorithm>
#include <cmath>

Vec3 reflect(const Vec3& v, const Vec3& n) {
    return v - 2.0 * v.dot(n) * n;
}

Vec3 refract(const Vec3& uv,
             const Vec3& n,
             double etai_over_etat)
{
    double cos_theta = std::min((-1.0 * uv).dot(n), 1.0);
    Vec3 r_out_perp = etai_over_etat * (uv + cos_theta * n);
    Vec3 r_out_parallel =
        (-std::sqrt(std::fabs(1.0 - r_out_perp.norm_squared()))) * n;
    return r_out_perp + r_out_parallel;
}

double schlick_reflectance(double cosine, double ref_idx) {
    double r0 = (1.0 - ref_idx) / (1.0 + ref_idx);
    r0 = r0 * r0;
    return r0 + (1.0 - r0) * std::pow(1.0 - cosine, 5.0);
}

Vec3 fresnel_schlick(double cos_theta, const Vec3& f0) {
    double factor = std::pow(1.0 - cos_theta, 5.0);
    return f0 + (Vec3{1, 1, 1} - f0) * factor;
}

Vec3 fresnel_conductor(double cos_theta, const Vec3& eta, const Vec3& k) {
    cos_theta = std::max(0.0, std::min(1.0, cos_theta));
    const double cos2 = cos_theta * cos_theta;

    auto component = [&](double eta_c, double k_c) {
        const double eta2 = eta_c * eta_c;
        const double k2 = k_c * k_c;
        const double t0 = eta2 + k2;
        const double two_eta_cos = 2.0 * eta_c * cos_theta;

        const double rs_num = t0 - two_eta_cos + cos2;
        const double rs_den = t0 + two_eta_cos + cos2;
        const double rs = rs_num / std::max(1e-8, rs_den);

        const double rp_num = t0 * cos2 - two_eta_cos + 1.0;
        const double rp_den = t0 * cos2 + two_eta_cos + 1.0;
        const double rp = rp_num / std::max(1e-8, rp_den);

        return 0.5 * (rs + rp);
    };

    return {
        component(eta.x, k.x),
        component(eta.y, k.y),
        component(eta.z, k.z)
    };
}

double smith_ggx_g1(double n_dot_x, double roughness) {
    double r = roughness + 1.0;
    double k = (r * r) / 8.0;
    return n_dot_x / (n_dot_x * (1.0 - k) + k);
}
