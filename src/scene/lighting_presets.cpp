#include "scene/lighting_presets.h"

#include "render/settings.h"

const char* lighting_preset_name(LightingPresetId id) {
    switch (id) {
        case LightingPresetId::ScatterDefault:
            return "scatter_default";
        case LightingPresetId::BigScatterDefault:
            return "big_scatter_default";
        case LightingPresetId::RowScatterDefault:
            return "row_scatter_default";
        case LightingPresetId::MaterialsStudio:
            return "materials_studio";
        case LightingPresetId::CornellCeiling:
            return "cornell_ceiling";
        case LightingPresetId::CornellCeilingPlusKey:
            return "cornell_ceiling_plus_key";
        case LightingPresetId::BenchmarkSoftbox:
            return "benchmark_softbox";
        case LightingPresetId::VolumesBacklit:
            return "volumes_backlit";
    }
    return "unknown";
}

LightingPresetId resolve_lighting_preset(int requested_preset,
                                         LightingPresetId scene_default)
{
    switch (requested_preset) {
        case RenderSettings::LightingPresetScatterDefault:
            return LightingPresetId::ScatterDefault;
        case RenderSettings::LightingPresetBigScatterDefault:
            return LightingPresetId::BigScatterDefault;
        case RenderSettings::LightingPresetRowScatterDefault:
            return LightingPresetId::RowScatterDefault;
        case RenderSettings::LightingPresetMaterialsStudio:
            return LightingPresetId::MaterialsStudio;
        case RenderSettings::LightingPresetCornellCeiling:
            return LightingPresetId::CornellCeiling;
        case RenderSettings::LightingPresetCornellCeilingPlusKey:
            return LightingPresetId::CornellCeilingPlusKey;
        case RenderSettings::LightingPresetBenchmarkSoftbox:
            return LightingPresetId::BenchmarkSoftbox;
        case RenderSettings::LightingPresetVolumesBacklit:
            return LightingPresetId::VolumesBacklit;
        case RenderSettings::LightingPresetSceneDefault:
        default:
            return scene_default;
    }
}

void append_lighting_preset(std::vector<PointLight>& point_lights,
                            std::vector<RectAreaLight>& area_lights,
                            LightingPresetId id)
{
    switch (id) {
        case LightingPresetId::ScatterDefault:
            point_lights.push_back({Vec3{-4.0, 3.0, -3.0}, Vec3{40.0, 40.0, 38.0}});
            point_lights.push_back({Vec3{4.0, 3.0, -3.0}, Vec3{40.0, 40.0, 38.0}});
            point_lights.push_back({Vec3{-4.0, 3.0, 3.0}, Vec3{24.0, 24.0, 24.0}});
            point_lights.push_back({Vec3{4.0, 3.0, 3.0}, Vec3{24.0, 24.0, 24.0}});
            area_lights.push_back({
                Vec3{0.0, 4.8, -6.0},
                Vec3{3.6, 0.0, 0.0},
                Vec3{0.0, 0.0, 2.2},
                Vec3{12.0, 12.0, 11.5}
            });
            return;
        case LightingPresetId::BigScatterDefault:
            point_lights.push_back({Vec3{-18.0, 16.0, -12.0}, Vec3{120.0, 115.0, 105.0}});
            point_lights.push_back({Vec3{18.0, 16.0, -12.0}, Vec3{120.0, 115.0, 105.0}});
            point_lights.push_back({Vec3{-20.0, 14.0, 14.0}, Vec3{75.0, 75.0, 72.0}});
            point_lights.push_back({Vec3{20.0, 14.0, 14.0}, Vec3{75.0, 75.0, 72.0}});
            area_lights.push_back({
                Vec3{0.0, 26.0, -8.0},
                Vec3{14.0, 0.0, 0.0},
                Vec3{0.0, 0.0, 10.0},
                Vec3{35.0, 34.0, 32.0}
            });
            return;
        case LightingPresetId::RowScatterDefault:
            point_lights.push_back({Vec3{-7.0, 6.0, -8.0}, Vec3{70.0, 68.0, 65.0}});
            point_lights.push_back({Vec3{7.0, 6.0, -8.0}, Vec3{70.0, 68.0, 65.0}});
            area_lights.push_back({
                Vec3{0.0, 8.0, -16.0},
                Vec3{10.0, 0.0, 0.0},
                Vec3{0.0, 0.0, 8.0},
                Vec3{18.0, 17.0, 16.0}
            });
            return;
        case LightingPresetId::MaterialsStudio:
            point_lights.push_back({Vec3{-2.5, 2.2, -2.0}, Vec3{45.0, 43.0, 40.0}});
            point_lights.push_back({Vec3{2.5, 2.2, -2.0}, Vec3{45.0, 43.0, 40.0}});
            area_lights.push_back({
                Vec3{0.0, 2.8, -3.6},
                Vec3{2.4, 0.0, 0.0},
                Vec3{0.0, 0.0, 1.2},
                Vec3{14.0, 13.0, 12.0}
            });
            return;
        case LightingPresetId::CornellCeiling:
            area_lights.push_back({
                Vec3{0.0, 1.99, -0.2},
                Vec3{0.6, 0.0, 0.0},
                Vec3{0.0, 0.0, 0.6},
                Vec3{13.0, 13.0, 13.0}
            });
            return;
        case LightingPresetId::CornellCeilingPlusKey:
            area_lights.push_back({
                Vec3{0.0, 1.99, -0.2},
                Vec3{0.6, 0.0, 0.0},
                Vec3{0.0, 0.0, 0.6},
                Vec3{13.0, 13.0, 13.0}
            });
            point_lights.push_back({Vec3{-7.0, 6.0, -8.0}, Vec3{70.0, 68.0, 65.0}});
            return;
        case LightingPresetId::BenchmarkSoftbox:
            area_lights.push_back({
                Vec3{0.0, 3.0, -5.0},
                Vec3{3.0, 0.0, 0.0},
                Vec3{0.0, 0.0, 3.0},
                Vec3{12.0, 12.0, 12.0}
            });
            return;
        case LightingPresetId::VolumesBacklit:
            area_lights.push_back({
                Vec3{0.0, 3.0, -4.2},
                Vec3{1.8, 0.0, 0.0},
                Vec3{0.0, 0.0, 1.8},
                Vec3{16.0, 15.0, 14.0}
            });
            point_lights.push_back({Vec3{-2.6, 2.0, -3.8}, Vec3{14.0, 14.0, 15.0}});
            point_lights.push_back({Vec3{2.6, 2.0, -3.8}, Vec3{15.0, 14.0, 14.0}});
            return;
    }
}
