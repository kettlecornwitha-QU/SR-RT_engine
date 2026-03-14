#pragma once

#include "math/vec3.h"
#include "render/settings.h"

Vec3 apply_atmospheric_perspective(const Vec3& surface_radiance,
                                   double segment_distance,
                                   const RenderSettings& settings);

