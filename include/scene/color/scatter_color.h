#pragma once

#include "math/vec3.h"

namespace scene::color {

Vec3 make_scatter_blue_or_red_base(int gx, int gz, double dz, double max_sat_dz);

Vec3 make_big_scatter_pleasant_base(int gx, int gz, double dist_t, int palette_mode);

Vec3 make_row_scatter_side_color(double x, int row_idx, int col_idx, double x_max);

} // namespace scene::color
