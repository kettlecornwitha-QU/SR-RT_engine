#pragma once

#include "core/material.h"

class Subsurface : public Material {
public:
    Vec3 albedo;
    double mean_free_path;
    double subsurface_mix;
    double forward_bias;
    bool random_walk_mode;

    Subsurface(const Vec3& a,
               double mfp,
               double mix,
               double bias,
               bool random_walk = false);

    Vec3 aov_albedo(const HitRecord& rec) const override;

    bool scatter(const Ray& ray_in,
                 const HitRecord& rec,
                 Vec3& attenuation,
                 Ray& scattered) const override;
};
