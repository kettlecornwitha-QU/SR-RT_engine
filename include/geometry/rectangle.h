#pragma once

#include "core/hit.h"

class Rectangle : public Hittable {
public:
    Vec3 center;
    Vec3 edge_u;
    Vec3 edge_v;
    const Material* material;

    Rectangle(const Vec3& c,
              const Vec3& u,
              const Vec3& v,
              const Material* m);

    bool hit(const Ray& r,
             double t_min,
             double t_max,
             HitRecord& rec) const override;

    bool bounding_box(AABB& output_box) const override;
};
