#include "app/arg_parsers.h"

#include <unordered_map>

#include "render/settings.h"

namespace {

const std::unordered_map<std::string, int>& materials_variant_map() {
    static const std::unordered_map<std::string, int> kMap = {
        {"all", RenderSettings::MaterialsAll},
        {"aniso-gold", RenderSettings::MaterialsAnisoGold},
        {"aniso-steel", RenderSettings::MaterialsAnisoSteel},
        {"clearcoat", RenderSettings::MaterialsClearcoat},
        {"chrome", RenderSettings::MaterialsChrome},
        {"glass", RenderSettings::MaterialsGlass},
        {"thin", RenderSettings::MaterialsThinTransmission},
        {"sheen", RenderSettings::MaterialsSheen},
        {"conductor-copper", RenderSettings::MaterialsConductorCopper},
        {"conductor-aluminum", RenderSettings::MaterialsConductorAluminum},
        {"coated-plastic", RenderSettings::MaterialsCoatedPlastic},
    };
    return kMap;
}

const std::unordered_map<std::string, int>& big_scatter_palette_map() {
    static const std::unordered_map<std::string, int> kMap = {
        {"balanced", RenderSettings::BigScatterPaletteBalanced},
        {"vivid", RenderSettings::BigScatterPaletteVivid},
        {"earthy", RenderSettings::BigScatterPaletteEarthy},
    };
    return kMap;
}

const std::unordered_map<std::string, int>& lighting_preset_map() {
    static const std::unordered_map<std::string, int> kMap = {
        {"scene-default", RenderSettings::LightingPresetSceneDefault},
        {"scatter-default", RenderSettings::LightingPresetScatterDefault},
        {"big-scatter-default", RenderSettings::LightingPresetBigScatterDefault},
        {"row-scatter-default", RenderSettings::LightingPresetRowScatterDefault},
        {"materials-studio", RenderSettings::LightingPresetMaterialsStudio},
        {"cornell-ceiling", RenderSettings::LightingPresetCornellCeiling},
        {"cornell-ceiling-plus-key", RenderSettings::LightingPresetCornellCeilingPlusKey},
        {"benchmark-softbox", RenderSettings::LightingPresetBenchmarkSoftbox},
        {"volumes-backlit", RenderSettings::LightingPresetVolumesBacklit},
    };
    return kMap;
}

} // namespace

bool parse_materials_variant(const std::string& value,
                             int& out_materials_variant)
{
    const auto& map = materials_variant_map();
    const auto it = map.find(value);
    if (it == map.end())
        return false;

    out_materials_variant = it->second;
    return true;
}

const char* materials_variant_expected_values() {
    return "all|aniso-gold|aniso-steel|clearcoat|chrome|glass|thin|sheen|conductor-copper|conductor-aluminum|coated-plastic";
}

bool parse_big_scatter_palette(const std::string& value,
                               int& out_big_scatter_palette)
{
    const auto& map = big_scatter_palette_map();
    const auto it = map.find(value);
    if (it == map.end())
        return false;

    out_big_scatter_palette = it->second;
    return true;
}

const char* big_scatter_palette_expected_values() {
    return "balanced|vivid|earthy";
}

bool parse_lighting_preset(const std::string& value,
                           int& out_lighting_preset)
{
    const auto& map = lighting_preset_map();
    const auto it = map.find(value);
    if (it == map.end())
        return false;

    out_lighting_preset = it->second;
    return true;
}

const char* lighting_preset_expected_values() {
    return "scene-default|scatter-default|big-scatter-default|row-scatter-default|materials-studio|cornell-ceiling|cornell-ceiling-plus-key|benchmark-softbox|volumes-backlit";
}
