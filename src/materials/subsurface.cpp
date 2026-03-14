#include "materials/subsurface.h"

#include <algorithm>
#include <cmath>

#include "core/hit.h"
#include "render/constants.h"
#include "render/rng.h"
#include "render/sampling.h"

namespace {

constexpr double kPi = 3.14159265358979323846;

double saturate(double v) {
    return std::max(0.0, std::min(1.0, v));
}

Vec3 clamp01(const Vec3& c) {
    return {saturate(c.x), saturate(c.y), saturate(c.z)};
}

Vec3 beer_lambert(const Vec3& sigma_a, double d) {
    return {
        std::exp(-sigma_a.x * d),
        std::exp(-sigma_a.y * d),
        std::exp(-sigma_a.z * d)
    };
}

double average_component(const Vec3& v) {
    return (v.x + v.y + v.z) / 3.0;
}

Vec3 orthonormal_tangent(const Vec3& n) {
    Vec3 axis = (std::fabs(n.x) > 0.5) ? Vec3{0.0, 1.0, 0.0} : Vec3{1.0, 0.0, 0.0};
    Vec3 t = axis.cross(n).normalize();
    if (t.norm_squared() <= 1e-12)
        return Vec3{1.0, 0.0, 0.0};
    return t;
}

Vec3 sample_henyey_greenstein(const Vec3& wo, double g) {
    const double u1 = random_double();
    const double u2 = random_double();

    double cos_theta = 0.0;
    if (std::fabs(g) < 1e-4) {
        cos_theta = 1.0 - 2.0 * u1;
    } else {
        const double sq =
            (1.0 - g * g) /
            std::max(1e-8, 1.0 + g - 2.0 * g * u1);
        cos_theta = (1.0 + g * g - sq * sq) / (2.0 * g);
        cos_theta = std::max(-1.0, std::min(1.0, cos_theta));
    }
    const double sin_theta = std::sqrt(std::max(0.0, 1.0 - cos_theta * cos_theta));
    const double phi = 2.0 * kPi * u2;

    const Vec3 w = wo.normalize();
    const Vec3 u = orthonormal_tangent(w);
    const Vec3 v = w.cross(u).normalize();
    return (u * (std::cos(phi) * sin_theta) +
            v * (std::sin(phi) * sin_theta) +
            w * cos_theta).normalize();
}

} // namespace

Subsurface::Subsurface(const Vec3& a,
                       double mfp,
                       double mix,
                       double bias,
                       bool random_walk)
    : albedo(clamp01(a)),
      mean_free_path(std::max(1e-3, mfp)),
      subsurface_mix(saturate(mix)),
      forward_bias(saturate(bias)),
      random_walk_mode(random_walk) {}

Vec3 Subsurface::aov_albedo(const HitRecord& rec) const {
    (void)rec;
    return albedo;
}

bool Subsurface::scatter(const Ray& ray_in,
                         const HitRecord& rec,
                         Vec3& attenuation,
                         Ray& scattered) const
{
    Vec3 n = rec.normal.normalize();
    Vec3 wi = ray_in.direction.normalize();
    double cos_in = std::max(0.0, (-1.0 * wi).dot(n));

    if (random_double() < subsurface_mix) {
        Vec3 sigma_a = {
            (1.0 - albedo.x) / mean_free_path,
            (1.0 - albedo.y) / mean_free_path,
            (1.0 - albedo.z) / mean_free_path
        };

        if (random_walk_mode) {
            // Slower random-walk approximation with HG phase sampling.
            Vec3 walk_dir = (n * (1.0 - forward_bias) + (-1.0 * wi) * forward_bias).normalize();
            if (walk_dir.norm_squared() <= 1e-12)
                walk_dir = n;

            Vec3 throughput = {1.0, 1.0, 1.0};
            const double sigma_t = 1.0 / std::max(1e-4, mean_free_path);
            const double g = -0.05 + 0.90 * forward_bias;
            const int step_count = 24 + static_cast<int>(random_double() * 41.0); // [24,64]

            for (int i = 0; i < step_count; ++i) {
                const double u = std::max(1e-8, random_double());
                const double step_d = -std::log(1.0 - u) / std::max(1e-6, sigma_t);
                throughput = throughput * beer_lambert(sigma_a, step_d);
                walk_dir = sample_henyey_greenstein(walk_dir, g);

                if (i >= 3) {
                    const double survive =
                        std::max(0.25, std::min(0.98, average_component(throughput)));
                    if (random_double() > survive)
                        break;
                    throughput = throughput * (1.0 / survive);
                }
            }

            Vec3 local_dir = random_cosine_direction();
            Vec3 hemi = local_to_world(local_dir, n).normalize();
            Vec3 dir = (hemi * (1.0 - forward_bias) + walk_dir * forward_bias).normalize();
            if (dir.dot(n) <= 1e-6)
                dir = (dir + n * (1.0 - dir.dot(n))).normalize();

            scattered.origin = rec.point + n * kRayEpsilon;
            scattered.direction = dir;
            attenuation = (albedo * throughput) * (1.0 / std::max(1e-6, subsurface_mix));
            return true;
        } else {
            // Fast SSS approximation for interactive iteration.
            Vec3 view_dir = (-1.0 * wi).normalize();
            Vec3 axis = (n * (1.0 - forward_bias) + view_dir * forward_bias).normalize();
            if (axis.norm_squared() <= 1e-12)
                axis = n;

            Vec3 local_dir = random_cosine_direction();
            Vec3 dir = local_to_world(local_dir, axis).normalize();
            if (dir.dot(n) <= 1e-6)
                dir = (dir + n * (1.0 - dir.dot(n))).normalize();

            const double d = 0.30 / std::max(0.08, cos_in);
            Vec3 transmittance = beer_lambert(sigma_a, d);

            scattered.origin = rec.point + n * kRayEpsilon;
            scattered.direction = dir;
            attenuation = (albedo * transmittance) * (1.0 / std::max(1e-6, subsurface_mix));
            return true;
        }
    }

    Vec3 local_dir = random_cosine_direction();
    Vec3 dir = local_to_world(local_dir, n).normalize();
    scattered.origin = rec.point + n * kRayEpsilon;
    scattered.direction = dir;
    attenuation = albedo * (1.0 / std::max(1e-6, 1.0 - subsurface_mix));
    return true;
}
