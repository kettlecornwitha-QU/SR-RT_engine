#pragma once

#include "math/vec3.h"

struct RenderSettings {
    enum MaterialsVariant {
        MaterialsAll = 0,
        MaterialsAnisoGold = 1,
        MaterialsAnisoSteel = 2,
        MaterialsClearcoat = 3,
        MaterialsChrome = 4,
        MaterialsGlass = 5,
        MaterialsThinTransmission = 6,
        MaterialsSheen = 7,
        MaterialsConductorCopper = 8,
        MaterialsConductorAluminum = 9,
        MaterialsCoatedPlastic = 10
    };
    enum BigScatterPalette {
        BigScatterPaletteBalanced = 0,
        BigScatterPaletteVivid = 1,
        BigScatterPaletteEarthy = 2
    };
    enum LightingPreset {
        LightingPresetSceneDefault = 0,
        LightingPresetScatterDefault = 1,
        LightingPresetBigScatterDefault = 2,
        LightingPresetRowScatterDefault = 3,
        LightingPresetMaterialsStudio = 4,
        LightingPresetCornellCeiling = 5,
        LightingPresetCornellCeilingPlusKey = 6,
        LightingPresetBenchmarkSoftbox = 7,
        LightingPresetVolumesBacklit = 8
    };

    int image_width = 1920;
    int image_height = 1080;
    int max_depth = 5;
    int samples_per_pixel = 128;
    bool adaptive_sampling = true;
    int adaptive_min_spp = 8;
    double adaptive_threshold = 0.01;
    bool aov_normal_camera_space = false;
    bool denoise = true;
    bool ppm_denoised_only = false;
    bool sss_random_walk = false;
    bool atmosphere_enabled = false;
    double atmosphere_density = 0.03;
    Vec3 atmosphere_color = {0.74, 0.83, 0.95};
    int area_light_samples = 4;
    int rr_start_depth = 3;
    double rr_min_survival = 0.1;
    double rr_max_survival = 0.95;
    double aperture = 0.0;
    double focus_distance = 4.0;
    double exposure = 1.0;
    double aberration_speed = 0.0;      // beta in [0, 1)
    double aberration_yaw_turns = 0.0;  // yaw / (2*pi), wrapped to [0, 1)
    double aberration_pitch_turns = 0.0; // pitch / (2*pi), camera-local
    int materials_variant = MaterialsAll;
    int big_scatter_palette = BigScatterPaletteBalanced;
    int lighting_preset = LightingPresetSceneDefault;
    int gltf_triangle_step = 1;
    int tile_size = 32;
    int thread_count = 0; // 0 = auto (hardware_concurrency)
    unsigned int rng_seed = 1337u;
};
