#pragma once

#include "core/hit.h"

class Plane : public Hittable {
public:
    Vec3 point;
    Vec3 normal;
    const Material* material;

    Plane(const Vec3& p,
          const Vec3& n,
          const Material* m);

    bool hit(const Ray& r,
             double t_min,
             double t_max,
             HitRecord& rec) const override;

    bool bounding_box(AABB& output_box) const override;
};
