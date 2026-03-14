#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "core/hit.h"

class BVHNode : public Hittable {
public:
    std::shared_ptr<Hittable> left;
    std::shared_ptr<Hittable> right;
    AABB box;

    BVHNode(std::vector<std::shared_ptr<Hittable>>& src_objects,
            size_t start,
            size_t end);

    bool hit(const Ray& r,
             double t_min,
             double t_max,
             HitRecord& rec) const override;

    bool bounding_box(AABB& output_box) const override;
};
