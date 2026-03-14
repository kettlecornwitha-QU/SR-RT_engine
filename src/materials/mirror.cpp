#include "materials/mirror.h"
#include "core/hit.h"

#include "render/constants.h"
#include "render/optics.h"

Mirror::Mirror(const Vec3& a) : albedo(a) {}

Vec3 Mirror::aov_albedo(const HitRecord& rec) const {
    (void)rec;
    return albedo;
}

bool Mirror::scatter(const Ray& ray_in,
                     const HitRecord& rec,
                     Vec3& attenuation,
                     Ray& scattered) const
{
    Vec3 reflected = reflect(ray_in.direction, rec.normal);

    scattered.origin = rec.point + rec.normal * kRayEpsilon;
    scattered.direction = reflected.normalize();
    attenuation = albedo;

    return true;
}
