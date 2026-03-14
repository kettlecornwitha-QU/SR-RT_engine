#pragma once

#include <algorithm>
#include <cmath>

#include "core/ray.h"

struct AABB {
    Vec3 minimum;
    Vec3 maximum;

    bool hit(const Ray& r, double t_min, double t_max) const {
        for (int a = 0; a < 3; ++a) {
            double origin = (a == 0) ? r.origin.x : (a == 1 ? r.origin.y : r.origin.z);
            double direction = (a == 0) ? r.direction.x : (a == 1 ? r.direction.y : r.direction.z);
            double minv = (a == 0) ? minimum.x : (a == 1 ? minimum.y : minimum.z);
            double maxv = (a == 0) ? maximum.x : (a == 1 ? maximum.y : maximum.z);

            if (std::fabs(direction) < 1e-12) {
                if (origin < minv || origin > maxv)
                    return false;
                continue;
            }

            double inv_d = 1.0 / direction;
            double t0 = (minv - origin) * inv_d;
            double t1 = (maxv - origin) * inv_d;
            if (inv_d < 0.0)
                std::swap(t0, t1);

            t_min = std::max(t0, t_min);
            t_max = std::min(t1, t_max);
            if (t_max <= t_min)
                return false;
        }
        return true;
    }
};

inline AABB surrounding_box(const AABB& box0, const AABB& box1) {
    return {
        min_vec3(box0.minimum, box1.minimum),
        max_vec3(box0.maximum, box1.maximum)
    };
}
