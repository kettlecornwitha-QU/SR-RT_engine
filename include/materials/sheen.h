#pragma once

#include "core/material.h"

class Sheen : public Material {
public:
    Vec3 base_color;
    Vec3 sheen_color;
    double sheen_weight;

    Sheen(const Vec3& base,
          const Vec3& sheen,
          double weight = 0.35);

    Vec3 aov_albedo(const HitRecord& rec) const override;

    bool scatter(const Ray& ray_in,
                 const HitRecord& rec,
                 Vec3& attenuation,
                 Ray& scattered) const override;
};

