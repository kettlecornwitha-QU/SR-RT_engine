#include "app/options_schema.h"

#include <sstream>

#include "render/settings.h"
#include "scene/presets/scene_params.h"

namespace {

std::string bool_json(bool v) {
    return v ? "true" : "false";
}

std::string big_scatter_palette_name(int v) {
    switch (v) {
        case RenderSettings::BigScatterPaletteBalanced:
            return "balanced";
        case RenderSettings::BigScatterPaletteVivid:
            return "vivid";
        case RenderSettings::BigScatterPaletteEarthy:
            return "earthy";
        default:
            return "balanced";
    }
}

std::string lighting_preset_name(int v) {
    switch (v) {
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

} // namespace

std::string make_options_schema_json() {
    constexpr int kOptionsSchemaVersion = 1;
    const RenderSettings rs{};
    const ScatterParams sp = make_scatter_params();
    const BigScatterParams bsp = make_big_scatter_params();
    const RowScatterParams rsp = make_row_scatter_params();

    std::ostringstream os;
    os << "{";
    os << "\"schema_version\":" << kOptionsSchemaVersion << ",";
    os << "\"render_defaults\":{";
    os << "\"width\":" << rs.image_width << ",";
    os << "\"height\":" << rs.image_height << ",";
    os << "\"spp\":" << rs.samples_per_pixel << ",";
    os << "\"max_depth\":" << rs.max_depth << ",";
    os << "\"adaptive\":" << bool_json(rs.adaptive_sampling) << ",";
    os << "\"adaptive_min_spp\":" << rs.adaptive_min_spp << ",";
    os << "\"adaptive_threshold\":" << rs.adaptive_threshold << ",";
    os << "\"denoise\":" << bool_json(rs.denoise) << ",";
    os << "\"ppm_denoised_only\":" << bool_json(rs.ppm_denoised_only) << ",";
    os << "\"atmosphere_enabled\":" << bool_json(rs.atmosphere_enabled) << ",";
    os << "\"atmosphere_density\":" << rs.atmosphere_density << ",";
    os << "\"atmosphere_color\":[" << rs.atmosphere_color.x << "," << rs.atmosphere_color.y << "," << rs.atmosphere_color.z << "]";
    os << "},";

    os << "\"scene_override_defaults\":{";
    os << "\"scatter_spacing\":" << sp.spacing << ",";
    os << "\"scatter_max_radius\":" << sp.max_field_radius << ",";
    os << "\"scatter_growth_scale\":" << sp.radius_growth_scale << ",";
    os << "\"big_scatter_spacing\":" << bsp.spacing << ",";
    os << "\"big_scatter_max_radius\":" << bsp.max_field_radius << ",";
    os << "\"row_scatter_xmax\":" << rsp.x_max << ",";
    os << "\"row_scatter_z_front\":" << rsp.z_front << ",";
    os << "\"row_scatter_z_back\":" << rsp.z_back << ",";
    os << "\"row_scatter_replacement_rate\":" << rsp.replacement_rate;
    os << "},";

    os << "\"choices\":{";
    os << "\"big_scatter_palette\":[\"balanced\",\"vivid\",\"earthy\"],";
    os << "\"lighting_preset\":[";
    os << "\"scene-default\",";
    os << "\"scatter-default\",";
    os << "\"big-scatter-default\",";
    os << "\"row-scatter-default\",";
    os << "\"materials-studio\",";
    os << "\"cornell-ceiling\",";
    os << "\"cornell-ceiling-plus-key\",";
    os << "\"benchmark-softbox\",";
    os << "\"volumes-backlit\"";
    os << "],";
    os << "\"video_fps\":[24,30,60]";
    os << "},";

    os << "\"defaults\":{";
    os << "\"big_scatter_palette\":\"" << big_scatter_palette_name(rs.big_scatter_palette) << "\",";
    os << "\"lighting_preset\":\"" << lighting_preset_name(rs.lighting_preset) << "\",";
    os << "\"video_keep_frames\":false";
    os << "}";
    os << "}";

    return os.str();
}
