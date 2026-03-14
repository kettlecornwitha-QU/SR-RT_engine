#pragma once

#include "math/vec3.h"

struct PointLight {
    Vec3 position;
    Vec3 intensity;
};

struct RectAreaLight {
    Vec3 center;
    Vec3 edge_u;
    Vec3 edge_v;
    Vec3 emission;
};
