#pragma once

#include <cstdint>
#include <vector>

#include "math/vec3.h"
#include "render/settings.h"
#include "scene/scene_builder.h"

struct RenderResult {
    std::vector<Vec3> framebuffer;
    std::vector<Vec3> albedo_aov;
    std::vector<Vec3> normal_aov;
    double elapsed_seconds = 0.0;
    unsigned int thread_count = 1;
    uint64_t total_primary_samples = 0;
};

RenderResult render_scene(const SceneBuild& scene,
                          const RenderSettings& settings);
