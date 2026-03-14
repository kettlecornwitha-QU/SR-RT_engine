#pragma once

#include "core/material.h"

class CoatedPlastic : public Material {
public:
    Vec3 base_color;
    double coat_ior;
    double coat_roughness;
    double coat_strength;

    CoatedPlastic(const Vec3& base,
                  double ior = 1.5,
                  double roughness = 0.04,
                  double strength = 1.0);

    Vec3 aov_albedo(const HitRecord& rec) const override;

    bool scatter(const Ray& ray_in,
                 const HitRecord& rec,
                 Vec3& attenuation,
                 Ray& scattered) const override;
};

