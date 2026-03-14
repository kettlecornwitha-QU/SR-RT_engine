#pragma once

#include <iosfwd>
#include <string>

#include "render/renderer.h"
#include "render/settings.h"

struct OutputPolicy {
    bool write_exr = true;
    bool write_png = true;
    bool write_ppm = true;
    bool write_aovs = true;
    bool run_denoise = false;
};

struct OutputArtifacts {
    std::string hdr_path;
    std::string albedo_exr_path;
    std::string normal_exr_path;
    std::string ldr_png_path;
    std::string ldr_ppm_path;
    std::string albedo_png_path;
    std::string normal_png_path;
    std::string albedo_ppm_path;
    std::string normal_ppm_path;

    std::string den_hdr_path;
    std::string den_png_path;
    std::string den_ppm_path;

    bool wrote_exr = false;
    bool wrote_albedo_exr = false;
    bool wrote_normal_exr = false;
    bool wrote_png = false;
    bool wrote_albedo_png = false;
    bool wrote_normal_png = false;
    bool wrote_ppm = false;
    bool wrote_albedo_ppm = false;
    bool wrote_normal_ppm = false;

    bool denoise_attempted = false;
    bool denoise_succeeded = false;
    std::string denoise_message;
    bool wrote_den_exr = false;
    bool wrote_den_png = false;
    bool wrote_den_ppm = false;
};

OutputPolicy make_output_policy(const RenderSettings& settings);

OutputArtifacts run_output_pipeline(const RenderResult& render,
                                    const RenderSettings& settings,
                                    const std::string& output_base,
                                    const OutputPolicy& policy,
                                    std::ostream& log_out,
                                    std::ostream& log_err);

std::string format_render_summary(const std::string& scene_name,
                                  const RenderResult& render,
                                  const RenderSettings& settings,
                                  const OutputArtifacts& outputs);
