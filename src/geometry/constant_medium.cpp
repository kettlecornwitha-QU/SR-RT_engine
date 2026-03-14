#include "geometry/constant_medium.h"

#include <cmath>
#include <limits>

#include "render/rng.h"

ConstantMedium::ConstantMedium(std::shared_ptr<Hittable> b,
                               double density,
                               const Material* phase)
    : boundary(std::move(b)),
      neg_inv_density(-1.0 / density),
      phase_function(phase) {}

bool ConstantMedium::hit(const Ray& r,
                         double t_min,
                         double t_max,
                         HitRecord& rec) const
{
    HitRecord rec1;
    HitRecord rec2;

    if (!boundary->hit(r,
                       -std::numeric_limits<double>::infinity(),
                       std::numeric_limits<double>::infinity(),
                       rec1))
        return false;

    if (!boundary->hit(r,
                       rec1.t + 1e-4,
                       std::numeric_limits<double>::infinity(),
                       rec2))
        return false;

    if (rec1.t < t_min)
        rec1.t = t_min;
    if (rec2.t > t_max)
        rec2.t = t_max;

    if (rec1.t >= rec2.t)
        return false;

    if (rec1.t < 0.0)
        rec1.t = 0.0;

    double ray_length = r.direction.norm();
    double distance_inside_boundary = (rec2.t - rec1.t) * ray_length;
    double sample = std::max(1e-12, random_double());
    double hit_distance = neg_inv_density * std::log(sample);

    if (hit_distance > distance_inside_boundary)
        return false;

    rec.t = rec1.t + hit_distance / ray_length;
    rec.point = r.at(rec.t);
    rec.normal = {1.0, 0.0, 0.0}; // Arbitrary for isotropic phase function.
    rec.front_face = true;
    rec.material = phase_function;
    rec.u = 0.0;
    rec.v = 0.0;
    return true;
}

bool ConstantMedium::bounding_box(AABB& output_box) const {
    return boundary->bounding_box(output_box);
}

