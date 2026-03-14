#pragma once

#include "core/material.h"

class AnisotropicGGXMetal : public Material {
public:
    Vec3 f0;
    double roughness_x;
    double roughness_y;
    Vec3 tangent_hint;

    AnisotropicGGXMetal(const Vec3& base_f0,
                        double rx,
                        double ry,
                        const Vec3& tangent = {1, 0, 0});

    Vec3 aov_albedo(const HitRecord& rec) const override;

    bool scatter(const Ray& ray_in,
                 const HitRecord& rec,
                 Vec3& attenuation,
                 Ray& scattered) const override;
};

