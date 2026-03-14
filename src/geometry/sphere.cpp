#include "geometry/sphere.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr double kPi = 3.14159265358979323846;
}


Sphere::Sphere(const Vec3& c, double r, const Material* m)
    : center(c), radius(r), material(m) {}

bool Sphere::hit(const Ray& r,
                 double t_min,
                 double t_max,
                 HitRecord& rec) const
{
    Vec3 oc = r.origin - center;

    double a = r.direction.dot(r.direction);
    double b = 2.0 * oc.dot(r.direction);
    double c = oc.dot(oc) - radius * radius;

    double discriminant = b * b - 4 * a * c;
    if (discriminant < 0)
        return false;

    double sqrt_d = std::sqrt(discriminant);

    double root = (-b - sqrt_d) / (2.0 * a);
    if (root < t_min || root > t_max) {
        root = (-b + sqrt_d) / (2.0 * a);
        if (root < t_min || root > t_max)
            return false;
    }

    rec.t = root;
    rec.point = r.at(rec.t);
    Vec3 outward_normal = (rec.point - center) / radius;
    rec.set_face_normal(r, outward_normal);
    double theta = std::acos(std::max(-1.0, std::min(1.0, -outward_normal.y)));
    double phi = std::atan2(-outward_normal.z, outward_normal.x) + kPi;
    rec.u = phi / (2.0 * kPi);
    rec.v = theta / kPi;
    rec.material = material;

    return true;
}

bool Sphere::bounding_box(AABB& output_box) const {
    Vec3 rvec = {radius, radius, radius};
    output_box = {center - rvec, center + rvec};
    return true;
}
