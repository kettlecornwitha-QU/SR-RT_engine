#include "geometry/triangle.h"

#include <cmath>

#include "math/vec3.h"

Triangle::Triangle(const Vec3& a, const Vec3& b, const Vec3& c, const Material* m)
    : v0(a),
      v1(b),
      v2(c),
      uv0({0, 0, 0}),
      uv1({0, 0, 0}),
      uv2({0, 0, 0}),
      n0({0, 0, 1}),
      n1({0, 0, 1}),
      n2({0, 0, 1}),
      has_uv(false),
      has_vertex_normals(false),
      material(m) {}

Triangle::Triangle(const Vec3& a,
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
                   bool use_normals)
    : v0(a),
      v1(b),
      v2(c),
      uv0(ta),
      uv1(tb),
      uv2(tc),
      n0(na),
      n1(nb),
      n2(nc),
      has_uv(use_uv),
      has_vertex_normals(use_normals),
      material(m) {}

bool Triangle::hit(const Ray& r,
                   double t_min,
                   double t_max,
                   HitRecord& rec) const
{
    Vec3 e1 = v1 - v0;
    Vec3 e2 = v2 - v0;
    Vec3 pvec = r.direction.cross(e2);
    double det = e1.dot(pvec);

    if (std::fabs(det) < 1e-9)
        return false;

    double inv_det = 1.0 / det;
    Vec3 tvec = r.origin - v0;
    double u = tvec.dot(pvec) * inv_det;
    if (u < 0.0 || u > 1.0)
        return false;

    Vec3 qvec = tvec.cross(e1);
    double v = r.direction.dot(qvec) * inv_det;
    if (v < 0.0 || u + v > 1.0)
        return false;

    double t = e2.dot(qvec) * inv_det;
    if (t < t_min || t > t_max)
        return false;

    const double bary_u = u;
    const double bary_v = v;
    const double bary_w = 1.0 - bary_u - bary_v;

    rec.t = t;
    rec.point = r.at(t);
    Vec3 outward_normal = e1.cross(e2).normalize();
    if (has_vertex_normals) {
        outward_normal =
            (n0 * bary_w + n1 * bary_u + n2 * bary_v).normalize();
    }
    rec.set_face_normal(r, outward_normal);
    if (has_uv) {
        Vec3 uv = uv0 * bary_w + uv1 * bary_u + uv2 * bary_v;
        rec.u = uv.x;
        rec.v = uv.y;
    } else {
        rec.u = bary_u;
        rec.v = bary_v;
    }
    rec.material = material;
    return true;
}

bool Triangle::bounding_box(AABB& output_box) const {
    Vec3 minp = min_vec3(v0, min_vec3(v1, v2));
    Vec3 maxp = max_vec3(v0, max_vec3(v1, v2));
    Vec3 eps = {1e-5, 1e-5, 1e-5};
    output_box = {minp - eps, maxp + eps};
    return true;
}
