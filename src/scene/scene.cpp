#include "scene/scene.h"

#include "accel/bvh.h"

void Scene::add(std::shared_ptr<Hittable> obj) {
    objects.push_back(obj);
    bvh_built = false;
}

void Scene::build_bvh() {
    std::vector<std::shared_ptr<Hittable>> bounded;
    non_bounded_objects.clear();
    for (const auto& obj : objects) {
        AABB b;
        if (obj->bounding_box(b))
            bounded.push_back(obj);
        else
            non_bounded_objects.push_back(obj);
    }

    if (bounded.empty()) {
        bvh_root.reset();
    } else {
        bvh_root = std::make_shared<BVHNode>(bounded, 0, bounded.size());
    }
    bvh_built = true;
}

bool Scene::hit(const Ray& r,
                double t_min,
                double t_max,
                HitRecord& rec) const
{
    if (!bvh_built) {
        HitRecord temp;
        bool hit_anything = false;
        double closest = t_max;
        for (const auto& obj : objects) {
            if (obj->hit(r, t_min, closest, temp)) {
                hit_anything = true;
                closest = temp.t;
                rec = temp;
            }
        }
        return hit_anything;
    }

    HitRecord temp;
    bool hit_anything = false;
    double closest = t_max;

    if (bvh_root && bvh_root->hit(r, t_min, closest, temp)) {
        hit_anything = true;
        closest = temp.t;
        rec = temp;
    }

    for (const auto& obj : non_bounded_objects) {
        if (obj->hit(r, t_min, closest, temp)) {
            hit_anything = true;
            closest = temp.t;
            rec = temp;
        }
    }

    return hit_anything;
}

bool Scene::bounding_box(AABB& output_box) const {
    if (bvh_root)
        return bvh_root->bounding_box(output_box);
    (void)output_box;
    return false;
}
