#pragma once

#include "scene/presets/scene_params.h"
#include "scene/scatter_material_factory.h"

struct SceneBuild;
struct RenderSettings;

void build_scatter_scene(SceneBuild& s,
                         RenderSettings& settings,
                         ScatterMaterialMode scatter_mode,
                         const ScatterParams& params);
