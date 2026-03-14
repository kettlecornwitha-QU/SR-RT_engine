#include "render/renderer.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <thread>

#include "render/aberration.h"
#include "render/integrator.h"
#include "render/output.h"
#include "render/rng.h"
#include "render/sampling.h"

namespace {

double luminance(const Vec3& c) {
    return 0.2126 * c.x + 0.7152 * c.y + 0.0722 * c.z;
}

} // namespace

RenderResult render_scene(const SceneBuild& scene,
                          const RenderSettings& settings)
{
    RenderResult result;
    result.framebuffer.assign(settings.image_width * settings.image_height, Vec3{0, 0, 0});
    result.albedo_aov.assign(settings.image_width * settings.image_height, Vec3{0, 0, 0});
    result.normal_aov.assign(settings.image_width * settings.image_height, Vec3{0, 0, 0});

    Vec3 player_velocity = {0.0, 0.0, 0.0};
    Vec3 origin = scene.camera.origin;
    Vec3 forward = scene.camera.forward.normalize();
    Vec3 right = scene.camera.world_up.cross(forward).normalize();
    Vec3 up = forward.cross(right);
    Vec3 camera_forward = forward;
    const double lens_radius = 0.5 * settings.aperture;

    double image_plane_x_max = std::tan(scene.camera.half_fov_x);
    double image_plane_y_max =
        image_plane_x_max *
        double(settings.image_height) / settings.image_width;

    player_velocity = aberration::world_velocity_from_turns(
        settings.aberration_speed,
        settings.aberration_yaw_turns,
        settings.aberration_pitch_turns);

    const int tiles_x = (settings.image_width + settings.tile_size - 1) / settings.tile_size;
    const int tiles_y = (settings.image_height + settings.tile_size - 1) / settings.tile_size;
    const int total_tiles = tiles_x * tiles_y;
    std::atomic<int> next_tile{0};
    std::atomic<int> completed_tiles{0};
    std::atomic<uint64_t> total_primary_samples{0};
    std::atomic<bool> render_done{false};

    if (settings.thread_count > 0) {
        result.thread_count = static_cast<unsigned int>(settings.thread_count);
    } else {
        result.thread_count = std::max(1u, std::thread::hardware_concurrency());
    }
    std::vector<std::thread> workers;
    workers.reserve(result.thread_count);

    auto start = std::chrono::steady_clock::now();

    auto render_tile = [&](unsigned int worker_id) {
        (void)worker_id;
        uint64_t local_samples = 0;
        while (true) {
            int tile_index = next_tile.fetch_add(1);
            if (tile_index >= total_tiles)
                break;

            int tx = tile_index % tiles_x;
            int ty = tile_index / tiles_x;
            int x0 = tx * settings.tile_size;
            int y0 = ty * settings.tile_size;
            int x1 = std::min(x0 + settings.tile_size, settings.image_width);
            int y1 = std::min(y0 + settings.tile_size, settings.image_height);

            for (int j = y0; j < y1; ++j) {
                for (int i = x0; i < x1; ++i) {
                    Vec3 color = {0, 0, 0};
                    double luma_mean = 0.0;
                    double luma_m2 = 0.0;
                    int used_spp = 0;

                    const int max_spp = std::max(1, settings.samples_per_pixel);
                    const int min_spp = std::min(max_spp, std::max(1, settings.adaptive_min_spp));

                    for (int sidx = 0; sidx < max_spp; ++sidx) {
                        seed_rng(sample_seed(settings.rng_seed, i, j, sidx));

                        double u =
                            image_plane_x_max *
                            ((2.0 * (i + random_double())) / settings.image_width - 1.0);

                        double v =
                            image_plane_y_max *
                            (1.0 - (2.0 * (j + random_double())) / settings.image_height);

                        Vec3 d_prime = right * u + up * v + forward;
                        Vec3 boosted =
                            aberration::relativistic_aberration(d_prime, player_velocity);

                        Vec3 pinhole_dir = boosted.normalize();
                        double focus_t =
                            settings.focus_distance /
                            std::max(1e-8, pinhole_dir.dot(camera_forward));

                        Vec3 focus_point = origin + pinhole_dir * focus_t;
                        Vec3 lens_sample = lens_radius * random_in_unit_disk();
                        Vec3 lens_offset = right * lens_sample.x + up * lens_sample.y;

                        Ray ray;
                        ray.origin = origin + lens_offset;
                        ray.direction = (focus_point - ray.origin).normalize();

                        Vec3 sample_color = trace(ray,
                                                  scene.world,
                                                  scene.point_lights,
                                                  scene.area_lights,
                                                  settings,
                                                  settings.max_depth);
                        color = color + sample_color;

                        used_spp++;
                        local_samples++;

                        if (settings.adaptive_sampling) {
                            double y = luminance(sample_color);
                            double delta = y - luma_mean;
                            luma_mean += delta / used_spp;
                            double delta2 = y - luma_mean;
                            luma_m2 += delta * delta2;

                            if (used_spp >= min_spp && used_spp > 1) {
                                double variance = luma_m2 / (used_spp - 1);
                                double std_error =
                                    std::sqrt(std::max(0.0, variance / used_spp));
                                if (std_error < settings.adaptive_threshold)
                                    break;
                            }
                        }
                    }

                    result.framebuffer[j * settings.image_width + i] =
                        color * (1.0 / std::max(1, used_spp));

                    // AOVs don't need per-sample Monte Carlo accumulation.
                    // One deterministic pinhole primary ray per pixel keeps the
                    // denoiser guides stable while avoiding extra BVH traversals.
                    Vec3 aov_albedo = {0, 0, 0};
                    Vec3 aov_normal = {0, 0, 0};
                    double aov_u =
                        image_plane_x_max *
                        ((2.0 * (i + 0.5)) / settings.image_width - 1.0);
                    double aov_v =
                        image_plane_y_max *
                        (1.0 - (2.0 * (j + 0.5)) / settings.image_height);
                    Vec3 aov_d_prime = right * aov_u + up * aov_v + forward;
                    Vec3 aov_boosted =
                        aberration::relativistic_aberration(aov_d_prime, player_velocity);

                    Ray aov_ray;
                    aov_ray.origin = origin;
                    aov_ray.direction = aov_boosted.normalize();
                    if (gather_primary_aovs(aov_ray, scene.world, aov_albedo, aov_normal)) {
                        if (settings.aov_normal_camera_space) {
                            aov_normal = {
                                aov_normal.dot(right),
                                aov_normal.dot(up),
                                aov_normal.dot(camera_forward)
                            };
                            aov_normal = aov_normal.normalize();
                        }
                    }

                    result.albedo_aov[j * settings.image_width + i] = aov_albedo;
                    result.normal_aov[j * settings.image_width + i] = aov_normal;
                }
            }
            completed_tiles.fetch_add(1, std::memory_order_relaxed);
        }
        total_primary_samples.fetch_add(local_samples, std::memory_order_relaxed);
    };

    std::thread progress_thread([&]() {
        while (!render_done.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            int done = completed_tiles.load(std::memory_order_relaxed);
            double progress = 100.0 * done / std::max(1, total_tiles);
            double elapsed = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - start).count();

            double eta = 0.0;
            if (done > 0) {
                double tiles_per_second = done / std::max(1e-8, elapsed);
                eta = (total_tiles - done) / std::max(1e-8, tiles_per_second);
            }

            std::cout << "\rProgress: "
                      << std::fixed << std::setprecision(1)
                      << progress << "% (" << done << "/" << total_tiles
                      << " tiles), ETA: "
                      << std::setprecision(1) << eta << "s"
                      << std::flush;
        }
    });

    for (unsigned int t = 0; t < result.thread_count; ++t)
        workers.emplace_back(render_tile, t);
    for (auto& worker : workers)
        worker.join();

    render_done.store(true, std::memory_order_relaxed);
    progress_thread.join();
    std::cout << "\rProgress: 100.0% (" << total_tiles << "/" << total_tiles
              << " tiles), ETA: 0.0s\n";

    result.elapsed_seconds = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - start).count();
    result.total_primary_samples = total_primary_samples.load(std::memory_order_relaxed);
    return result;
}
