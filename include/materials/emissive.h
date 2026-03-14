#pragma once

#include "core/material.h"

class Emissive : public Material {
public:
    Vec3 radiance;

    explicit Emissive(const Vec3& e);

    Vec3 aov_albedo(const HitRecord& rec) const override;
    Vec3 emitted(const HitRecord& rec) const override;

    bool scatter(const Ray& ray_in,
                 const HitRecord& rec,
                 Vec3& attenuation,
                 Ray& scattered) const override;
};
