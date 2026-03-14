#include "scene/builders/row_scatter_builder.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#include "geometry/plane.h"
#include "geometry/sphere.h"
#include "materials/checker_material.h"
#include "materials/clearcoat.h"
#include "materials/conductor_metal.h"
#include "materials/lambertian.h"
#include "scene/builders/row_scatter_material_policy.h"
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

void build_row_scatter_scene(SceneBuild& s,
                             RenderSettings& settings,
                             const RowScatterParams& params)
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

    s.camera.origin = {0.0, 1.2, 9.0};
    s.camera.forward = {0.0, -0.07, -1.0};
    s.camera.half_fov_x = kPi / 5.2;

    append_lighting_preset(
        s.point_lights,
        s.area_lights,
        resolve_lighting_preset(settings.lighting_preset, LightingPresetId::CornellCeilingPlusKey));

    auto* floor_copper = add_material<ConductorMetal>(
        s,
        Vec3{0.271, 0.675, 1.316},
        Vec3{3.609, 2.624, 2.292},
        0.18);
    auto* floor_white_base = add_material<Lambertian>(s, Vec3{0.92, 0.92, 0.92});
    auto* floor_white_clearcoat = add_material<Clearcoat>(s, floor_white_base, 1.5, 0.05);
    auto* checker = add_material<CheckerMaterial>(
        s,
        floor_copper,
        floor_white_clearcoat,
        2.0, 2.0);
    s.world.add(std::make_shared<Plane>(Vec3{0, -0.5, 0}, Vec3{0, 1, 0}, checker));

    const double ground_y = params.ground_y;
    const double x_max = params.x_max;

    const std::vector<double> positive_columns = scene::layout::build_row_scatter_positive_columns(x_max);
    const std::vector<double> columns = scene::layout::build_symmetric_columns_from_positive(positive_columns);

    const double max_radius = scene::layout::row_scatter_radius_from_abs_x(x_max, x_max);
    const double row_spacing = params.row_spacing_scale * max_radius;
    const double z_back = params.z_back;
    const double z_front = params.z_front;
    const int row_count = std::max(2,
        static_cast<int>(std::floor((z_front - z_back) / row_spacing)) + 1);

    struct PlacedSphere {
        Vec3 center;
        double radius;
    };
    std::vector<PlacedSphere> placed;
    placed.reserve(row_count * columns.size());

    const RowScatterMaterialPolicy material_policy = make_row_scatter_material_policy(params);

    for (int row = 0; row < row_count; ++row) {
        const double z_nominal = z_front - row * row_spacing;

        for (int col = 0; col < static_cast<int>(columns.size()); ++col) {
            const double x_nominal = columns[static_cast<std::size_t>(col)];

            const double ideal_radius = scene::layout::row_scatter_radius_from_abs_x(std::fabs(x_nominal), x_max);
            double radius = ideal_radius;
            const double radius_noise =
                1.0 + params.radius_noise_amount *
                          (scene::random::hash01(row, col, scene::random::SeedChannel::RowScatterRadiusNoise) - 0.5) * 2.0;
            radius *= radius_noise;
            radius = std::max(params.min_radius, radius);

            const double jitter_x =
                (scene::random::hash01(row, col, scene::random::SeedChannel::RowScatterJitterX) - 0.5) *
                params.jitter_x_amount * radius;
            const double jitter_z =
                (scene::random::hash01(row, col, scene::random::SeedChannel::RowScatterJitterZ) - 0.5) *
                params.jitter_z_amount * radius;

            auto has_overlap = [&](const Vec3& c, double r) {
                for (const auto& p : placed) {
                    const double min_sep = (r + p.radius) * params.overlap_radius_scale + params.overlap_padding;
                    if ((c - p.center).norm_squared() < min_sep * min_sep)
                        return true;
                }
                return false;
            };

            double x = x_nominal + jitter_x;
            double z = z_nominal + jitter_z;
            Vec3 center = {x, ground_y + radius, z};

            // Keep every slot populated: progressively reduce jitter/size if needed.
            bool overlaps = has_overlap(center, radius);
            const int overlap_attempts = std::max(1, params.overlap_relax_attempts);
            for (int attempt = 0; overlaps && attempt < overlap_attempts; ++attempt) {
                const double t = (attempt + 1) / static_cast<double>(overlap_attempts);
                const double j_scale = 1.0 - params.overlap_jitter_relax * t;
                radius = std::max(params.min_radius, radius * (1.0 - params.overlap_radius_relax * t));
                x = x_nominal + jitter_x * j_scale;
                z = z_nominal + jitter_z * j_scale;
                center = {x, ground_y + radius, z};
                overlaps = has_overlap(center, radius);
            }
            if (overlaps) {
                radius = std::max(params.min_radius,
                                  std::min(radius, ideal_radius * params.overlap_fallback_radius_scale));
                x = x_nominal;
                z = z_nominal;
                center = {x, ground_y + radius, z};
            }

            const Vec3 base = scene::color::make_row_scatter_side_color(x, row, col, x_max);

            const Material* sphere_material = make_row_scatter_material_for_slot(
                s,
                params,
                material_policy,
                row,
                col,
                base);

            s.world.add(std::make_shared<Sphere>(center, radius, sphere_material));
            placed.push_back({center, radius});
        }
    }

    // Additional front row (+z) of large copper spheres.
    auto* copper_row = add_material<ConductorMetal>(
        s,
        Vec3{0.271, 0.675, 1.316},
        Vec3{3.609, 2.624, 2.292},
        0.10);
    const double copper_radius = 1.2 - ground_y;
    const double center_spacing = 2.0 * copper_radius * 1.05;
    const double x_limit = x_max - copper_radius;

    if (x_limit >= 0.0) {
        auto add_curved_copper_sphere = [&](double x) {
            // Curve the front row inward: center spheres sit closer to the next row,
            // while edge spheres stay farther to avoid crowding larger edge spheres.
            const double next_row_radius =
                scene::layout::row_scatter_radius_from_abs_x(std::fabs(x), x_max);
            const double target_center_distance = 1.05 * (copper_radius + next_row_radius);
            const double dy = copper_radius - next_row_radius;
            const double dz = std::sqrt(std::max(
                0.0,
                target_center_distance * target_center_distance - dy * dy));
            const double copper_z = z_front + dz;
            s.world.add(std::make_shared<Sphere>(Vec3{x, ground_y + copper_radius, copper_z},
                                                 copper_radius,
                                                 copper_row));
        };

        add_curved_copper_sphere(0.0);
        for (int i = 1;; ++i) {
            const double dx = center_spacing * static_cast<double>(i);
            if (dx > x_limit + 1e-9)
                break;
            add_curved_copper_sphere(dx);
            add_curved_copper_sphere(-dx);
        }
    }
}
