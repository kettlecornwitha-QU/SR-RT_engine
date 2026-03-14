#include "geometry/rectangle.h"

#include <cmath>

#include "math/vec3.h"

Rectangle::Rectangle(const Vec3& c,
                     const Vec3& u,
                     const Vec3& v,
                     const Material* m)
    : center(c),
      edge_u(u),
      edge_v(v),
      material(m) {}

bool Rectangle::hit(const Ray& r,
                    double t_min,
                    double t_max,
                    HitRecord& rec) const
{
    Vec3 normal = edge_u.cross(edge_v).normalize();
    double denom = normal.dot(r.direction);
    if (std::fabs(denom) < 1e-8)
        return false;

    double t = (center - r.origin).dot(normal) / denom;
    if (t < t_min || t > t_max)
        return false;

    Vec3 p = r.at(t);
    Vec3 rel = p - center;

    double u_len = edge_u.norm();
    double v_len = edge_v.norm();
    if (u_len <= 1e-10 || v_len <= 1e-10)
        return false;

    Vec3 u_axis = edge_u / u_len;
    Vec3 v_axis = edge_v / v_len;
    double u_proj = rel.dot(u_axis);
    double v_proj = rel.dot(v_axis);

    if (std::fabs(u_proj) > 0.5 * u_len ||
        std::fabs(v_proj) > 0.5 * v_len)
        return false;

    rec.t = t;
    rec.point = p;
    rec.set_face_normal(r, normal);
    rec.u = u_proj / u_len + 0.5;
    rec.v = v_proj / v_len + 0.5;
    rec.material = material;
    return true;
}

bool Rectangle::bounding_box(AABB& output_box) const {
    Vec3 p0 = center + 0.5 * edge_u + 0.5 * edge_v;
    Vec3 p1 = center + 0.5 * edge_u - 0.5 * edge_v;
    Vec3 p2 = center - 0.5 * edge_u + 0.5 * edge_v;
    Vec3 p3 = center - 0.5 * edge_u - 0.5 * edge_v;

    Vec3 minp = min_vec3(min_vec3(p0, p1), min_vec3(p2, p3));
    Vec3 maxp = max_vec3(max_vec3(p0, p1), max_vec3(p2, p3));
    Vec3 eps = {1e-5, 1e-5, 1e-5};
    output_box = {minp - eps, maxp + eps};
    return true;
}
