#include "app/cli.h"
#include "app/arg_parsers.h"
#include "scene/scene_presets.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

bool parse_vec3_csv(const std::string& s, Vec3& out) {
    size_t c1 = s.find(',');
    if (c1 == std::string::npos)
        return false;
    size_t c2 = s.find(',', c1 + 1);
    if (c2 == std::string::npos)
        return false;
    if (s.find(',', c2 + 1) != std::string::npos)
        return false;
    try {
        out.x = std::stod(s.substr(0, c1));
        out.y = std::stod(s.substr(c1 + 1, c2 - c1 - 1));
        out.z = std::stod(s.substr(c2 + 1));
    } catch (...) {
        return false;
    }
    return true;
}

void init_defaults_from_settings(ParsedArgs& out) {
    out.width_value = out.settings.image_width;
    out.height_value = out.settings.image_height;
    out.spp_value = out.settings.samples_per_pixel;
    out.max_depth_value = out.settings.max_depth;
    out.aperture_value = out.settings.aperture;
    out.focus_distance_value = out.settings.focus_distance;
    out.exposure_value = out.settings.exposure;
    out.adaptive_value = out.settings.adaptive_sampling;
    out.adaptive_min_spp_value = out.settings.adaptive_min_spp;
    out.adaptive_threshold_value = out.settings.adaptive_threshold;
    out.aov_camera_space_value = out.settings.aov_normal_camera_space;
    out.denoise_value = out.settings.denoise;
    out.ppm_denoised_only_value = out.settings.ppm_denoised_only;
    out.sss_random_walk_value = out.settings.sss_random_walk;
    out.atmosphere_value = out.settings.atmosphere_enabled;
    out.atmosphere_density_value = out.settings.atmosphere_density;
    out.atmosphere_color_value = out.settings.atmosphere_color;
    out.tile_size_value = out.settings.tile_size;
    out.thread_count_value = out.settings.thread_count;
    out.aberration_speed_value = out.settings.aberration_speed;
    out.aberration_yaw_value = out.settings.aberration_yaw_turns;
    out.aberration_pitch_value = out.settings.aberration_pitch_turns;
}

std::string join_name_list(const std::vector<std::string>& names)
{
    std::ostringstream oss;
    for (std::size_t i = 0; i < names.size(); ++i) {
        if (i > 0)
            oss << ", ";
        oss << names[i];
    }
    return oss.str();
}

} // namespace

void print_usage() {
    std::cout << "Usage:\n"
              << "  raytracer [--scene NAME] [--output-base PATH] [--width W] [--height H] [--spp N] [--max-depth D]\n"
              << "           [--aperture A] [--focus-distance F] [--exposure E]\n"
              << "           [--cam-origin X,Y,Z] [--cam-x X] [--cam-y Y] [--cam-z Z]\n"
              << "           [--cam-lookat X,Y,Z] [--cam-forward X,Y,Z] [--fov DEG]\n"
              << "           [--cam-yaw-turns T] [--cam-pitch-turns T]\n"
              << "           [--cam-preset NAME]\n"
              << "           [--aberration-speed BETA] [--aberration-yaw-turns T] [--aberration-pitch-turns T]\n"
              << "           [--adaptive 0|1] [--adaptive-min-spp N] [--adaptive-threshold T]\n"
              << "           [--gltf-triangle-step N]\n"
              << "           [--materials-variant all|aniso-gold|aniso-steel|clearcoat|chrome|glass|thin|sheen|conductor-copper|conductor-aluminum|coated-plastic]\n"
              << "           [--big-scatter-palette balanced|vivid|earthy]\n"
              << "           [--lighting-preset scene-default|scatter-default|big-scatter-default|row-scatter-default|materials-studio|cornell-ceiling|cornell-ceiling-plus-key|benchmark-softbox|volumes-backlit]\n"
              << "           [--lighting-sweep 0|1]\n"
              << "           [--scatter-spacing X] [--scatter-max-radius X] [--scatter-growth-scale X]\n"
              << "           [--big-scatter-spacing X] [--big-scatter-max-radius X]\n"
              << "           [--row-scatter-xmax X] [--row-scatter-z-front Z] [--row-scatter-z-back Z]\n"
              << "           [--row-scatter-replacement-rate R]\n"
              << "           [--threads N] [--tile-size N]\n"
              << "           [--aov-space world|camera]\n"
              << "           [--ppm-denoised-only 0|1]\n"
              << "           [--sss-random-walk 0|1]\n"
              << "           [--atmosphere 0|1] [--atmosphere-density D] [--atmosphere-color R,G,B]\n"
              << "           [--denoise 0|1]\n"
              << "  raytracer --compare REF.ppm CAND.ppm [--rmse-threshold X] [--ssim-threshold Y]\n";
    std::cout << "  raytracer --print-options-schema\n";
    std::cout << "  raytracer --print-scene-registry\n";

    const std::vector<std::string> primary = list_primary_scene_names();
    const std::vector<std::string> aliases = list_scene_alias_names();
    std::cout << "Scenes: " << join_name_list(primary) << "\n";
    if (!aliases.empty())
        std::cout << "Aliases: " << join_name_list(aliases) << "\n";
}

bool lookup_camera_preset(const std::string& name, CameraPreset& out) {
    if (name == "nave") {
        out.origin = {0.0, 1.7, 1.5};
        out.lookat = {0.0, 1.5, -8.0};
        out.fov_degrees = 55.0;
        return true;
    }
    if (name == "side") {
        out.origin = {-6.5, 1.8, 0.0};
        out.lookat = {0.0, 1.5, -8.0};
        out.fov_degrees = 52.0;
        return true;
    }
    if (name == "overview") {
        out.origin = {0.0, 5.5, 2.0};
        out.lookat = {0.0, 1.2, -8.0};
        out.fov_degrees = 60.0;
        return true;
    }
    return false;
}

CliParseResult parse_cli_args(int argc,
                              char** argv,
                              ParsedArgs& out,
                              std::string& error_message,
                              bool& show_usage_on_error)
{
    show_usage_on_error = false;
    error_message.clear();
    init_defaults_from_settings(out);

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--scene" && i + 1 < argc) {
            out.scene_name = argv[++i];
        } else if (arg == "--output-base" && i + 1 < argc) {
            out.output_base = argv[++i];
            out.output_base_override = true;
        } else if (arg == "--width" && i + 1 < argc) {
            out.width_value = std::stoi(argv[++i]);
            out.settings.image_width = out.width_value;
            out.width_override = true;
        } else if (arg == "--height" && i + 1 < argc) {
            out.height_value = std::stoi(argv[++i]);
            out.settings.image_height = out.height_value;
            out.height_override = true;
        } else if (arg == "--spp" && i + 1 < argc) {
            out.spp_value = std::stoi(argv[++i]);
            out.settings.samples_per_pixel = out.spp_value;
            out.spp_override = true;
        } else if (arg == "--max-depth" && i + 1 < argc) {
            out.max_depth_value = std::stoi(argv[++i]);
            out.settings.max_depth = out.max_depth_value;
            out.max_depth_override = true;
        } else if (arg == "--aperture" && i + 1 < argc) {
            out.aperture_value = std::stod(argv[++i]);
            out.settings.aperture = out.aperture_value;
            out.aperture_override = true;
        } else if (arg == "--focus-distance" && i + 1 < argc) {
            out.focus_distance_value = std::stod(argv[++i]);
            out.settings.focus_distance = out.focus_distance_value;
            out.focus_distance_override = true;
        } else if (arg == "--exposure" && i + 1 < argc) {
            out.exposure_value = std::stod(argv[++i]);
            out.settings.exposure = out.exposure_value;
            out.exposure_override = true;
        } else if (arg == "--cam-origin" && i + 1 < argc) {
            if (!parse_vec3_csv(argv[++i], out.cam_origin_value)) {
                error_message = "Invalid value for --cam-origin (expected X,Y,Z)";
                return CliParseResult::Error;
            }
            out.cam_origin_override = true;
        } else if (arg == "--cam-x" && i + 1 < argc) {
            out.cam_x_value = std::stod(argv[++i]);
            out.cam_x_override = true;
        } else if (arg == "--cam-y" && i + 1 < argc) {
            out.cam_y_value = std::stod(argv[++i]);
            out.cam_y_override = true;
        } else if (arg == "--cam-z" && i + 1 < argc) {
            out.cam_z_value = std::stod(argv[++i]);
            out.cam_z_override = true;
        } else if (arg == "--cam-lookat" && i + 1 < argc) {
            if (!parse_vec3_csv(argv[++i], out.cam_lookat_value)) {
                error_message = "Invalid value for --cam-lookat (expected X,Y,Z)";
                return CliParseResult::Error;
            }
            out.cam_lookat_override = true;
        } else if (arg == "--cam-forward" && i + 1 < argc) {
            if (!parse_vec3_csv(argv[++i], out.cam_forward_value)) {
                error_message = "Invalid value for --cam-forward (expected X,Y,Z)";
                return CliParseResult::Error;
            }
            out.cam_forward_override = true;
        } else if (arg == "--cam-yaw-turns" && i + 1 < argc) {
            out.cam_yaw_turns_value = std::stod(argv[++i]);
            out.cam_yaw_override = true;
        } else if (arg == "--cam-pitch-turns" && i + 1 < argc) {
            out.cam_pitch_turns_value = std::stod(argv[++i]);
            if (!(out.cam_pitch_turns_value > -0.249 && out.cam_pitch_turns_value < 0.249)) {
                error_message = "Invalid value for --cam-pitch-turns: " +
                                std::to_string(out.cam_pitch_turns_value) +
                                " (expected -0.249 < T < 0.249)";
                return CliParseResult::Error;
            }
            out.cam_pitch_override = true;
        } else if (arg == "--fov" && i + 1 < argc) {
            out.fov_degrees = std::stod(argv[++i]);
            if (!(out.fov_degrees > 1e-6 && out.fov_degrees < 179.0)) {
                error_message = "Invalid value for --fov: " + std::to_string(out.fov_degrees) +
                                " (expected 0 < DEG < 179)";
                return CliParseResult::Error;
            }
            out.fov_override = true;
        } else if (arg == "--aberration-speed" && i + 1 < argc) {
            out.aberration_speed_value = std::stod(argv[++i]);
            if (!(out.aberration_speed_value >= 0.0 && out.aberration_speed_value < 1.0)) {
                error_message = "Invalid value for --aberration-speed: " +
                                std::to_string(out.aberration_speed_value) +
                                " (expected 0.0 <= beta < 1.0)";
                return CliParseResult::Error;
            }
            out.settings.aberration_speed = out.aberration_speed_value;
            out.aberration_speed_override = true;
        } else if (arg == "--aberration-yaw-turns" && i + 1 < argc) {
            out.aberration_yaw_value = std::stod(argv[++i]);
            out.aberration_yaw_value = out.aberration_yaw_value - std::floor(out.aberration_yaw_value);
            out.settings.aberration_yaw_turns = out.aberration_yaw_value;
            out.aberration_yaw_override = true;
        } else if (arg == "--aberration-pitch-turns" && i + 1 < argc) {
            out.aberration_pitch_value = std::stod(argv[++i]);
            out.settings.aberration_pitch_turns = out.aberration_pitch_value;
            out.aberration_pitch_override = true;
        } else if (arg == "--cam-preset" && i + 1 < argc) {
            out.cam_preset_name = argv[++i];
            out.cam_preset_override = true;
        } else if (arg == "--adaptive" && i + 1 < argc) {
            out.adaptive_value = std::stoi(argv[++i]) != 0;
            out.settings.adaptive_sampling = out.adaptive_value;
            out.adaptive_override = true;
        } else if (arg == "--adaptive-min-spp" && i + 1 < argc) {
            out.adaptive_min_spp_value = std::stoi(argv[++i]);
            out.settings.adaptive_min_spp = out.adaptive_min_spp_value;
            out.adaptive_min_spp_override = true;
        } else if (arg == "--adaptive-threshold" && i + 1 < argc) {
            out.adaptive_threshold_value = std::stod(argv[++i]);
            out.settings.adaptive_threshold = out.adaptive_threshold_value;
            out.adaptive_threshold_override = true;
        } else if (arg == "--gltf-triangle-step" && i + 1 < argc) {
            out.settings.gltf_triangle_step = std::stoi(argv[++i]);
            if (out.settings.gltf_triangle_step < 1) {
                error_message = "Invalid value for --gltf-triangle-step: " +
                                std::to_string(out.settings.gltf_triangle_step) +
                                " (expected >= 1)";
                return CliParseResult::Error;
            }
        } else if (arg == "--materials-variant" && i + 1 < argc) {
            std::string v = argv[++i];
            if (!parse_materials_variant(v, out.settings.materials_variant)) {
                error_message = "Invalid value for --materials-variant: " + v +
                                " (expected " + materials_variant_expected_values() + ")";
                return CliParseResult::Error;
            }
        } else if (arg == "--big-scatter-palette" && i + 1 < argc) {
            std::string v = argv[++i];
            if (!parse_big_scatter_palette(v, out.settings.big_scatter_palette)) {
                error_message = "Invalid value for --big-scatter-palette: " + v +
                                " (expected " + big_scatter_palette_expected_values() + ")";
                return CliParseResult::Error;
            }
        } else if (arg == "--lighting-preset" && i + 1 < argc) {
            std::string v = argv[++i];
            if (!parse_lighting_preset(v, out.settings.lighting_preset)) {
                error_message = "Invalid value for --lighting-preset: " + v +
                                " (expected " + lighting_preset_expected_values() + ")";
                return CliParseResult::Error;
            }
        } else if (arg == "--lighting-sweep" && i + 1 < argc) {
            out.lighting_sweep_value = std::stoi(argv[++i]) != 0;
            out.lighting_sweep_override = true;
        } else if (arg == "--scatter-spacing" && i + 1 < argc) {
            out.scene_param_overrides.scatter_spacing = std::stod(argv[++i]);
            if (out.scene_param_overrides.scatter_spacing <= 0.0) {
                error_message = "Invalid value for --scatter-spacing: " +
                                std::to_string(out.scene_param_overrides.scatter_spacing) +
                                " (expected > 0.0)";
                return CliParseResult::Error;
            }
            out.scene_param_overrides.scatter_spacing_override = true;
        } else if (arg == "--scatter-max-radius" && i + 1 < argc) {
            out.scene_param_overrides.scatter_max_radius = std::stod(argv[++i]);
            if (out.scene_param_overrides.scatter_max_radius <= 0.0) {
                error_message = "Invalid value for --scatter-max-radius: " +
                                std::to_string(out.scene_param_overrides.scatter_max_radius) +
                                " (expected > 0.0)";
                return CliParseResult::Error;
            }
            out.scene_param_overrides.scatter_max_radius_override = true;
        } else if (arg == "--scatter-growth-scale" && i + 1 < argc) {
            out.scene_param_overrides.scatter_growth_scale = std::stod(argv[++i]);
            if (out.scene_param_overrides.scatter_growth_scale < 0.0) {
                error_message = "Invalid value for --scatter-growth-scale: " +
                                std::to_string(out.scene_param_overrides.scatter_growth_scale) +
                                " (expected >= 0.0)";
                return CliParseResult::Error;
            }
            out.scene_param_overrides.scatter_growth_scale_override = true;
        } else if (arg == "--big-scatter-spacing" && i + 1 < argc) {
            out.scene_param_overrides.big_scatter_spacing = std::stod(argv[++i]);
            if (out.scene_param_overrides.big_scatter_spacing <= 0.0) {
                error_message = "Invalid value for --big-scatter-spacing: " +
                                std::to_string(out.scene_param_overrides.big_scatter_spacing) +
                                " (expected > 0.0)";
                return CliParseResult::Error;
            }
            out.scene_param_overrides.big_scatter_spacing_override = true;
        } else if (arg == "--big-scatter-max-radius" && i + 1 < argc) {
            out.scene_param_overrides.big_scatter_max_radius = std::stod(argv[++i]);
            if (out.scene_param_overrides.big_scatter_max_radius <= 0.0) {
                error_message = "Invalid value for --big-scatter-max-radius: " +
                                std::to_string(out.scene_param_overrides.big_scatter_max_radius) +
                                " (expected > 0.0)";
                return CliParseResult::Error;
            }
            out.scene_param_overrides.big_scatter_max_radius_override = true;
        } else if (arg == "--row-scatter-xmax" && i + 1 < argc) {
            out.scene_param_overrides.row_scatter_xmax = std::stod(argv[++i]);
            if (out.scene_param_overrides.row_scatter_xmax <= 0.0) {
                error_message = "Invalid value for --row-scatter-xmax: " +
                                std::to_string(out.scene_param_overrides.row_scatter_xmax) +
                                " (expected > 0.0)";
                return CliParseResult::Error;
            }
            out.scene_param_overrides.row_scatter_xmax_override = true;
        } else if (arg == "--row-scatter-z-front" && i + 1 < argc) {
            out.scene_param_overrides.row_scatter_z_front = std::stod(argv[++i]);
            out.scene_param_overrides.row_scatter_z_front_override = true;
        } else if (arg == "--row-scatter-z-back" && i + 1 < argc) {
            out.scene_param_overrides.row_scatter_z_back = std::stod(argv[++i]);
            out.scene_param_overrides.row_scatter_z_back_override = true;
        } else if (arg == "--row-scatter-replacement-rate" && i + 1 < argc) {
            out.scene_param_overrides.row_scatter_replacement_rate = std::stod(argv[++i]);
            if (!(out.scene_param_overrides.row_scatter_replacement_rate >= 0.0 &&
                  out.scene_param_overrides.row_scatter_replacement_rate <= 1.0))
            {
                error_message = "Invalid value for --row-scatter-replacement-rate: " +
                                std::to_string(out.scene_param_overrides.row_scatter_replacement_rate) +
                                " (expected 0.0 <= R <= 1.0)";
                return CliParseResult::Error;
            }
            out.scene_param_overrides.row_scatter_replacement_rate_override = true;
        } else if (arg == "--threads" && i + 1 < argc) {
            out.thread_count_value = std::stoi(argv[++i]);
            if (out.thread_count_value < 1) {
                error_message = "Invalid value for --threads: " +
                                std::to_string(out.thread_count_value) +
                                " (expected >= 1)";
                return CliParseResult::Error;
            }
            out.settings.thread_count = out.thread_count_value;
            out.thread_count_override = true;
        } else if (arg == "--tile-size" && i + 1 < argc) {
            out.tile_size_value = std::stoi(argv[++i]);
            if (out.tile_size_value < 4) {
                error_message = "Invalid value for --tile-size: " +
                                std::to_string(out.tile_size_value) +
                                " (expected >= 4)";
                return CliParseResult::Error;
            }
            out.settings.tile_size = out.tile_size_value;
            out.tile_size_override = true;
        } else if (arg == "--aov-space" && i + 1 < argc) {
            std::string space = argv[++i];
            if (space == "world") {
                out.aov_camera_space_value = false;
                out.settings.aov_normal_camera_space = false;
            } else if (space == "camera") {
                out.aov_camera_space_value = true;
                out.settings.aov_normal_camera_space = true;
            } else {
                error_message = "Invalid value for --aov-space: " + space + " (expected world|camera)";
                return CliParseResult::Error;
            }
            out.aov_space_override = true;
        } else if (arg == "--ppm-denoised-only" && i + 1 < argc) {
            out.ppm_denoised_only_value = std::stoi(argv[++i]) != 0;
            out.settings.ppm_denoised_only = out.ppm_denoised_only_value;
            out.ppm_denoised_only_override = true;
        } else if (arg == "--sss-random-walk" && i + 1 < argc) {
            out.sss_random_walk_value = std::stoi(argv[++i]) != 0;
            out.settings.sss_random_walk = out.sss_random_walk_value;
            out.sss_random_walk_override = true;
        } else if (arg == "--atmosphere" && i + 1 < argc) {
            out.atmosphere_value = std::stoi(argv[++i]) != 0;
            out.settings.atmosphere_enabled = out.atmosphere_value;
            out.atmosphere_override = true;
        } else if (arg == "--atmosphere-density" && i + 1 < argc) {
            out.atmosphere_density_value = std::stod(argv[++i]);
            if (out.atmosphere_density_value < 0.0) {
                error_message = "Invalid value for --atmosphere-density: " +
                                std::to_string(out.atmosphere_density_value) +
                                " (expected >= 0.0)";
                return CliParseResult::Error;
            }
            out.settings.atmosphere_density = out.atmosphere_density_value;
            out.atmosphere_density_override = true;
        } else if (arg == "--atmosphere-color" && i + 1 < argc) {
            if (!parse_vec3_csv(argv[++i], out.atmosphere_color_value)) {
                error_message = "Invalid value for --atmosphere-color (expected R,G,B)";
                return CliParseResult::Error;
            }
            out.settings.atmosphere_color = out.atmosphere_color_value;
            out.atmosphere_color_override = true;
        } else if (arg == "--denoise" && i + 1 < argc) {
            out.denoise_value = std::stoi(argv[++i]) != 0;
            out.settings.denoise = out.denoise_value;
            out.denoise_override = true;
        } else if (arg == "--compare" && i + 2 < argc) {
            out.compare_mode = true;
            out.compare_ref = argv[++i];
            out.compare_candidate = argv[++i];
        } else if (arg == "--rmse-threshold" && i + 1 < argc) {
            out.rmse_threshold = std::stod(argv[++i]);
        } else if (arg == "--ssim-threshold" && i + 1 < argc) {
            out.ssim_threshold = std::stod(argv[++i]);
        } else if (arg == "--help") {
            return CliParseResult::Help;
        } else if (arg == "--print-options-schema") {
            return CliParseResult::PrintOptionsSchema;
        } else if (arg == "--print-scene-registry") {
            return CliParseResult::PrintSceneRegistry;
        } else {
            error_message = "Unknown or incomplete argument: " + arg;
            show_usage_on_error = true;
            return CliParseResult::Error;
        }
    }

    return CliParseResult::Ok;
}
