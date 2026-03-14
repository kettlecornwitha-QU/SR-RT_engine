#pragma once

#include <string>
#include <vector>

struct LdrImage {
    int width = 0;
    int height = 0;
    std::vector<double> rgb; // [0,1]
};

bool load_ppm(const std::string& path, LdrImage& img);
double compute_rmse(const LdrImage& a, const LdrImage& b);
double compute_ssim_luma(const LdrImage& a, const LdrImage& b);
