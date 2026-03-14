#include "scene/presets/scene_configs.h"
#include "scene/lighting_presets.h"

namespace {

constexpr double kPi = 3.14159265358979323846;

} // namespace

ScenePresetConfig make_materials_config() {
    ScenePresetConfig cfg;
    cfg.apply_render_settings = true;
    cfg.render_settings.image_width = 1280;
    cfg.render_settings.image_height = 720;
    cfg.render_settings.samples_per_pixel = 64;
    cfg.render_settings.max_depth = 8;
    cfg.render_settings.aperture = 0.0;
    cfg.render_settings.focus_distance = 6.0;
    cfg.render_settings.exposure = 1.3;
    cfg.render_settings.adaptive_sampling = true;
    cfg.render_settings.adaptive_min_spp = 12;
    cfg.render_settings.adaptive_threshold = 0.008;

    cfg.apply_camera = true;
    cfg.camera.origin = {0.0, 1.0, 4.6};
    cfg.camera.forward = {0.0, -0.05, -1.0};
    cfg.camera.half_fov_x = kPi / 5.5;

    append_lighting_preset(cfg.point_lights, cfg.area_lights, LightingPresetId::MaterialsStudio);
    return cfg;
}

ScenePresetConfig make_cornell_config() {
    ScenePresetConfig cfg;
    cfg.apply_render_settings = true;
    cfg.render_settings.aperture = 0.0;
    cfg.render_settings.focus_distance = 4.0;
    cfg.render_settings.exposure = 1.2;

    cfg.apply_camera = true;
    cfg.camera.origin = {0.0, 1.0, 3.8};
    cfg.camera.forward = {0.0, 0.0, -1.0};
    cfg.camera.half_fov_x = kPi / 6.0;

    append_lighting_preset(cfg.point_lights, cfg.area_lights, LightingPresetId::CornellCeiling);
    return cfg;
}

ScenePresetConfig make_benchmark_config() {
    ScenePresetConfig cfg;
    cfg.apply_render_settings = true;
    cfg.render_settings.image_width = 1280;
    cfg.render_settings.image_height = 720;
    cfg.render_settings.samples_per_pixel = 8;
    cfg.render_settings.max_depth = 4;
    cfg.render_settings.aperture = 0.0;
    cfg.render_settings.focus_distance = 4.0;
    cfg.render_settings.exposure = 1.0;

    cfg.apply_camera = true;
    cfg.camera.origin = {0.0, 1.1, 5.5};
    cfg.camera.forward = {0.0, -0.12, -1.0};

    append_lighting_preset(cfg.point_lights, cfg.area_lights, LightingPresetId::BenchmarkSoftbox);
    return cfg;
}

ScenePresetConfig make_volumes_config() {
    ScenePresetConfig cfg;
    cfg.apply_render_settings = true;
    cfg.render_settings.image_width = 1920;
    cfg.render_settings.image_height = 1080;
    cfg.render_settings.samples_per_pixel = 128;
    cfg.render_settings.max_depth = 5;
    cfg.render_settings.aperture = 0.0;
    cfg.render_settings.focus_distance = 4.0;
    cfg.render_settings.exposure = 1.0;
    cfg.render_settings.adaptive_sampling = true;
    cfg.render_settings.adaptive_min_spp = 8;
    cfg.render_settings.adaptive_threshold = 0.01;

    cfg.apply_camera = true;
    cfg.camera.origin = {0.0, 1.1, 6.8};
    cfg.camera.forward = {0.0, -0.06, -1.0};
    cfg.camera.half_fov_x = kPi / 5.4;

    append_lighting_preset(cfg.point_lights, cfg.area_lights, LightingPresetId::VolumesBacklit);
    return cfg;
}
