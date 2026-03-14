#include "render/camera_controls.h"

#include <cmath>

namespace {

constexpr double kPi = 3.14159265358979323846;

Vec3 rotate_axis(const Vec3& v, Vec3 axis, double radians) {
    if (axis.norm_squared() <= 1e-12)
        return v;
    axis = axis.normalize();
    const double c = std::cos(radians);
    const double s = std::sin(radians);
    return v * c + axis.cross(v) * s + axis * (axis.dot(v) * (1.0 - c));
}

} // namespace

Vec3 apply_camera_yaw_pitch_turns(const Vec3& forward,
                                  const Vec3& world_up,
                                  double yaw_turns,
                                  double pitch_turns)
{
    Vec3 f = forward.normalize();
    if (f.norm_squared() <= 1e-12)
        f = Vec3{0.0, 0.0, -1.0};

    const double yaw_rad = yaw_turns * (2.0 * kPi);
    // Positive pitch turns should look upward (+Y), matching the aberration
    // pitch convention and common 3D camera controls.
    const double pitch_rad = -pitch_turns * (2.0 * kPi);

    Vec3 up = world_up.normalize();
    if (up.norm_squared() <= 1e-12)
        up = Vec3{0.0, 1.0, 0.0};

    if (std::fabs(yaw_rad) > 1e-12)
        f = rotate_axis(f, up, yaw_rad).normalize();

    Vec3 right = up.cross(f).normalize();
    if (right.norm_squared() <= 1e-12) {
        right = Vec3{1.0, 0.0, 0.0}.cross(f).normalize();
        if (right.norm_squared() <= 1e-12)
            right = Vec3{0.0, 0.0, 1.0};
    }
    if (std::fabs(pitch_rad) > 1e-12)
        f = rotate_axis(f, right, pitch_rad).normalize();

    return f;
}
