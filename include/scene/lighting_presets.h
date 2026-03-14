#pragma once

#include <vector>

#include "scene/lights.h"

enum class LightingPresetId {
    ScatterDefault,
    BigScatterDefault,
    RowScatterDefault,
    MaterialsStudio,
    CornellCeiling,
    CornellCeilingPlusKey,
    BenchmarkSoftbox,
    VolumesBacklit
};

const char* lighting_preset_name(LightingPresetId id);

LightingPresetId resolve_lighting_preset(int requested_preset,
                                         LightingPresetId scene_default);

void append_lighting_preset(std::vector<PointLight>& point_lights,
                            std::vector<RectAreaLight>& area_lights,
                            LightingPresetId id);
