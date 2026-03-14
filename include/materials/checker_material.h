#pragma once

#include "core/material.h"

class CheckerMaterial : public Material {
public:
    const Material* material_a;
    const Material* material_b;
    double scale_u;
    double scale_v;

    CheckerMaterial(const Material* a,
                    const Material* b,
                    double su = 1.0,
                    double sv = 1.0);

    Vec3 emitted(const HitRecord& rec) const override;
    Vec3 aov_albedo(const HitRecord& rec) const override;

    bool scatter(const Ray& ray_in,
                 const HitRecord& rec,
                 Vec3& attenuation,
                 Ray& scattered) const override;

private:
    const Material* pick(const HitRecord& rec) const;
};
