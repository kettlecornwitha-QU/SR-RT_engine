#pragma once

#include "math/vec3.h"

Vec3 random_in_unit_disk();
Vec3 random_cosine_direction();
Vec3 local_to_world(const Vec3& local, const Vec3& normal);
