#pragma once

#include "scene/presets/scene_params.h"
#include "scene/scene_builder.h"

void build_row_scatter_scene(SceneBuild& s,
                             RenderSettings& settings,
                             const RowScatterParams& params);
