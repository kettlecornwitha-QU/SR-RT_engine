#pragma once

#include "core/aabb.h"

class Material;

struct HitRecord {
    Vec3 point;
    Vec3 normal;
    double t;
    double u = 0.0;
    double v = 0.0;
    const Material* material;
    bool front_face;

    void set_face_normal(const Ray& r, const Vec3& outward_normal) {
        front_face = r.direction.dot(outward_normal) < 0.0;
        normal = front_face ? outward_normal : (-1.0 * outward_normal);
    }
};

class Hittable {
public:
    virtual ~Hittable() = default;
    virtual bool hit(
        const Ray& r,
        double t_min,
        double t_max,
        HitRecord& rec
    ) const = 0;
    virtual bool bounding_box(AABB& output_box) const = 0;
};
