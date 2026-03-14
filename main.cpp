#include <iostream>
#include <cmath>
#include <vector>
#include <limits>
#include <thread>
#include <atomic>
#include <algorithm>
#include <cstdint>
#include <cctype>
#include <string>
#include <filesystem>

#include "app/cli.h"
#include "app/options_schema.h"
#include "app/run_config.h"
#include "math/vec3.h"
#include "core/hit.h"
#include "render/output.h"
#include "render/output_pipeline.h"
#include "render/quality.h"
#include "render/renderer.h"
#include "render/settings.h"
#include "scene/scene_builder.h"
#include "scene/scene_registry.h"

// ======================================================
// MAIN
// ======================================================

namespace {

const char* lighting_preset_cli_name(int preset) {
    switch (preset) {
        case RenderSettings::LightingPresetSceneDefault:
            return "scene-default";
        case RenderSettings::LightingPresetScatterDefault:
            return "scatter-default";
        case RenderSettings::LightingPresetBigScatterDefault:
            return "big-scatter-default";
        case RenderSettings::LightingPresetRowScatterDefault:
            return "row-scatter-default";
        case RenderSettings::LightingPresetMaterialsStudio:
            return "materials-studio";
        case RenderSettings::LightingPresetCornellCeiling:
            return "cornell-ceiling";
        case RenderSettings::LightingPresetCornellCeilingPlusKey:
            return "cornell-ceiling-plus-key";
        case RenderSettings::LightingPresetBenchmarkSoftbox:
            return "benchmark-softbox";
        case RenderSettings::LightingPresetVolumesBacklit:
            return "volumes-backlit";
        default:
            return "scene-default";
    }
}

std::string sanitize_token_for_filename(std::string token) {
    for (char& c : token) {
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-'))
            c = '_';
        if (c == '-')
            c = '_';
    }
    return token;
}

bool ensure_output_parent_exists(const std::string& output_base, std::string& error) {
    std::error_code ec;
    std::filesystem::path out_path(output_base);
    const std::filesystem::path parent = out_path.parent_path();
    if (!parent.empty() &&
        !std::filesystem::exists(parent, ec) &&
        !std::filesystem::create_directories(parent, ec))
    {
        error = "Failed to create output directory: " + parent.string();
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    ParsedArgs args;
    std::string cli_error;
    bool show_usage_on_error = false;
    CliParseResult cli_result = parse_cli_args(argc, argv, args, cli_error, show_usage_on_error);
    if (cli_result == CliParseResult::Help) {
        print_usage();
        return 0;
    }
    if (cli_result == CliParseResult::PrintOptionsSchema) {
        std::cout << make_options_schema_json() << "\n";
        return 0;
    }
    if (cli_result == CliParseResult::PrintSceneRegistry) {
        std::cout << make_scene_registry_json() << "\n";
        return 0;
    }
    if (cli_result == CliParseResult::Error) {
        std::cerr << cli_error << "\n";
        if (show_usage_on_error)
            print_usage();
        return 1;
    }

    std::string scene_name = args.scene_name;

    if (args.compare_mode) {
        LdrImage ref, cand;
        if (!load_ppm(args.compare_ref, ref)) {
            std::cerr << "Failed to read reference image: " << args.compare_ref << "\n";
            return 1;
        }
        if (!load_ppm(args.compare_candidate, cand)) {
            std::cerr << "Failed to read candidate image: " << args.compare_candidate << "\n";
            return 1;
        }

        double rmse = compute_rmse(ref, cand);
        double ssim = compute_ssim_luma(ref, cand);
        std::cout << "RMSE: " << rmse << "\n";
        std::cout << "SSIM (luma): " << ssim << "\n";

        bool pass = true;
        if (args.rmse_threshold >= 0.0)
            pass = pass && (rmse <= args.rmse_threshold);
        if (args.ssim_threshold >= 0.0)
            pass = pass && (ssim >= args.ssim_threshold);

        if (!pass) {
            std::cerr << "Comparison failed thresholds.\n";
            return 2;
        }
        return 0;
    }

    auto render_single = [&](const ParsedArgs& run_args,
                             const std::string& run_scene_name,
                             const std::string& display_scene_name,
                             const std::string& output_suffix) -> int
    {
        SceneBuild scene;
        RenderSettings run_settings = run_args.settings;
        std::string run_output_base = run_args.output_base;

        if (!build_scene_by_name(run_scene_name, scene, run_settings, run_args.scene_param_overrides)) {
            std::cerr << "Unknown scene: " << run_scene_name << "\n";
            print_usage();
            return 1;
        }

        std::string run_config_error;
        if (!apply_run_configuration(run_args, run_settings, scene, run_output_base, run_config_error)) {
            std::cerr << run_config_error << "\n";
            return 1;
        }

        if (!output_suffix.empty()) {
            run_output_base += output_suffix;
            if (!ensure_output_parent_exists(run_output_base, run_config_error)) {
                std::cerr << run_config_error << "\n";
                return 1;
            }
        }

        scene.world.build_bvh();
        RenderResult render = render_scene(scene, run_settings);
        OutputPolicy output_policy = make_output_policy(run_settings);
        OutputArtifacts outputs = run_output_pipeline(render,
                                                      run_settings,
                                                      run_output_base,
                                                      output_policy,
                                                      std::cout,
                                                      std::cerr);

        std::cout << format_render_summary(display_scene_name, render, run_settings, outputs) << "\n";
        return 0;
    };

    if (args.lighting_sweep_value) {
        const int presets[] = {
            RenderSettings::LightingPresetSceneDefault,
            RenderSettings::LightingPresetScatterDefault,
            RenderSettings::LightingPresetBigScatterDefault,
            RenderSettings::LightingPresetRowScatterDefault,
            RenderSettings::LightingPresetMaterialsStudio,
            RenderSettings::LightingPresetCornellCeiling,
            RenderSettings::LightingPresetCornellCeilingPlusKey,
            RenderSettings::LightingPresetBenchmarkSoftbox,
            RenderSettings::LightingPresetVolumesBacklit
        };

        for (int preset : presets) {
            ParsedArgs run_args = args;
            run_args.settings.lighting_preset = preset;
            const std::string preset_name = lighting_preset_cli_name(preset);
            const std::string output_suffix = "__L_" + sanitize_token_for_filename(preset_name);
            const std::string display_name = scene_name + " [lighting=" + preset_name + "]";
            const int rc = render_single(run_args, scene_name, display_name, output_suffix);
            if (rc != 0)
                return rc;
        }
        return 0;
    }

    return render_single(args, scene_name, scene_name, "");
}
