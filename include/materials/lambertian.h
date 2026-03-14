#pragma once

#include "core/material.h"

class Lambertian : public Material {
public:
    Vec3 albedo;

    explicit Lambertian(const Vec3& a);

    Vec3 aov_albedo(const HitRecord& rec) const override;

    bool scatter(const Ray& ray_in,
                 const HitRecord& rec,
                 Vec3& attenuation,
                 Ray& scattered) const override;
};
