#pragma once

#include "core/material.h"

class Clearcoat : public Material {
public:
    const Material* base_material;
    double coat_ior;
    double coat_roughness;

    Clearcoat(const Material* base,
              double ior = 1.5,
              double roughness = 0.05);

    Vec3 aov_albedo(const HitRecord& rec) const override;

    bool scatter(const Ray& ray_in,
                 const HitRecord& rec,
                 Vec3& attenuation,
                 Ray& scattered) const override;
};

