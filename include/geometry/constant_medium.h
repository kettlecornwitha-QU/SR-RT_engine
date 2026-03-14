#pragma once

#include <memory>

#include "core/hit.h"

class ConstantMedium : public Hittable {
public:
    std::shared_ptr<Hittable> boundary;
    double neg_inv_density;
    const Material* phase_function;

    ConstantMedium(std::shared_ptr<Hittable> b,
                   double density,
                   const Material* phase);

    bool hit(const Ray& r,
             double t_min,
             double t_max,
             HitRecord& rec) const override;

    bool bounding_box(AABB& output_box) const override;
};

