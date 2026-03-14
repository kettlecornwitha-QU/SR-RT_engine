#include "render/quality.h"

#include <cmath>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

namespace {

double clamp01(double x) {
    if (x < 0.0)
        return 0.0;
    if (x > 1.0)
        return 1.0;
    return x;
}

} // namespace

bool load_ppm(const std::string& path, LdrImage& img) {
    std::ifstream in(path);
    if (!in)
        return false;

    std::string magic;
    in >> magic;
    if (magic != "P3")
        return false;

    int width = 0;
    int height = 0;
    int maxv = 0;
    in >> width >> height >> maxv;
    if (!in || width <= 0 || height <= 0 || maxv <= 0)
        return false;

    img.width = width;
    img.height = height;
    img.rgb.assign(width * height * 3, 0.0);

    for (int i = 0; i < width * height * 3; ++i) {
        int v = 0;
        in >> v;
        if (!in)
            return false;
        img.rgb[i] = clamp01(static_cast<double>(v) / maxv);
    }
    return true;
}

double compute_rmse(const LdrImage& a, const LdrImage& b) {
    if (a.width != b.width || a.height != b.height || a.rgb.size() != b.rgb.size())
        return std::numeric_limits<double>::infinity();

    double mse = 0.0;
    for (size_t i = 0; i < a.rgb.size(); ++i) {
        double d = a.rgb[i] - b.rgb[i];
        mse += d * d;
    }
    mse /= static_cast<double>(a.rgb.size());
    return std::sqrt(mse);
}

double compute_ssim_luma(const LdrImage& a, const LdrImage& b) {
    if (a.width != b.width || a.height != b.height || a.rgb.size() != b.rgb.size())
        return -1.0;

    const int pixels = a.width * a.height;
    if (pixels == 0)
        return -1.0;

    std::vector<double> ya(pixels, 0.0);
    std::vector<double> yb(pixels, 0.0);
    for (int i = 0; i < pixels; ++i) {
        const int idx = i * 3;
        ya[i] = 0.2126 * a.rgb[idx + 0] + 0.7152 * a.rgb[idx + 1] + 0.0722 * a.rgb[idx + 2];
        yb[i] = 0.2126 * b.rgb[idx + 0] + 0.7152 * b.rgb[idx + 1] + 0.0722 * b.rgb[idx + 2];
    }

    double mu_a = 0.0;
    double mu_b = 0.0;
    for (int i = 0; i < pixels; ++i) {
        mu_a += ya[i];
        mu_b += yb[i];
    }
    mu_a /= pixels;
    mu_b /= pixels;

    double var_a = 0.0;
    double var_b = 0.0;
    double cov = 0.0;
    for (int i = 0; i < pixels; ++i) {
        double da = ya[i] - mu_a;
        double db = yb[i] - mu_b;
        var_a += da * da;
        var_b += db * db;
        cov += da * db;
    }
    var_a /= pixels;
    var_b /= pixels;
    cov /= pixels;

    const double c1 = 0.01 * 0.01;
    const double c2 = 0.03 * 0.03;

    return ((2.0 * mu_a * mu_b + c1) * (2.0 * cov + c2)) /
           ((mu_a * mu_a + mu_b * mu_b + c1) * (var_a + var_b + c2));
}
