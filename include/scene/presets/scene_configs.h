#pragma once

#include <vector>

#include "render/settings.h"
#include "scene/lights.h"
#include "scene/scene_builder.h"

struct ScenePresetConfig {
    bool apply_render_settings = false;
    RenderSettings render_settings{};
    bool apply_camera = false;
    CameraConfig camera{};
    std::vector<PointLight> point_lights;
    std::vector<RectAreaLight> area_lights;
};

ScenePresetConfig make_materials_config();
ScenePresetConfig make_cornell_config();
ScenePresetConfig make_benchmark_config();
ScenePresetConfig make_volumes_config();
