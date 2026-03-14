#pragma once

#include "core/material.h"

class Dielectric : public Material {
public:
    double ior;

    explicit Dielectric(double index_of_refraction);

    Vec3 aov_albedo(const HitRecord& rec) const override;

    bool scatter(const Ray& ray_in,
                 const HitRecord& rec,
                 Vec3& attenuation,
                 Ray& scattered) const override;
};
