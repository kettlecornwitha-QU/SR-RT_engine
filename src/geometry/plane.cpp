#include "geometry/plane.h"

#include <cmath>

Plane::Plane(const Vec3& p,
             const Vec3& n,
             const Material* m)
    : point(p),
      normal(n.normalize()),
      material(m) {}

bool Plane::hit(const Ray& r,
                double t_min,
                double t_max,
                HitRecord& rec) const
{
    double denom = normal.dot(r.direction);

    if (std::fabs(denom) < 1e-6)
        return false;

    double t = (point - r.origin).dot(normal) / denom;

    if (t < t_min || t > t_max)
        return false;

    rec.t = t;
    rec.point = r.at(t);
    rec.set_face_normal(r, normal);
    rec.u = rec.point.x;
    rec.v = rec.point.z;
    rec.material = material;

    return true;
}

bool Plane::bounding_box(AABB& output_box) const {
    (void)output_box;
    return false;
}
