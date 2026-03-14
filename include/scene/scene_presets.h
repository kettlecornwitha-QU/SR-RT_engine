#pragma once

#include <string>
#include <vector>

#include "render/settings.h"
#include "scene/presets/scene_params.h"
#include "scene/scene_builder.h"

bool build_standard_scene_by_name(const std::string& name,
                                  SceneBuild& s,
                                  RenderSettings& settings,
                                  const SceneParamOverrides& scene_param_overrides);

struct ModelScenePreset {
    RenderSettings render_settings{};
    CameraConfig camera{};
    std::vector<PointLight> point_lights;
    std::vector<RectAreaLight> area_lights;
    std::vector<std::string> model_candidates;
};

bool get_model_scene_preset_by_name(const std::string& name,
                                    ModelScenePreset& preset);

std::vector<std::string> list_primary_scene_names();
std::vector<std::string> list_scene_alias_names();
