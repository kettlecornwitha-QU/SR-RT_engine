#pragma once

#include "math/vec3.h"

namespace aberration {

Vec3 world_velocity_from_turns(double speed,
                               double yaw_turns,
                               double pitch_turns);

Vec3 relativistic_aberration(const Vec3& p_prime,
                             const Vec3& velocity);

} // namespace aberration

