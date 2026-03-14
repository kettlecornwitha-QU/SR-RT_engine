#include "render/atmosphere.h"

#include <algorithm>
#include <cmath>

Vec3 apply_atmospheric_perspective(const Vec3& surface_radiance,
                                   double segment_distance,
                                   const RenderSettings& settings)
{
    if (!settings.atmosphere_enabled || settings.atmosphere_density <= 0.0)
        return surface_radiance;

    const double transmittance =
        std::exp(-settings.atmosphere_density * std::max(0.0, segment_distance));
    return surface_radiance * transmittance +
           settings.atmosphere_color * (1.0 - transmittance);
}

