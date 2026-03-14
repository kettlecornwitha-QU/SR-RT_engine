#pragma once

#include <vector>

namespace scene::layout {

double hash01(int x, int z, int seed);

double row_scatter_radius_from_abs_x(double abs_x, double x_max);

std::vector<double> build_row_scatter_positive_columns(double x_max);

std::vector<double> build_symmetric_columns_from_positive(const std::vector<double>& positive_columns);

} // namespace scene::layout
