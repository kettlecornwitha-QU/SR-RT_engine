#include "render/aberration.h"

#include <cmath>

namespace aberration {

namespace {

constexpr double kPi = 3.14159265358979323846;

} // namespace

Vec3 world_velocity_from_turns(double speed,
                               double yaw_turns,
                               double pitch_turns)
{
    if (speed <= 0.0)
        return {0.0, 0.0, 0.0};

    double yaw = (yaw_turns - std::floor(yaw_turns)) * 2.0 * kPi;
    double pitch = pitch_turns * 2.0 * kPi;
    double cos_pitch = std::cos(pitch);

    Vec3 dir = {
        std::sin(yaw) * cos_pitch,
        std::sin(pitch),
        -std::cos(yaw) * cos_pitch
    };

    if (dir.norm_squared() > 1e-12)
        dir = dir.normalize();
    else
        dir = {0.0, 0.0, -1.0};

    return dir * speed;
}

Vec3 relativistic_aberration(const Vec3& p_prime,
                             const Vec3& velocity)
{
    double beta2 = velocity.norm_squared();
    if (beta2 == 0.0)
        return p_prime;

    double gamma = 1.0 / std::sqrt(1.0 - beta2);
    double v_dot_p = velocity.dot(p_prime);
    double p_mag = p_prime.norm();

    double factor =
        ((gamma - 1.0) / beta2) * v_dot_p
        - gamma * p_mag;

    return p_prime + factor * velocity;
}

} // namespace aberration

