#include "scene/builders/scatter_builder.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#include "geometry/constant_medium.h"
#include "geometry/plane.h"
#include "geometry/rectangle.h"
#include "geometry/sphere.h"
#include "materials/checker_lambertian.h"
#include "materials/conductor_metal.h"
#include "materials/dielectric.h"
#include "materials/emissive.h"
#include "materials/isotropic.h"
#include "scene/color/scatter_color.h"
#include "scene/lighting_presets.h"
#include "scene/layout/scatter_layout.h"
#include "scene/random/hash.h"

namespace {

constexpr double kPi = 3.14159265358979323846;

template <typename T, typename... Args>
T* add_material(SceneBuild& s, Args&&... args) {
    auto mat = std::make_shared<T>(std::forward<Args>(args)...);
    T* raw = mat.get();
    s.materials.push_back(mat);
    return raw;
}

} // namespace

void build_scatter_scene(SceneBuild& s,
                         RenderSettings& settings,
                         ScatterMaterialMode scatter_mode,
                         const ScatterParams& params)
{
    settings.image_width = 1920;
    settings.image_height = 1080;
    settings.samples_per_pixel = 128;
    settings.max_depth = 5;
    settings.aperture = 0.0;
    settings.focus_distance = 4.0;
    settings.exposure = 1.0;
    settings.adaptive_sampling = true;
    settings.adaptive_min_spp = 8;
    settings.adaptive_threshold = 0.01;

    s.camera.origin = {0.0, 1.0, 7.5};
    s.camera.forward = {0.0, -0.18, -1.0};
    s.camera.half_fov_x = kPi / 4.8;

    append_lighting_preset(
        s.point_lights,
        s.area_lights,
        resolve_lighting_preset(settings.lighting_preset, LightingPresetId::ScatterDefault));

    auto* checker = add_material<CheckerLambertian>(
        s,
        Vec3{0.86, 0.86, 0.86},
        Vec3{0.28, 0.28, 0.28},
        2.0, 2.0);

    auto* glass = add_material<Dielectric>(s, 1.5);
    auto* area_emissive = add_material<Emissive>(s, Vec3{12.0, 12.0, 11.5});
    const bool use_random_walk_sss = settings.sss_random_walk;
    auto* conductor_copper = add_material<ConductorMetal>(
        s,
        Vec3{0.271, 0.675, 1.316},
        Vec3{3.609, 2.624, 2.292},
        0.08);
    auto* conductor_aluminum = add_material<ConductorMetal>(
        s,
        Vec3{1.50, 0.98, 0.62},
        Vec3{7.30, 6.60, 5.40},
        0.12);
    ScatterMaterialContext scatter_ctx;
    scatter_ctx.glass = glass;
    scatter_ctx.conductor_copper = conductor_copper;
    scatter_ctx.conductor_aluminum = conductor_aluminum;
    scatter_ctx.use_random_walk_sss = use_random_walk_sss;

    s.world.add(std::make_shared<Plane>(Vec3{0, params.ground_y, 0}, Vec3{0, 1, 0}, checker));

    const double cam_x = s.camera.origin.x;
    const double cam_z = s.camera.origin.z;
    const double spacing = params.spacing;
    const double max_sat_dz = params.max_sat_dz;
    const double min_field_radius = params.min_field_radius;
    const double max_field_radius = params.max_field_radius;
    const int grid_extent = static_cast<int>(std::ceil(max_field_radius / spacing)) + 2;

    struct PlacedSphere {
        Vec3 center;
        double radius;
    };
    std::vector<PlacedSphere> placed;
    placed.reserve(params.placed_reserve);

    for (int gz = -grid_extent; gz <= grid_extent; ++gz) {
        for (int gx = -grid_extent; gx <= grid_extent; ++gx) {
            double jitter_x = (scene::random::hash01(gx, gz, scene::random::SeedChannel::ScatterJitterX) - 0.5) *
                              params.jitter_amount;
            double jitter_z = (scene::random::hash01(gx, gz, scene::random::SeedChannel::ScatterJitterZ) - 0.5) *
                              params.jitter_amount;
            Vec3 c = {
                cam_x + gx * spacing + jitter_x,
                params.ground_y,
                cam_z + gz * spacing + jitter_z
            };

            double dx = c.x - cam_x;
            double dz = c.z - cam_z;
            double dist = std::sqrt(dx * dx + dz * dz);
            if (dist < min_field_radius || dist > max_field_radius)
                continue;

            Vec3 base = scene::color::make_scatter_blue_or_red_base(gx, gz, dz, max_sat_dz);

            const double depth = std::fabs(dz);
            const double dist_t =
                std::min(1.0, std::max(0.0, (depth - min_field_radius) / (max_field_radius - min_field_radius)));
            const double growth = 1.0 + params.radius_growth_scale * std::pow(dist_t, params.radius_growth_power);
            double r = (params.radius_base + params.radius_dist_scale * dist) * growth *
                       (params.radius_rand_min + params.radius_rand_scale *
                        scene::random::hash01(gx, gz, scene::random::SeedChannel::ScatterRadiusJitter));
            r = std::max(params.radius_min, std::min(params.radius_max, r));

            Vec3 sphere_center = Vec3{c.x, c.y + r, c.z};
            bool overlaps = false;
            for (const auto& p : placed) {
                const double min_sep = (r + p.radius) * params.overlap_radius_scale + params.overlap_padding;
                if ((sphere_center - p.center).norm_squared() < min_sep * min_sep) {
                    overlaps = true;
                    break;
                }
            }
            if (overlaps)
                continue;

            ScatterMaterialChoice choice =
                choose_scatter_material(s, scatter_mode, base, scatter_ctx, gx, gz);

            if (choice.use_volume) {
                auto* phase = add_material<Isotropic>(s, choice.volume_albedo);
                auto boundary = std::make_shared<Sphere>(sphere_center, r, glass);
                s.world.add(boundary);
                s.world.add(std::make_shared<ConstantMedium>(boundary,
                                                             choice.volume_density,
                                                             phase));
            } else if (choice.surface_material) {
                s.world.add(std::make_shared<Sphere>(sphere_center,
                                                     r,
                                                     choice.surface_material));
            }
            placed.push_back({sphere_center, r});
        }
    }

    s.world.add(std::make_shared<Rectangle>(
        Vec3{0.0, 4.8, -6.0},
        Vec3{3.6, 0.0, 0.0},
        Vec3{0.0, 0.0, 2.2},
        area_emissive));
}
