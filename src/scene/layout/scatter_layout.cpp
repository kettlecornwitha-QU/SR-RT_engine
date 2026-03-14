#include "scene/layout/scatter_layout.h"

#include <algorithm>
#include <cmath>

#include "scene/random/hash.h"

namespace scene::layout {

double hash01(int x, int z, int seed) {
    return scene::random::hash01(x, z, seed);
}

double row_scatter_radius_from_abs_x(double abs_x, double x_max) {
    const double t = std::max(0.0, std::min(1.0, abs_x / x_max));
    const double width_scale = std::max(0.0, x_max / 18.0 - 1.0);
    const double max_radius = 1.30 + 0.70 * width_scale;
    return 0.20 + (max_radius - 0.20) * std::pow(t, 2.25);
}

std::vector<double> build_row_scatter_positive_columns(double x_max) {
    std::vector<double> positive_columns;
    double x = 0.35;
    while (x <= x_max) {
        const double r = row_scatter_radius_from_abs_x(x, x_max);
        positive_columns.push_back(x);
        double x_next = x + 2.2 * r;
        for (int iter = 0; iter < 2; ++iter) {
            const double r_next = row_scatter_radius_from_abs_x(x_next, x_max);
            x_next = x + (r + r_next) + 0.30 * std::min(r, r_next);
        }
        x = x_next;
    }
    return positive_columns;
}

std::vector<double> build_symmetric_columns_from_positive(const std::vector<double>& positive_columns) {
    std::vector<double> columns;
    columns.reserve(positive_columns.size() * 2);
    for (auto it = positive_columns.rbegin(); it != positive_columns.rend(); ++it)
        columns.push_back(-(*it));
    for (double x : positive_columns)
        columns.push_back(x);
    return columns;
}

} // namespace scene::layout
