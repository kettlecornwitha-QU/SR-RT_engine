#pragma once

#include <memory>
#include <string>
#include <vector>

#include "core/material.h"
#include "render/settings.h"
#include "scene/presets/scene_params.h"
#include "scene/lights.h"
#include "scene/scene.h"

struct CameraConfig {
    Vec3 origin = {0, 0, 0};
    Vec3 forward = {0, 0, -1};
    Vec3 world_up = {0, 1, 0};
    double half_fov_x = 3.14159265358979323846 / 4.0;
};

struct SceneBuild {
    Scene world;
    std::vector<PointLight> point_lights;
    std::vector<RectAreaLight> area_lights;
    std::vector<std::shared_ptr<Material>> materials;
    CameraConfig camera;
};

bool build_scene_by_name(const std::string& name,
                         SceneBuild& s,
                         RenderSettings& settings,
                         const SceneParamOverrides& scene_param_overrides);
