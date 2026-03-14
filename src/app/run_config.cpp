#include "app/run_config.h"

#include <cmath>
#include <filesystem>
#include <string>

#include "render/camera_controls.h"
#include "render/output_naming.h"

namespace {

constexpr double kPi = 3.14159265358979323846;

} // namespace

bool apply_run_configuration(const ParsedArgs& args,
                             RenderSettings& settings,
                             SceneBuild& scene,
                             std::string& output_base,
                             std::string& error_message)
{
    error_message.clear();

    // Scene presets may set defaults; explicit CLI values take precedence.
    if (args.width_override) settings.image_width = args.width_value;
    if (args.height_override) settings.image_height = args.height_value;
    if (args.spp_override) settings.samples_per_pixel = args.spp_value;
    if (args.max_depth_override) settings.max_depth = args.max_depth_value;
    if (args.aperture_override) settings.aperture = args.aperture_value;
    if (args.focus_distance_override) settings.focus_distance = args.focus_distance_value;
    if (args.exposure_override) settings.exposure = args.exposure_value;
    if (args.adaptive_override) settings.adaptive_sampling = args.adaptive_value;
    if (args.adaptive_min_spp_override) settings.adaptive_min_spp = args.adaptive_min_spp_value;
    if (args.adaptive_threshold_override) settings.adaptive_threshold = args.adaptive_threshold_value;
    if (args.aov_space_override) settings.aov_normal_camera_space = args.aov_camera_space_value;
    if (args.denoise_override) settings.denoise = args.denoise_value;
    if (args.ppm_denoised_only_override) settings.ppm_denoised_only = args.ppm_denoised_only_value;
    if (args.sss_random_walk_override) settings.sss_random_walk = args.sss_random_walk_value;
    if (args.atmosphere_override) settings.atmosphere_enabled = args.atmosphere_value;
    if (args.atmosphere_density_override) settings.atmosphere_density = args.atmosphere_density_value;
    if (args.atmosphere_color_override) settings.atmosphere_color = args.atmosphere_color_value;
    if (args.tile_size_override) settings.tile_size = args.tile_size_value;
    if (args.thread_count_override) settings.thread_count = args.thread_count_value;
    if (args.aberration_speed_override) settings.aberration_speed = args.aberration_speed_value;
    if (args.aberration_yaw_override) settings.aberration_yaw_turns = args.aberration_yaw_value;
    if (args.aberration_pitch_override) settings.aberration_pitch_turns = args.aberration_pitch_value;

    output_base = args.output_base;
    if (!args.output_base_override) {
        OutputNameTags tags;
        tags.aberration_speed = settings.aberration_speed;
        tags.aberration_pitch = settings.aberration_pitch_turns;
        tags.aberration_yaw = settings.aberration_yaw_turns;
        tags.camera_pitch = args.cam_pitch_override ? args.cam_pitch_turns_value : 0.0;
        tags.camera_yaw = args.cam_yaw_override ? args.cam_yaw_turns_value : 0.0;
        output_base = make_default_output_base(args.scene_name, tags);
    }

    {
        std::error_code ec;
        std::filesystem::path out_path(output_base);
        const std::filesystem::path parent = out_path.parent_path();
        if (!parent.empty() &&
            !std::filesystem::exists(parent, ec) &&
            !std::filesystem::create_directories(parent, ec))
        {
            error_message = "Failed to create output directory: " + parent.string();
            return false;
        }
    }

    if (args.cam_preset_override) {
        CameraPreset preset;
        if (!lookup_camera_preset(args.cam_preset_name, preset)) {
            error_message = "Invalid value for --cam-preset: " + args.cam_preset_name +
                            " (expected nave|side|overview)";
            return false;
        }
        constexpr double kPi = 3.14159265358979323846;
        scene.camera.origin = preset.origin;
        scene.camera.forward = (preset.lookat - preset.origin).normalize();
        scene.camera.half_fov_x = (preset.fov_degrees * kPi / 180.0) * 0.5;
    }

    if (args.cam_origin_override)
        scene.camera.origin = args.cam_origin_value;
    if (args.cam_x_override)
        scene.camera.origin.x = args.cam_x_value;
    if (args.cam_y_override)
        scene.camera.origin.y = args.cam_y_value;
    if (args.cam_z_override)
        scene.camera.origin.z = args.cam_z_value;

    if (args.cam_forward_override) {
        if (args.cam_forward_value.norm_squared() <= 1e-12) {
            error_message = "--cam-forward cannot be zero vector";
            return false;
        }
        scene.camera.forward = args.cam_forward_value.normalize();
    } else if (args.cam_lookat_override) {
        Vec3 to_target = args.cam_lookat_value - scene.camera.origin;
        if (to_target.norm_squared() <= 1e-12) {
            error_message = "--cam-lookat equals camera origin; direction undefined";
            return false;
        }
        scene.camera.forward = to_target.normalize();
    }

    if (args.fov_override) {
        scene.camera.half_fov_x = (args.fov_degrees * kPi / 180.0) * 0.5;
    }

    if (args.cam_yaw_override || args.cam_pitch_override) {
        const double yaw_turns = args.cam_yaw_override ? args.cam_yaw_turns_value : 0.0;
        const double pitch_turns = args.cam_pitch_override ? args.cam_pitch_turns_value : 0.0;
        scene.camera.forward = apply_camera_yaw_pitch_turns(scene.camera.forward,
                                                            scene.camera.world_up,
                                                            yaw_turns,
                                                            pitch_turns);
    }

    return true;
}
