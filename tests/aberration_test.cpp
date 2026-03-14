#include <cmath>
#include <iostream>

#include "render/aberration.h"

namespace {

bool near(double a, double b, double eps = 1e-6) {
    return std::fabs(a - b) <= eps;
}

int require_vec3_close(const Vec3& got,
                       const Vec3& expected,
                       const char* label,
                       double eps = 1e-6)
{
    if (near(got.x, expected.x, eps) &&
        near(got.y, expected.y, eps) &&
        near(got.z, expected.z, eps))
        return 0;

    std::cerr << "FAIL " << label
              << " got=(" << got.x << "," << got.y << "," << got.z << ")"
              << " expected=(" << expected.x << "," << expected.y << "," << expected.z << ")\n";
    return 1;
}

} // namespace

int main() {
    int failures = 0;

    {
        Vec3 v = aberration::world_velocity_from_turns(0.7071067811865476, 0.125, 0.0);
        failures += require_vec3_close(v, Vec3{0.5, 0.0, -0.5}, "yaw_0.125_mapping", 1e-6);
    }
    {
        Vec3 v = aberration::world_velocity_from_turns(0.3, 0.0, 0.0);
        failures += require_vec3_close(v, Vec3{0.0, 0.0, -0.3}, "yaw_0_maps_to_minus_z");
    }
    {
        Vec3 v = aberration::world_velocity_from_turns(0.3, 0.25, 0.0);
        failures += require_vec3_close(v, Vec3{0.3, 0.0, 0.0}, "yaw_quarter_maps_to_plus_x");
    }
    {
        Vec3 v = aberration::world_velocity_from_turns(0.4, 0.125, 0.0);
        Vec3 w = aberration::world_velocity_from_turns(0.4, 1.125, 0.0);
        failures += require_vec3_close(v, w, "yaw_wraps_every_turn");
    }
    {
        Vec3 v = aberration::world_velocity_from_turns(0.2, 0.0, 0.25);
        failures += require_vec3_close(v, Vec3{0.0, 0.2, 0.0}, "pitch_quarter_maps_to_plus_y");
    }

    if (failures == 0) {
        std::cout << "aberration tests passed\n";
        return 0;
    }

    std::cerr << failures << " aberration tests failed\n";
    return 1;
}

