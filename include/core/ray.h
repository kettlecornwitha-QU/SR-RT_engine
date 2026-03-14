#pragma once

#include "math/vec3.h"

struct Ray {
    Vec3 origin;
    Vec3 direction;

    Vec3 at(double t) const {
        return origin + direction * t;
    }
};
