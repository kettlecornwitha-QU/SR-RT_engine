#include "scene/builders/big_scatter_builder.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#include "geometry/constant_medium.h"
#include "geometry/plane.h"
#include "geometry/sphere.h"
#include "materials/checker_lambertian.h"
#include "materials/conductor_metal.h"
#include "materials/dielectric.h"
#include "materials/isotropic.h"
#include "scene/color/scatter_color.h"
#include "scene/lighting_presets.h"
#include "scene/layout/scatter_layout.h"
#include "scene/random/hash.h"
#include "scene/polyhedra_builder.h"

namespace {

template <typename T, typename... Args>
T* add_material(SceneBuild& s, Args&&... args) {
    auto mat = std::make_shared<T>(std::forward<Args>(args)...);
    T* raw = mat.get();
    s.materials.push_back(mat);
    return raw;
}

} // namespace

void build_big_scatter_scene(SceneBuild& s,
                             RenderSettings& settings,
                             ScatterMaterialMode scatter_mode,
                             const BigScatterParams& params)
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

    s.camera.origin = {0.0, 2.2, 18.0};
    s.camera.forward = {0.0, -0.10, -1.0};
    s.camera.half_fov_x = 3.14159265358979323846 / 4.2;

    append_lighting_preset(
        s.point_lights,
        s.area_lights,
        resolve_lighting_preset(settings.lighting_preset, LightingPresetId::BigScatterDefault));

    auto* checker = add_material<CheckerLambertian>(
        s,
        Vec3{0.88, 0.88, 0.88},
        Vec3{0.26, 0.26, 0.26},
        0.45, 0.45);

    auto* glass = add_material<Dielectric>(s, 1.5);
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
    const double ground_y = params.ground_y;
    const double spacing = params.spacing;
    const double min_field_radius = params.min_field_radius;
    const double max_field_radius = params.max_field_radius;
    const int grid_extent = static_cast<int>(std::ceil(max_field_radius / spacing)) + 2;
    const double kPolyBoundScale = params.poly_bound_scale;

    struct PlacedObject {
        Vec3 center;
        double bound_radius;
    };
    std::vector<PlacedObject> placed;
    placed.reserve(params.placed_reserve);

    for (int gz = -grid_extent; gz <= grid_extent; ++gz) {
        for (int gx = -grid_extent; gx <= grid_extent; ++gx) {
            double jitter_x = (scene::random::hash01(gx, gz, scene::random::SeedChannel::BigScatterJitterX) - 0.5) *
                              params.jitter_amount;
            double jitter_z = (scene::random::hash01(gx, gz, scene::random::SeedChannel::BigScatterJitterZ) - 0.5) *
                              params.jitter_amount;
            Vec3 c = {
                cam_x + gx * spacing + jitter_x,
                ground_y,
                cam_z + gz * spacing + jitter_z
            };

            double dx = c.x - cam_x;
            double dz = c.z - cam_z;
            double dist = std::sqrt(dx * dx + dz * dz);
            if (dist < min_field_radius || dist > max_field_radius)
                continue;

            const double dist_t =
                std::min(1.0, std::max(0.0, (dist - min_field_radius) / (max_field_radius - min_field_radius)));
            const double size_rand = scene::random::hash01(gx, gz, scene::random::SeedChannel::BigScatterSizePick);
            const double size = params.size_base +
                                params.size_rand_scale * std::pow(size_rand, params.size_rand_power) +
                                params.size_dist_scale * dist_t;

            const double shape_pick = scene::random::hash01(gx, gz, scene::random::SeedChannel::BigScatterShapePick);
            const bool base_shape_sphere = (shape_pick < params.shape_sphere_threshold);
            const bool volume_slot =
                (scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialMixedVolumeGate) <
                 params.volume_slot_rate);
            const bool is_sphere = base_shape_sphere ||
                                   volume_slot ||
                                   (scatter_mode == ScatterMaterialMode::IsotropicVolume);
            const bool is_cube = !is_sphere && (shape_pick < params.shape_cube_threshold);

            double bound_radius = 0.0;
            Vec3 center = c;
            if (is_sphere) {
                bound_radius = size * 0.5;
            } else {
                bound_radius = kPolyBoundScale * size;
            }
            const Vec3 overlap_center = Vec3{center.x, ground_y + bound_radius, center.z};

            bool overlaps = false;
            for (const auto& p : placed) {
                const double min_sep =
                    (bound_radius + p.bound_radius) * params.overlap_radius_scale + params.overlap_padding;
                if ((overlap_center - p.center).norm_squared() < min_sep * min_sep) {
                    overlaps = true;
                    break;
                }
            }
            if (overlaps)
                continue;

            Vec3 base = scene::color::make_big_scatter_pleasant_base(gx,
                                                                      gz,
                                                                      dist_t,
                                                                      settings.big_scatter_palette);

            ScatterMaterialChoice choice =
                choose_scatter_material(s, scatter_mode, base, scatter_ctx, gx, gz);

            if (choice.use_volume) {
                auto* phase = add_material<Isotropic>(s, choice.volume_albedo);
                const Vec3 sphere_center = Vec3{center.x, ground_y + bound_radius, center.z};
                auto boundary = std::make_shared<Sphere>(sphere_center, bound_radius, glass);
                s.world.add(boundary);
                s.world.add(std::make_shared<ConstantMedium>(boundary,
                                                             choice.volume_density,
                                                             phase));
            } else if (choice.surface_material) {
                if (is_sphere) {
                    const Vec3 sphere_center = Vec3{center.x, ground_y + bound_radius, center.z};
                    s.world.add(std::make_shared<Sphere>(sphere_center, bound_radius, choice.surface_material));
                } else if (is_cube) {
                    const Vec3 cube_center = Vec3{center.x, ground_y + 0.5 * size, center.z};
                    add_cube(s, cube_center, size, choice.surface_material);
                } else {
                    add_tetrahedron_flat_base(s, center, size, choice.surface_material);
                }
            }

            placed.push_back({overlap_center, bound_radius});
        }
    }
}
