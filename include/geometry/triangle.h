#pragma once

#include "core/hit.h"

class Triangle : public Hittable {
public:
    Vec3 v0;
    Vec3 v1;
    Vec3 v2;
    Vec3 uv0;
    Vec3 uv1;
    Vec3 uv2;
    Vec3 n0;
    Vec3 n1;
    Vec3 n2;
    bool has_uv = false;
    bool has_vertex_normals = false;
    const Material* material;

    Triangle(const Vec3& a, const Vec3& b, const Vec3& c, const Material* m);
    Triangle(const Vec3& a,
             const Vec3& b,
             const Vec3& c,
             const Material* m,
             const Vec3& ta,
             const Vec3& tb,
             const Vec3& tc,
             const Vec3& na,
             const Vec3& nb,
             const Vec3& nc,
             bool use_uv,
             bool use_normals);

    bool hit(const Ray& r,
             double t_min,
             double t_max,
             HitRecord& rec) const override;

    bool bounding_box(AABB& output_box) const override;
};
