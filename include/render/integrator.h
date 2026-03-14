#pragma once

#include <vector>

#include "core/hit.h"
#include "render/settings.h"
#include "scene/lights.h"

Vec3 trace(const Ray& r,
           const Hittable& world,
           const std::vector<PointLight>& point_lights,
           const std::vector<RectAreaLight>& area_lights,
           const RenderSettings& settings,
           int depth,
           bool prev_was_lambertian = false,
           const Vec3& prev_point = {0, 0, 0},
           const Vec3& prev_normal = {0, 0, 0});
