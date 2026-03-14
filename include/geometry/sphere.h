#pragma once

#include "core/hit.h"

class Sphere : public Hittable {
public:
    Vec3 center;
    double radius;
    const Material* material;

    Sphere(const Vec3& c, double r, const Material* m);

    bool hit(const Ray& r,
             double t_min,
             double t_max,
             HitRecord& rec) const override;

    bool bounding_box(AABB& output_box) const override;
};
