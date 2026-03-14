#pragma once

#include "core/ray.h"

struct HitRecord;

class Material {
public:
    virtual ~Material() = default;
    virtual Vec3 emitted(const HitRecord& rec) const {
        (void)rec;
        return {0,0,0};
    }
    virtual Vec3 aov_albedo(const HitRecord& rec) const {
        (void)rec;
        return {1,1,1};
    }
    virtual bool scatter(
        const Ray& ray_in,
        const HitRecord& rec,
        Vec3& attenuation,
        Ray& scattered
    ) const = 0;
};
