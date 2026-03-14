#include "render/output_pipeline.h"

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <vector>

#include "math/vec3.h"
#include "render/output.h"

OutputPolicy make_output_policy(const RenderSettings& settings) {
    OutputPolicy policy;
    policy.write_exr = true;
    policy.write_png = true;
    policy.write_ppm = !settings.ppm_denoised_only;
    policy.write_aovs = true;
    policy.run_denoise = settings.denoise;
    return policy;
}

OutputArtifacts run_output_pipeline(const RenderResult& render,
                                    const RenderSettings& settings,
                                    const std::string& output_base,
                                    const OutputPolicy& policy,
                                    std::ostream& log_out,
                                    std::ostream& log_err)
{
    OutputArtifacts out;

    out.hdr_path = output_base + ".exr";
    out.albedo_exr_path = output_base + "_albedo.exr";
    out.normal_exr_path = output_base + "_normal.exr";
    out.ldr_png_path = output_base + ".png";
    out.ldr_ppm_path = output_base + ".ppm";
    out.albedo_png_path = output_base + "_albedo.png";
    out.normal_png_path = output_base + "_normal.png";
    out.albedo_ppm_path = output_base + "_albedo.ppm";
    out.normal_ppm_path = output_base + "_normal.ppm";

    if (policy.write_exr) {
        out.wrote_exr = write_exr_if_available(out.hdr_path,
                                               render.framebuffer,
                                               settings.image_width,
                                               settings.image_height);
        if (policy.write_aovs) {
            out.wrote_albedo_exr = write_exr_if_available(out.albedo_exr_path,
                                                          render.albedo_aov,
                                                          settings.image_width,
                                                          settings.image_height);
            out.wrote_normal_exr = write_exr_if_available(out.normal_exr_path,
                                                          render.normal_aov,
                                                          settings.image_width,
                                                          settings.image_height);
        }
    }

    if (!out.wrote_exr)
        log_out << "EXR output unavailable (missing tinyexr.h), skipping " << out.hdr_path << "\n";

    std::vector<unsigned char> ldr =
        make_ldr_buffer(render.framebuffer,
                        settings.image_width,
                        settings.image_height,
                        settings.exposure);

    std::vector<unsigned char> ldr_albedo;
    std::vector<unsigned char> ldr_normal;
    if (policy.write_aovs) {
        ldr_albedo = make_aov_ldr_buffer(render.albedo_aov,
                                         settings.image_width,
                                         settings.image_height,
                                         false);
        ldr_normal = make_aov_ldr_buffer(render.normal_aov,
                                         settings.image_width,
                                         settings.image_height,
                                         true);
    }

    if (policy.write_png) {
        out.wrote_png = write_png_if_available(out.ldr_png_path,
                                               ldr,
                                               settings.image_width,
                                               settings.image_height);
        if (policy.write_aovs) {
            out.wrote_albedo_png = write_png_if_available(out.albedo_png_path,
                                                          ldr_albedo,
                                                          settings.image_width,
                                                          settings.image_height);
            out.wrote_normal_png = write_png_if_available(out.normal_png_path,
                                                          ldr_normal,
                                                          settings.image_width,
                                                          settings.image_height);
        }
    }

    if (policy.write_ppm) {
        out.wrote_ppm = write_ppm(out.ldr_ppm_path,
                                  ldr,
                                  settings.image_width,
                                  settings.image_height);
        if (policy.write_aovs) {
            out.wrote_albedo_ppm = write_ppm(out.albedo_ppm_path,
                                             ldr_albedo,
                                             settings.image_width,
                                             settings.image_height);
            out.wrote_normal_ppm = write_ppm(out.normal_ppm_path,
                                             ldr_normal,
                                             settings.image_width,
                                             settings.image_height);
        }

        if (!out.wrote_ppm)
            log_err << "Failed to write PPM output: " << out.ldr_ppm_path << "\n";
        if (policy.write_aovs && !out.wrote_albedo_ppm)
            log_err << "Failed to write albedo AOV PPM output: " << out.albedo_ppm_path << "\n";
        if (policy.write_aovs && !out.wrote_normal_ppm)
            log_err << "Failed to write normal AOV PPM output: " << out.normal_ppm_path << "\n";
    }

    if (policy.run_denoise) {
        out.denoise_attempted = true;
        std::vector<Vec3> denoised_framebuffer;
        out.denoise_succeeded = denoise_oidn_if_available(render.framebuffer,
                                                          render.albedo_aov,
                                                          render.normal_aov,
                                                          settings.image_width,
                                                          settings.image_height,
                                                          denoised_framebuffer,
                                                          out.denoise_message);

        if (!out.denoise_succeeded) {
            log_out << "Denoise skipped: " << out.denoise_message << "\n";
        } else {
            out.den_hdr_path = output_base + "_denoised.exr";
            out.den_png_path = output_base + "_denoised.png";
            out.den_ppm_path = output_base + "_denoised.ppm";

            out.wrote_den_exr = policy.write_exr &&
                                write_exr_if_available(out.den_hdr_path,
                                                       denoised_framebuffer,
                                                       settings.image_width,
                                                       settings.image_height);

            std::vector<unsigned char> den_ldr =
                make_ldr_buffer(denoised_framebuffer,
                                settings.image_width,
                                settings.image_height,
                                settings.exposure);

            out.wrote_den_png = policy.write_png &&
                                write_png_if_available(out.den_png_path,
                                                       den_ldr,
                                                       settings.image_width,
                                                       settings.image_height);

            out.wrote_den_ppm = write_ppm(out.den_ppm_path,
                                          den_ldr,
                                          settings.image_width,
                                          settings.image_height);

            if (!out.wrote_den_ppm)
                log_err << "Failed to write denoised PPM output: " << out.den_ppm_path << "\n";

            log_out << "Denoised output";
            if (out.wrote_den_exr)
                log_out << " | HDR: " << out.den_hdr_path;
            if (out.wrote_den_png)
                log_out << " | LDR: " << out.den_png_path;
            else
                log_out << " | LDR: " << out.den_ppm_path;
            log_out << "\n";
        }
    }

    return out;
}

std::string format_render_summary(const std::string& scene_name,
                                  const RenderResult& render,
                                  const RenderSettings& settings,
                                  const OutputArtifacts& outputs)
{
    std::ostringstream oss;
    oss << "Rendered scene '" << scene_name << "' in "
        << render.elapsed_seconds << "s using " << render.thread_count << " threads";

    const double primary_samples = static_cast<double>(render.total_primary_samples);
    oss << std::defaultfloat
        << " | " << std::setprecision(3)
        << (primary_samples / std::max(1e-8, render.elapsed_seconds) / 1e6)
        << " M primary samples/s";

    if (outputs.wrote_exr)
        oss << " | HDR: " << outputs.hdr_path;
    if (outputs.wrote_albedo_exr)
        oss << " | AOV Albedo HDR: " << outputs.albedo_exr_path;
    if (outputs.wrote_normal_exr)
        oss << " | AOV Normal HDR: " << outputs.normal_exr_path;
    if (outputs.wrote_png)
        oss << " | LDR: " << outputs.ldr_png_path;
    if (outputs.wrote_albedo_png)
        oss << " | AOV Albedo: " << outputs.albedo_png_path;
    else if (outputs.wrote_albedo_ppm)
        oss << " | AOV Albedo: " << outputs.albedo_ppm_path;
    if (outputs.wrote_normal_png)
        oss << " | AOV Normal: " << outputs.normal_png_path;
    else if (outputs.wrote_normal_ppm)
        oss << " | AOV Normal: " << outputs.normal_ppm_path;
    if (outputs.wrote_ppm)
        oss << " | REF/COMPARE: " << outputs.ldr_ppm_path;
    else if (settings.ppm_denoised_only)
        oss << " | REF/COMPARE: (suppressed by --ppm-denoised-only)";

    return oss.str();
}
