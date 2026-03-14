#include "accel/bvh.h"

#include <algorithm>

BVHNode::BVHNode(std::vector<std::shared_ptr<Hittable>>& src_objects,
                 size_t start,
                 size_t end)
{
    auto comparator = [](const std::shared_ptr<Hittable>& a,
                         const std::shared_ptr<Hittable>& b,
                         int axis) {
        AABB box_a, box_b;
        if (!a->bounding_box(box_a) || !b->bounding_box(box_b))
            return false;

        double a_min = axis == 0 ? box_a.minimum.x : (axis == 1 ? box_a.minimum.y : box_a.minimum.z);
        double b_min = axis == 0 ? box_b.minimum.x : (axis == 1 ? box_b.minimum.y : box_b.minimum.z);
        return a_min < b_min;
    };

    AABB centroid_box;
    bool centroid_init = false;
    for (size_t i = start; i < end; ++i) {
        AABB b;
        if (!src_objects[i]->bounding_box(b))
            continue;
        Vec3 c = (b.minimum + b.maximum) * 0.5;
        AABB cb = {c, c};
        centroid_box = centroid_init ? surrounding_box(centroid_box, cb) : cb;
        centroid_init = true;
    }

    Vec3 extent = centroid_box.maximum - centroid_box.minimum;
    int axis = 0;
    if (extent.y > extent.x && extent.y >= extent.z)
        axis = 1;
    else if (extent.z > extent.x && extent.z >= extent.y)
        axis = 2;

    size_t object_span = end - start;

    if (object_span == 1) {
        left = right = src_objects[start];
    } else if (object_span == 2) {
        if (comparator(src_objects[start], src_objects[start + 1], axis)) {
            left = src_objects[start];
            right = src_objects[start + 1];
        } else {
            left = src_objects[start + 1];
            right = src_objects[start];
        }
    } else {
        std::sort(src_objects.begin() + start,
                  src_objects.begin() + end,
                  [&](const std::shared_ptr<Hittable>& a,
                      const std::shared_ptr<Hittable>& b) {
                      return comparator(a, b, axis);
                  });

        size_t mid = start + object_span / 2;
        left = std::make_shared<BVHNode>(src_objects, start, mid);
        right = std::make_shared<BVHNode>(src_objects, mid, end);
    }

    AABB box_left, box_right;
    left->bounding_box(box_left);
    right->bounding_box(box_right);
    box = surrounding_box(box_left, box_right);
}

bool BVHNode::hit(const Ray& r,
                  double t_min,
                  double t_max,
                  HitRecord& rec) const
{
    if (!box.hit(r, t_min, t_max))
        return false;

    HitRecord left_rec;
    HitRecord right_rec;
    bool hit_left = left->hit(r, t_min, t_max, left_rec);
    bool hit_right = right->hit(r, t_min, hit_left ? left_rec.t : t_max, right_rec);

    if (hit_right) {
        rec = right_rec;
        return true;
    }
    if (hit_left) {
        rec = left_rec;
        return true;
    }
    return false;
}

bool BVHNode::bounding_box(AABB& output_box) const {
    output_box = box;
    return true;
}
