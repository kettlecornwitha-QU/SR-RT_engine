#pragma once

#include "core/material.h"

class ThinTransmission : public Material {
public:
    Vec3 tint;
    double ior;
    double roughness;

    ThinTransmission(const Vec3& transmittance_tint,
                     double index_of_refraction = 1.5,
                     double surface_roughness = 0.02);

    Vec3 aov_albedo(const HitRecord& rec) const override;

    bool scatter(const Ray& ray_in,
                 const HitRecord& rec,
                 Vec3& attenuation,
                 Ray& scattered) const override;
};

