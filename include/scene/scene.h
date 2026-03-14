#pragma once

#include <memory>
#include <vector>

#include "core/hit.h"

class Scene : public Hittable {
public:
    std::vector<std::shared_ptr<Hittable>> objects;
    std::vector<std::shared_ptr<Hittable>> non_bounded_objects;
    std::shared_ptr<Hittable> bvh_root;
    bool bvh_built = false;

    void add(std::shared_ptr<Hittable> obj);
    void build_bvh();

    bool hit(const Ray& r,
             double t_min,
             double t_max,
             HitRecord& rec) const override;

    bool bounding_box(AABB& output_box) const override;
};
