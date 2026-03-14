#pragma once

#include <string>

#include "math/vec3.h"
#include "render/settings.h"
#include "scene/presets/scene_params.h"

enum class CliParseResult {
    Ok,
    Help,
    PrintOptionsSchema,
    PrintSceneRegistry,
    Error
};

struct CameraPreset {
    Vec3 origin{};
    Vec3 lookat{};
    double fov_degrees = 0.0;
};

struct ParsedArgs {
    std::string scene_name = "spheres";
    std::string output_base = "output";
    bool output_base_override = false;

    bool compare_mode = false;
    std::string compare_ref;
    std::string compare_candidate;
    double rmse_threshold = -1.0;
    double ssim_threshold = -1.0;

    RenderSettings settings{};

    bool width_override = false;
    bool height_override = false;
    bool spp_override = false;
    bool max_depth_override = false;
    bool aperture_override = false;
    bool focus_distance_override = false;
    bool exposure_override = false;
    bool adaptive_override = false;
    bool adaptive_min_spp_override = false;
    bool adaptive_threshold_override = false;
    bool aov_space_override = false;
    bool denoise_override = false;
    bool ppm_denoised_only_override = false;
    bool sss_random_walk_override = false;
    bool atmosphere_override = false;
    bool atmosphere_density_override = false;
    bool atmosphere_color_override = false;
    bool tile_size_override = false;
    bool thread_count_override = false;
    bool aberration_speed_override = false;
    bool aberration_yaw_override = false;
    bool aberration_pitch_override = false;
    bool lighting_sweep_override = false;

    int width_value = 0;
    int height_value = 0;
    int spp_value = 0;
    int max_depth_value = 0;
    double aperture_value = 0.0;
    double focus_distance_value = 0.0;
    double exposure_value = 0.0;
    bool adaptive_value = false;
    int adaptive_min_spp_value = 0;
    double adaptive_threshold_value = 0.0;
    bool aov_camera_space_value = false;
    bool denoise_value = false;
    bool ppm_denoised_only_value = false;
    bool sss_random_walk_value = false;
    bool atmosphere_value = false;
    double atmosphere_density_value = 0.0;
    Vec3 atmosphere_color_value{};
    int tile_size_value = 0;
    int thread_count_value = 0;
    double aberration_speed_value = 0.0;
    double aberration_yaw_value = 0.0;
    double aberration_pitch_value = 0.0;
    bool lighting_sweep_value = false;

    bool cam_origin_override = false;
    bool cam_x_override = false;
    bool cam_y_override = false;
    bool cam_z_override = false;
    bool cam_lookat_override = false;
    bool cam_forward_override = false;
    bool cam_yaw_override = false;
    bool cam_pitch_override = false;
    bool fov_override = false;
    bool cam_preset_override = false;
    Vec3 cam_origin_value{};
    double cam_x_value = 0.0;
    double cam_y_value = 0.0;
    double cam_z_value = 0.0;
    Vec3 cam_lookat_value{};
    Vec3 cam_forward_value{};
    double cam_yaw_turns_value = 0.0;
    double cam_pitch_turns_value = 0.0;
    double fov_degrees = 0.0;
    std::string cam_preset_name;

    SceneParamOverrides scene_param_overrides{};
};

void print_usage();
bool lookup_camera_preset(const std::string& name, CameraPreset& out);

CliParseResult parse_cli_args(int argc,
                              char** argv,
                              ParsedArgs& out,
                              std::string& error_message,
                              bool& show_usage_on_error);
