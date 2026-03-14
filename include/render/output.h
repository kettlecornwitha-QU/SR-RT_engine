#pragma once

#include <string>
#include <vector>

#include "core/hit.h"

std::vector<unsigned char> make_ldr_buffer(const std::vector<Vec3>& framebuffer,
                                           int width,
                                           int height,
                                           double exposure);

std::vector<unsigned char> make_aov_ldr_buffer(const std::vector<Vec3>& framebuffer,
                                               int width,
                                               int height,
                                               bool is_normal);

bool gather_primary_aovs(const Ray& r,
                         const Hittable& world,
                         Vec3& albedo,
                         Vec3& normal);

bool write_ppm(const std::string& path,
               const std::vector<unsigned char>& rgb,
               int width,
               int height);

bool write_png_if_available(const std::string& path,
                            const std::vector<unsigned char>& rgb,
                            int width,
                            int height);

bool write_exr_if_available(const std::string& path,
                            const std::vector<Vec3>& framebuffer,
                            int width,
                            int height);

bool denoise_oidn_if_available(const std::vector<Vec3>& beauty,
                               const std::vector<Vec3>& albedo,
                               const std::vector<Vec3>& normal,
                               int width,
                               int height,
                               std::vector<Vec3>& denoised,
                               std::string& message);
