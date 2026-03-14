#pragma once

#include "math/vec3.h"

Vec3 apply_camera_yaw_pitch_turns(const Vec3& forward,
                                  const Vec3& world_up,
                                  double yaw_turns,
                                  double pitch_turns);

