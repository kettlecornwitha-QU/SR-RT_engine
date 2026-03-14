#include "scene/color/scatter_color.h"

#include <algorithm>
#include <cmath>

#include "render/settings.h"
#include "scene/random/hash.h"

namespace {

double saturate(double x) {
    return std::max(0.0, std::min(1.0, x));
}

Vec3 mixv(const Vec3& a, const Vec3& b, double t) {
    return a * (1.0 - t) + b * t;
}

} // namespace

namespace scene::color {

Vec3 make_scatter_blue_or_red_base(int gx, int gz, double dz, double max_sat_dz) {
    double t_sat = saturate(std::fabs(dz) / max_sat_dz);
    double saturation = 0.18 + 0.82 * t_sat;
    double value = 0.45 + 0.45 * scene::random::hash01(gx, gz, scene::random::SeedChannel::ScatterColorValue);
    Vec3 gray = {value, value, value};

    Vec3 hue;
    if (dz < 0.0) {
        hue = {
            0.08 + 0.14 * scene::random::hash01(gx, gz, scene::random::SeedChannel::ScatterColorHueA),
            0.20 + 0.24 * scene::random::hash01(gx, gz, scene::random::SeedChannel::ScatterColorHueB),
            0.85 + 0.15 * scene::random::hash01(gx, gz, scene::random::SeedChannel::ScatterColorHueC)
        };
    } else {
        hue = {
            0.85 + 0.15 * scene::random::hash01(gx, gz, scene::random::SeedChannel::ScatterColorHueA),
            0.08 + 0.14 * scene::random::hash01(gx, gz, scene::random::SeedChannel::ScatterColorHueB),
            0.08 + 0.14 * scene::random::hash01(gx, gz, scene::random::SeedChannel::ScatterColorHueC)
        };
    }

    Vec3 base = mixv(gray, hue * value, saturation);
    return {
        saturate(base.x),
        saturate(base.y),
        saturate(base.z)
    };
}

Vec3 make_big_scatter_pleasant_base(int gx, int gz, double dist_t, int palette_mode) {
    static const Vec3 kPaletteBalanced[] = {
        Vec3{0.78, 0.46, 0.35}, // terracotta
        Vec3{0.68, 0.57, 0.40}, // ochre
        Vec3{0.56, 0.66, 0.48}, // sage
        Vec3{0.43, 0.63, 0.66}, // muted teal
        Vec3{0.46, 0.54, 0.72}, // slate blue
        Vec3{0.66, 0.52, 0.70}, // dusty violet
        Vec3{0.72, 0.60, 0.52}, // warm taupe
        Vec3{0.52, 0.52, 0.56}  // cool gray
    };
    static const Vec3 kPaletteVivid[] = {
        Vec3{0.84, 0.40, 0.34}, // warm red
        Vec3{0.88, 0.63, 0.32}, // amber
        Vec3{0.49, 0.73, 0.44}, // green
        Vec3{0.33, 0.72, 0.70}, // cyan
        Vec3{0.37, 0.50, 0.84}, // blue
        Vec3{0.74, 0.46, 0.82}, // violet
        Vec3{0.87, 0.56, 0.68}, // rose
        Vec3{0.60, 0.62, 0.66}  // neutral support
    };
    static const Vec3 kPaletteEarthy[] = {
        Vec3{0.60, 0.42, 0.33}, // clay
        Vec3{0.55, 0.48, 0.34}, // olive-brown
        Vec3{0.47, 0.56, 0.40}, // moss
        Vec3{0.41, 0.52, 0.47}, // desaturated teal
        Vec3{0.45, 0.49, 0.57}, // slate
        Vec3{0.57, 0.50, 0.46}, // stone
        Vec3{0.64, 0.56, 0.47}, // sand
        Vec3{0.49, 0.47, 0.45}  // umber gray
    };

    const Vec3* palette = kPaletteBalanced;
    if (palette_mode == RenderSettings::BigScatterPaletteVivid) {
        palette = kPaletteVivid;
    } else if (palette_mode == RenderSettings::BigScatterPaletteEarthy) {
        palette = kPaletteEarthy;
    }
    constexpr int kPaletteSize = static_cast<int>(sizeof(kPaletteBalanced) / sizeof(kPaletteBalanced[0]));

    const int idx = static_cast<int>(
        scene::random::hash01(gx, gz, scene::random::SeedChannel::BigScatterPaletteIndex) * kPaletteSize) % kPaletteSize;
    const Vec3 primary = palette[idx];
    const Vec3 secondary = palette[(idx + 1 + (gx & 1)) % kPaletteSize];

    const double blend = 0.08 + 0.22 * scene::random::hash01(gx, gz, scene::random::SeedChannel::BigScatterPaletteBlend);
    const Vec3 mixed = mixv(primary, secondary, blend);

    const Vec3 neutral = (palette_mode == RenderSettings::BigScatterPaletteEarthy)
        ? Vec3{0.56, 0.55, 0.53}
        : Vec3{0.62, 0.62, 0.62};
    double neutral_pull = 0.18 + 0.18 * scene::random::hash01(gx, gz, scene::random::SeedChannel::BigScatterNeutralPull);
    if (palette_mode == RenderSettings::BigScatterPaletteVivid)
        neutral_pull *= 0.75;
    if (palette_mode == RenderSettings::BigScatterPaletteEarthy)
        neutral_pull *= 1.15;
    Vec3 base = mixv(mixed, neutral, neutral_pull);

    double gain = 0.94 + 0.14 * dist_t + 0.06 * scene::random::hash01(gx, gz, scene::random::SeedChannel::BigScatterGainJitter);
    if (palette_mode == RenderSettings::BigScatterPaletteVivid)
        gain += 0.04;
    base = base * gain;

    return {
        saturate(base.x),
        saturate(base.y),
        saturate(base.z)
    };
}

Vec3 make_row_scatter_side_color(double x, int row_idx, int col_idx, double x_max) {
    const double sat_x_max = std::max(1e-9, (2.0 / 3.0) * x_max);
    const double t = std::max(0.0, std::min(1.0, std::fabs(x) / sat_x_max));
    const double saturation = 0.22 + 0.72 * t;
    const double value = 0.44 + 0.46 * scene::random::hash01(row_idx, col_idx, scene::random::SeedChannel::RowScatterColorValue);
    Vec3 gray = {value, value, value};

    Vec3 hue;
    if (x < 0.0) {
        hue = {
            0.07 + 0.16 * scene::random::hash01(row_idx, col_idx, scene::random::SeedChannel::RowScatterColorHueA),
            0.20 + 0.24 * scene::random::hash01(row_idx, col_idx, scene::random::SeedChannel::RowScatterColorHueB),
            0.82 + 0.16 * scene::random::hash01(row_idx, col_idx, scene::random::SeedChannel::RowScatterColorHueC)
        };
    } else {
        hue = {
            0.82 + 0.16 * scene::random::hash01(row_idx, col_idx, scene::random::SeedChannel::RowScatterColorHueA),
            0.08 + 0.14 * scene::random::hash01(row_idx, col_idx, scene::random::SeedChannel::RowScatterColorHueB),
            0.08 + 0.14 * scene::random::hash01(row_idx, col_idx, scene::random::SeedChannel::RowScatterColorHueC)
        };
    }

    Vec3 base = mixv(gray, hue * value, saturation);
    return {
        saturate(base.x),
        saturate(base.y),
        saturate(base.z)
    };
}

} // namespace scene::color
