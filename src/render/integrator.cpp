#include "render/integrator.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "materials/lambertian.h"
#include "render/atmosphere.h"
#include "render/constants.h"
#include "render/rng.h"

namespace {

constexpr double kPi = 3.14159265358979323846;

double max_component(const Vec3& v) {
    return std::max(v.x, std::max(v.y, v.z));
}

Vec3 sky_color(const Ray& r)
{
    Vec3 unit = r.direction.normalize();
    double t = 0.5 * (unit.y + 1.0);

    return (1.0 - t) * Vec3{1, 1, 1}
         + t * Vec3{0.5, 0.7, 1.0};
}

double area_light_pdf_for_hit(const Vec3& shading_point,
                              const Vec3& light_point,
                              const std::vector<RectAreaLight>& area_lights)
{
    for (const auto& light : area_lights) {
        Vec3 area_vec = light.edge_u.cross(light.edge_v);
        double area = area_vec.norm();
        if (area <= 1e-10)
            continue;

        Vec3 normal = area_vec / area;
        Vec3 rel = light_point - light.center;

        double u_len = light.edge_u.norm();
        double v_len = light.edge_v.norm();
        if (u_len <= 1e-10 || v_len <= 1e-10)
            continue;

        Vec3 u_axis = light.edge_u / u_len;
        Vec3 v_axis = light.edge_v / v_len;
        double u_proj = rel.dot(u_axis);
        double v_proj = rel.dot(v_axis);
        double plane_dist = std::fabs(rel.dot(normal));

        if (plane_dist > 1e-3)
            continue;
        if (std::fabs(u_proj) > 0.5 * u_len + 1e-3 ||
            std::fabs(v_proj) > 0.5 * v_len + 1e-3)
            continue;

        Vec3 to_light = light_point - shading_point;
        double distance_sq = to_light.norm_squared();
        if (distance_sq <= 1e-10)
            continue;

        Vec3 wi = to_light / std::sqrt(distance_sq);
        double light_cos = normal.dot(-1.0 * wi);
        if (light_cos <= 0.0)
            continue;

        return distance_sq / (light_cos * area);
    }

    return 0.0;
}

Vec3 sample_direct_lighting(const HitRecord& rec,
                            const Hittable& world,
                            const Vec3& diffuse_albedo,
                            const std::vector<PointLight>& point_lights,
                            const std::vector<RectAreaLight>& area_lights,
                            int area_light_samples)
{
    Vec3 direct = {0, 0, 0};

    for (const auto& light : point_lights) {
        Vec3 to_light = light.position - rec.point;
        double distance_sq = to_light.norm_squared();

        if (distance_sq <= 1e-10)
            continue;

        double distance = std::sqrt(distance_sq);
        Vec3 wi = to_light / distance;
        double n_dot_l = rec.normal.dot(wi);

        if (n_dot_l <= 0.0)
            continue;

        Ray shadow_ray;
        shadow_ray.origin = rec.point + rec.normal * kRayEpsilon;
        shadow_ray.direction = wi;

        HitRecord shadow_hit;
        if (world.hit(shadow_ray,
                      kHitTMin,
                      distance - kRayEpsilon,
                      shadow_hit))
            continue;

        Vec3 brdf = diffuse_albedo * (1.0 / kPi);
        Vec3 irradiance = light.intensity * (n_dot_l / distance_sq);
        direct = direct + brdf * irradiance;
    }

    const int area_samples = std::max(1, area_light_samples);

    for (const auto& light : area_lights) {
        Vec3 area_vec = light.edge_u.cross(light.edge_v);
        double area = area_vec.norm();
        if (area <= 1e-10)
            continue;

        Vec3 light_accum = {0, 0, 0};
        Vec3 light_normal = area_vec / area;
        Vec3 brdf = diffuse_albedo * (1.0 / kPi);

        for (int s = 0; s < area_samples; ++s) {
            double su = random_double() - 0.5;
            double sv = random_double() - 0.5;
            Vec3 light_point =
                light.center +
                light.edge_u * su +
                light.edge_v * sv;

            Vec3 to_light = light_point - rec.point;
            double distance_sq = to_light.norm_squared();
            if (distance_sq <= 1e-10)
                continue;

            double distance = std::sqrt(distance_sq);
            Vec3 wi = to_light / distance;
            double n_dot_l = rec.normal.dot(wi);
            if (n_dot_l <= 0.0)
                continue;

            double light_cos = light_normal.dot(-1.0 * wi);
            if (light_cos <= 0.0)
                continue;

            Ray shadow_ray;
            shadow_ray.origin = rec.point + rec.normal * kRayEpsilon;
            shadow_ray.direction = wi;

            HitRecord shadow_hit;
            if (world.hit(shadow_ray,
                          kHitTMin,
                          distance - kRayEpsilon,
                          shadow_hit))
                continue;

            double pdf_light = distance_sq / (light_cos * area);
            double pdf_bsdf = n_dot_l / kPi;
            double mis_weight = pdf_light / (pdf_light + pdf_bsdf);

            light_accum = light_accum +
                          brdf * light.emission *
                          (n_dot_l / pdf_light) *
                          mis_weight;
        }

        direct = direct + light_accum * (1.0 / area_samples);
    }

    return direct;
}

} // namespace

Vec3 trace(const Ray& r,
           const Hittable& world,
           const std::vector<PointLight>& point_lights,
           const std::vector<RectAreaLight>& area_lights,
           const RenderSettings& settings,
           int depth,
           bool prev_was_lambertian,
           const Vec3& prev_point,
           const Vec3& prev_normal)
{
    if (depth <= 0)
        return {0, 0, 0};

    HitRecord rec;

    if (world.hit(r, kHitTMin,
                  std::numeric_limits<double>::infinity(),
                  rec))
    {
        Vec3 emitted = rec.material->emitted(rec);
        if (prev_was_lambertian && max_component(emitted) > 0.0) {
            Vec3 wi = (rec.point - prev_point).normalize();
            double cos_prev = std::max(0.0, prev_normal.dot(wi));
            if (cos_prev > 0.0) {
                double pdf_bsdf = cos_prev / kPi;
                double pdf_light =
                    area_light_pdf_for_hit(prev_point,
                                           rec.point,
                                           area_lights);

                if (pdf_light > 0.0) {
                    double mis_weight =
                        pdf_bsdf / (pdf_bsdf + pdf_light);
                    emitted = emitted * mis_weight;
                }
            }
        }
        Ray scattered;
        Vec3 attenuation;

        if (rec.material->scatter(r, rec,
                                  attenuation,
                                  scattered))
        {
            Vec3 direct = {0, 0, 0};
            const Lambertian* lambertian =
                dynamic_cast<const Lambertian*>(rec.material);

            if (lambertian) {
                Vec3 diffuse_albedo = rec.material->aov_albedo(rec);
                direct = sample_direct_lighting(rec,
                                                world,
                                                diffuse_albedo,
                                                point_lights,
                                                area_lights,
                                                settings.area_light_samples);
            }

            Vec3 bounce_attenuation = attenuation;
            if (depth <= settings.rr_start_depth) {
                double survival =
                    std::min(settings.rr_max_survival,
                             std::max(settings.rr_min_survival,
                                      max_component(attenuation)));

                if (random_double() > survival)
                    return emitted + direct;

                bounce_attenuation = attenuation * (1.0 / survival);
            }

            Vec3 shaded =
                emitted +
                direct +
                bounce_attenuation *
                trace(scattered,
                      world,
                      point_lights,
                      area_lights,
                      settings,
                      depth - 1,
                      lambertian != nullptr,
                      rec.point,
                      rec.normal);
            const double segment_distance = rec.t * r.direction.norm();
            return apply_atmospheric_perspective(shaded, segment_distance, settings);
        }

        const double segment_distance = rec.t * r.direction.norm();
        return apply_atmospheric_perspective(emitted, segment_distance, settings);
    }

    return sky_color(r);
}
