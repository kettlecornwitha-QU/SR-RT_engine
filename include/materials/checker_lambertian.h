#pragma once

#include "materials/lambertian.h"

class CheckerLambertian : public Lambertian {
public:
    Vec3 color_a;
    Vec3 color_b;
    double scale_u;
    double scale_v;

    CheckerLambertian(const Vec3& a,
                      const Vec3& b,
                      double su = 1.0,
                      double sv = 1.0);

    Vec3 aov_albedo(const HitRecord& rec) const override;

    bool scatter(const Ray& ray_in,
                 const HitRecord& rec,
                 Vec3& attenuation,
                 Ray& scattered) const override;
};

