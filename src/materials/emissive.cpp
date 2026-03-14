#include "materials/emissive.h"

Emissive::Emissive(const Vec3& e) : radiance(e) {}

Vec3 Emissive::aov_albedo(const HitRecord& rec) const {
    (void)rec;
    return {0, 0, 0};
}

Vec3 Emissive::emitted(const HitRecord& rec) const {
    (void)rec;
    return radiance;
}

bool Emissive::scatter(const Ray& ray_in,
                       const HitRecord& rec,
                       Vec3& attenuation,
                       Ray& scattered) const
{
    (void)ray_in;
    (void)rec;
    (void)attenuation;
    (void)scattered;
    return false;
}
