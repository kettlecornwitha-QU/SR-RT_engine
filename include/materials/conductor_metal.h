#pragma once

#include "core/material.h"

class ConductorMetal : public Material {
public:
    Vec3 eta;
    Vec3 k;
    double roughness;

    ConductorMetal(const Vec3& eta_rgb,
                   const Vec3& k_rgb,
                   double r);

    Vec3 aov_albedo(const HitRecord& rec) const override;

    bool scatter(const Ray& ray_in,
                 const HitRecord& rec,
                 Vec3& attenuation,
                 Ray& scattered) const override;
};

