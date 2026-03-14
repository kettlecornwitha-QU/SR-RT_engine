#pragma once

#include "core/material.h"

class GGXMetal : public Material {
public:
    Vec3 f0;
    double roughness;

    GGXMetal(const Vec3& base_f0, double r);

    Vec3 aov_albedo(const HitRecord& rec) const override;

    bool scatter(const Ray& ray_in,
                 const HitRecord& rec,
                 Vec3& attenuation,
                 Ray& scattered) const override;
};
